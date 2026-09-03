
#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/MythicTrade.h"
#include "Itemization/Storage/MythicStorageContainer.h"
#include "Itemization/Vendor/MythicEconomyPricing.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicVendor.generated.h"

class UItemDefinition;
class UMythicItemInstance;
class UMythicInventoryComponent;
class AMythicPlayerController;
class UProficiencyDefinition;
enum class EMythicStandingTier : uint8;

USTRUCT(BlueprintType)
struct FVendorReputationPricing {
    GENERATED_BODY()

    // Multipliers on the BUY price by buyer tier (×BuyPriceMultiplier). <1 = discount, >1 = surcharge.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float HostileBuyMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float NeutralBuyMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float FriendlyBuyMultiplier = 1.0f;

    // Multipliers on the SELL payout by seller tier (×SellRate). >1 = better payout for liked players. The sale-price
    // decision still clamps the effective rate to [0,1], so a friendly bonus never pays above item value.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float HostileSellMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float NeutralSellMultiplier = 1.0f;
    UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.0")) float FriendlySellMultiplier = 1.0f;
};

struct FVendorBuybackEntry {
    TWeakObjectPtr<UMythicItemInstance> Instance;
    int32 PricePaid = 0;
};

UCLASS()
class MYTHIC_API AMythicVendor : public AMythicStorageContainer {
    GENERATED_BODY()

public:
    AMythicVendor();


    // What it costs the player to buy Quantity units of the item in StockSlotIndex (0 if the slot is empty/unpriced).
    // Pass Buyer to fold in their reputation discount/surcharge (null = base price; the UI should pass the local PC so the
    // shown price matches what the server charges).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetBuyPriceForSlot(int32 StockSlotIndex, int32 Quantity, AMythicPlayerController *Buyer = nullptr) const;

    // What this vendor pays the player for Quantity units of Item (0 if worthless/unsellable or no currency def set).
    // Pass Seller to fold in their reputation payout bonus (null = base payout).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetSalePriceForItem(const UMythicItemInstance *Item, int32 Quantity, AMythicPlayerController *Seller = nullptr) const;

    static float ComputeReputationAdjustedMultiplier(float BaseMultiplier, EMythicStandingTier Tier, float HostileMult, float NeutralMult, float FriendlyMult);

    static float EnforceBuyAboveSellRate(float EffBuyMultiplier, float EffSellRate);


    static bool TradeConsumesWholeStack(int32 PlanQuantity, int32 AvailableStacks);

    static bool IsValidBuybackIndex(int32 Index, int32 Num);

    static float ComputeTradingXpReward(float XpPerCoin, int32 Coins, int32 TraderLevel, int32 NoGainAtOrAboveLevel);

    static int32 BuybackRingWriteSlot(int32 TotalPushes, int32 Capacity);

    // Resolve the buyer/seller's standing tier toward this vendor's faction (Neutral when no controller / no standing
    // component / unset VendorFaction — i.e. reputation pricing is a no-op by default). Reads the COND_OwnerOnly standing
    // on the player state, so it's valid on both server (authoritative) and the owning client (display).
    UFUNCTION(BlueprintPure, Category = "Vendor|Reputation")
    EMythicStandingTier ResolvePatronTier(AMythicPlayerController *Patron) const;

    // True when this vendor actually prices on reputation — it has a faction AND at least one tier multiplier differs
    // from 1.0. The trade UI uses this to decide whether showing a standing line would mean anything.
    UFUNCTION(BlueprintPure, Category = "Vendor|Reputation")
    bool HasReputationPricing() const;

    float ResolveEffectiveBuyMultiplier(AMythicPlayerController *Buyer, const UItemDefinition *Def = nullptr) const;
    float ResolveEffectiveSellRate(AMythicPlayerController *Seller, const UItemDefinition *Def = nullptr) const;

    float ResolveScarcityMultiplier(const UItemDefinition *Def) const;

    // True iff this vendor can pay out (a currency definition resolves). UI can hide the Sell tab when false.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    bool CanVendorBuyFromPlayers() const { return ResolveCurrencyItemDefinition() != nullptr; }

    // The currency this vendor pays with: its own override, else the project wallet definition in Mythic settings.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    UItemDefinition *ResolveCurrencyItemDefinition() const;

    // True iff this vendor offers repair (a blacksmith). UI hides the Repair option when false.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    bool CanVendorRepair() const { return bCanRepair; }

    // The currency cost to fully repair Item at this vendor (0 if it has no durability, is already full, or this vendor
    // doesn't repair). For the shop UI to show the price / gray out an unaffordable repair.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetRepairCostForItem(UMythicItemInstance *Item) const;

    FMythicTradePlan Server_ExecuteBuy(AMythicPlayerController *Buyer, int32 StockSlotIndex, int32 Quantity);

    FMythicTradePlan Server_ExecuteSell(AMythicPlayerController *Seller, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex, int32 Quantity);

    FMythicTradePlan Server_ExecuteDelivery(AMythicPlayerController *Deliverer, UMythicInventoryComponent *PlayerInventory,
                                            int32 PlayerSlotIndex, int32 Quantity, const FGameplayTag &ContractItemTag,
                                            EMythicResourceType ReserveAxis);

    // WAVE O: does this vendor redeem trade-contract deliveries? (UI shows the hand-over verb; the contract
    // component pre-validates.)
    UFUNCTION(BlueprintPure, Category = "Vendor|Trading")
    bool AcceptsDeliveries() const { return bAcceptsDeliveries; }

    bool IsContrabandItem(const UItemDefinition *Def) const;

    static int32 ApplyContrabandPremium(int32 Payout, float Premium);

    static float ComputeHagglingBuyMultiplier(int32 TraderLevel, float DiscountPerLevel, float MinMultiplier = 0.5f);
    static float ComputeHagglingSellMultiplier(int32 TraderLevel, float BonusPerLevel, float MaxMultiplier = 1.5f);

    FMythicTradePlan Server_ExecuteRepair(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex);

    FMythicTradePlan Server_ExecuteRepairAll(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory);


    FMythicTradePlan Server_ExecuteBuyback(AMythicPlayerController *Buyer, int32 BuybackIndex);

    // Number of buyback ring slots (some may be empty/spent). UI iterates [0, this) and skips null GetBuybackItem.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetNumBuybackSlots() const { return BuybackEntries.Num(); }

    // The live sold instance in buyback slot Index (null if the slot is empty or its item already left the stock).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    UMythicItemInstance *GetBuybackItem(int32 Index) const;

    // The currency cost to repurchase buyback slot Index (0 if empty/spent) = ceil(price paid × BuybackPriceMultiplier).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetBuybackPrice(int32 Index) const;

    const FMythicFactionId &GetVendorFaction() const { return VendorFaction; }

public:
    /** Name shown at the top of the trade screen ("Village Trader"). Empty falls back to nothing on the UI. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    FText VendorDisplayName;

protected:
    virtual void BeginPlay() override;

    // Price multiplier applied to an item's Value when a player BUYS from this vendor (1.0 = at value, >1.0 = margin).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0"))
    float BuyPriceMultiplier = 1.25f;

    // Fraction of an item's Value this vendor PAYS when a player SELLS to it (0.5 = half value). Clamped [0,1] by the
    // sale-price decision so a vendor never pays above value (no buy-low-sell-high money pump at sane knobs).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SellRate = 0.4f;

    // Optional override of the project wallet definition (UMythicDeveloperSettings::CurrencyItemDefinition) this
    // vendor mints to pay sale proceeds. With neither assigned the vendor only sells goods (player sells are rejected).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    TObjectPtr<UItemDefinition> CurrencyItemDefinition = nullptr;

    // Which faction's standing this vendor prices on (the buyer's reputation toward THIS faction selects the tier
    // multiplier). Unset (default invalid) → reputation pricing is inert (Neutral tier → 1.0). Makes the otherwise
    // debug-only EMythicStandingTier classification matter to the economy.
    //
    // NOT designer-authored: FMythicFactionId is a raw index handed out in registration order at runtime, so a value
    // typed into a Blueprint would silently point at whichever faction happened to register there. Author
    // VendorFactionTag instead; BeginPlay resolves it into this.
    UPROPERTY(BlueprintReadOnly, Category = "Vendor|Reputation")
    FMythicFactionId VendorFaction;

    /**
     * The faction this vendor belongs to, as a stable tag (e.g. "Faction.AzrianRepublic"). Resolved to VendorFaction
     * on BeginPlay through the living-world faction database. Unset (default) leaves VendorFaction invalid, which
     * keeps reputation pricing inert exactly as before — so every existing vendor is unaffected.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Reputation", meta = (Categories = "Faction"))
    FGameplayTag VendorFactionTag;

    // Per-tier price multipliers (all 1.0 by default → no effect). See FVendorReputationPricing.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Reputation")
    FVendorReputationPricing ReputationPricing;


    // Scarcity curve knobs. Elasticity 0 (default) ⇒ the scarcity multiplier is exactly 1.0 (no economy effect). Raise it
    // to let VendorFaction's famines/deficits/surpluses move THIS vendor's buy AND sell prices (applied to both, so the
    // reputation multiplier and the buy>sell money-pump guard are preserved).
    // WAVE O (O6) CONTENT NOTE — trade-goods vendor preset: provisioners/caravanserai vendors that exist to be
    // arbitraged should author WIDENED bands (MinBand 0.5 / MaxBand 2.0, Elasticity ~0.15) so cross-settlement price
    // spreads are worth hauling; ordinary vendors keep the tight defaults (0.75/1.5).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Economy")
    FMythicEconomyPricingParams EconomyParams;

    // Maps an item's type tag (UItemDefinition::ItemType) to an economy axis. EMPTY (default) ⇒ every item is axis None ⇒
    // scarcity pricing is inert. Keys may be specific tags (exact match) or parent tags (hierarchical match), e.g.
    // "Itemization.Type.Consumable.Food" → Food, "Itemization.Type.Mining" → Materials, "Itemization.Type.Equipment.Weapon"
    // → Arms, "Itemization.Type.Currency" → Wealth.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Economy")
    TMap<FGameplayTag, EMythicEconomyAxis> ItemAxisMap;

    // Local sell-pressure: how much each unit a player DUMPS into this vendor decays that item type's buy-back rate toward
    // SellPressureFloorMult. 0 (default) ⇒ the feature is off (no accumulator effect, prices as before). A small value
    // (e.g. 0.02) makes flooding a vendor with one commodity crash its local buy price for that commodity.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Economy", meta = (ClampMin = "0.0"))
    float SellPressurePerUnit = 0.0f;

    // The absolute sell-rate floor local dumping decays toward (only used when SellPressurePerUnit > 0). Clamped [0,1].
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Economy", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SellPressureFloorMult = 0.1f;

    // Half-life (seconds) of accumulated local sell pressure — how fast a flooded commodity's buy-back rate recovers once
    // players stop dumping. Timestamp-decayed on read/record (NO Tick). Only used when SellPressurePerUnit > 0.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Economy", meta = (ClampMin = "1.0"))
    float SellPressureHalfLifeSeconds = 120.0f;

    // When true (default) items a player sells are added to this vendor's stock so it can resell them; when the stock
    // can't accept them they are consumed and the player is still paid. Pure policy knob.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    bool bAbsorbSoldItems = true;

    // When true this vendor is a blacksmith that repairs durable items for currency (default false — a general goods
    // vendor only trades). Repair uses CurrencyItemDefinition's wallet model (spends the player's currency).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    bool bCanRepair = false;

    // Fraction of an item's Value charged to repair it from fully-broken to full (scaled down by how much durability is
    // actually missing). 0.5 = a fully-broken item costs half its value to mend. Clamped non-negative by the cost decision.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0"))
    float RepairCostFraction = 0.5f;

    // Buy back at this multiple of what the vendor paid the seller (1.0 = exactly the sale price; a small markup > 1 is a
    // convenience fee that still stays far below the full buy price — so sell→buyback can never money-pump).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "1.0"))
    float BuybackPriceMultiplier = 1.0f;

    // How many recently-sold instances this vendor keeps repurchasable (the buyback ring capacity). Oldest is evicted
    // once full.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "1"))
    int32 BuybackCapacity = 12;


    // The proficiency track that buying/selling at this vendor trains. Null ⇒ trading grants no XP (the default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Proficiency")
    TObjectPtr<UProficiencyDefinition> TradingProficiency = nullptr;

    // Trading XP granted per coin of a completed transaction (buy total / sell payout). <= 0 ⇒ no XP (the default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Proficiency", meta = (ClampMin = "0.0"))
    float TradingXpPerCoin = 0.0f;

    // Anti-grind: once the trader's Trading level reaches/passes this, transactions stop paying XP. 0 ⇒ no cap (default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Proficiency", meta = (ClampMin = "0"))
    int32 TradingXpNoGainAtOrAboveLevel = 0;


    // Buy-price discount per Trading level (0.005 = −0.5%/level, floored at 50% of base by the pure fold).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Proficiency", meta = (ClampMin = "0.0"))
    float HagglingBuyDiscountPerLevel = 0.0f;

    // Sell-payout bonus per Trading level (0.005 = +0.5%/level, ceilinged at 150% by the pure fold; the sale-price
    // decision still clamps the effective rate to [0,1] — haggling never pays above item value).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Proficiency", meta = (ClampMin = "0.0"))
    float HagglingSellBonusPerLevel = 0.0f;

    // --- WAVE O (O2): DELIVERIES — default false ⇒ this vendor never redeems contracts (inert). Flag the faction's
    //     provisioners/quartermasters (CONTENT). Reserve injection additionally needs VendorFaction set. ---
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Trading")
    bool bAcceptsDeliveries = false;


    // Item-type tags this vendor treats as contraband (hierarchical match against UItemDefinition::ItemType).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Smuggling")
    FGameplayTagContainer BannedItemTags;

    // False (default): contraband is REFUSED (NotSellable). True: this is a black-market fence — contraband sells
    // here at ContrabandPremium × payout, and the sale rides the crime-witness pipeline (real perceivers only).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Smuggling")
    bool bBlackMarket = false;

    // Black-market payout multiplier on contraband sales (≥ 1; only read when bBlackMarket).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Smuggling", meta = (ClampMin = "1.0"))
    float ContrabandPremium = 1.25f;

private:
    void GrantTradingXp(AMythicPlayerController *Trader, int32 Coins);

    int32 ResolveTraderLevel(AMythicPlayerController *Trader) const;

    void RecordContrabandSale(AMythicPlayerController *Seller, int32 Units);

    TArray<FVendorBuybackEntry> BuybackEntries;
    int32 BuybackPushCount = 0;

    void RecordBuyback(UMythicItemInstance *Instance, int32 PricePaid);

    void RemoveFromBuyback(const UMythicItemInstance *Instance);

    int32 ComputeBuybackPrice(int32 PricePaid) const;

    bool IsBuybackEntryLive(const FVendorBuybackEntry &Entry) const;


    struct FSellPressureState {
        float Units = 0.0f;
        double LastUpdateSeconds = 0.0;
    };
    TMap<FGameplayTag, FSellPressureState> SellPressureByType;

    float GetDecayedSellPressureUnits(const FGameplayTag &TypeTag) const;

    void RecordSellPressure(const FGameplayTag &TypeTag, int32 Units);
};
