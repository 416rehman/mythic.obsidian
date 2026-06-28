// Mythic — merchant vendor (the Acquire→vendors verb).
// A vendor IS-A storage container (interaction, opener registry, range gate, replicated stock inventory, save) PLUS a
// server-authoritative currency-gated buy/sell. Pricing is data-driven: items are priced from UItemDefinition::Value,
// marked up by BuyPriceMultiplier on buy and discounted by SellRate on sell; sale proceeds are paid in
// CurrencyItemDefinition. Standard ARPG vendor: infinite vendor funds, finite stock, fungible def-based goods
// (per-instance affix/durability preservation on resale is a logged follow-up). The transaction is INITIATED through
// AMythicPlayerController::ServerVendorBuy/Sell (the client owns its PC); this class executes it server-side once the
// PC has authorized access (opener + range, reusing CanPlayerAccessInventory on the stock inventory).

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/MythicTrade.h" // EMythicTradeResult + FMythicTradePlan (buy/sell outcome)
#include "Itemization/Storage/MythicStorageContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h" // FMythicFactionId (the vendor's faction, for reputation pricing)
#include "MythicVendor.generated.h"

class UItemDefinition;
class UMythicItemInstance;
class UMythicInventoryComponent;
class AMythicPlayerController;
enum class EMythicStandingTier : uint8; // resolved from the buyer's standing toward VendorFaction

// Per-standing-tier price multipliers for a vendor. Each scales the vendor's base BuyPriceMultiplier (buy) or SellRate
// (sell) by the buyer's reputation tier toward the vendor's faction. 1.0 = no effect (the default → reputation pricing
// is opt-in: a default vendor with an unset VendorFaction or all-1.0 mults prices identically to before). Friendly < 1
// on buy = a loyalty discount; Hostile > 1 = a markup (or refusal at a high enough value).
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

UCLASS()
class MYTHIC_API AMythicVendor : public AMythicStorageContainer {
    GENERATED_BODY()

public:
    AMythicVendor();

    // --- Pricing accessors for the vendor UI (BlueprintPure): predict the price client-side so the shop can gray out
    //     unaffordable buys / show the payout. The server independently re-validates on the actual transaction. ---

    // What it costs the player to buy Quantity units of the item in StockSlotIndex (0 if the slot is empty/unpriced).
    // Pass Buyer to fold in their reputation discount/surcharge (null = base price; the UI should pass the local PC so the
    // shown price matches what the server charges).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetBuyPriceForSlot(int32 StockSlotIndex, int32 Quantity, AMythicPlayerController *Buyer = nullptr) const;

    // What this vendor pays the player for Quantity units of Item (0 if worthless/unsellable or no currency def set).
    // Pass Seller to fold in their reputation payout bonus (null = base payout).
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetSalePriceForItem(const UMythicItemInstance *Item, int32 Quantity, AMythicPlayerController *Seller = nullptr) const;

    // Pure: scale BaseMultiplier by the per-tier multiplier selected for Tier (clamped non-negative). Hostile/Neutral/
    // Friendly map to the three args; any other value falls back to NeutralMult. Static + unit-testable.
    static float ComputeReputationAdjustedMultiplier(float BaseMultiplier, EMythicStandingTier Tier, float HostileMult, float NeutralMult, float FriendlyMult);

    // Pure: the effective buy multiplier floored so the buy price can never fall to/below the SAME-tier sell payout (else
    // a liked player could money-pump by buying then reselling to the same vendor). EffSellRate is clamped [0,1] to match
    // ComputeSalePrice. Floors EffBuyMultiplier at that clamped rate → buy rate ≥ sell rate → buy price ≥ sell payout.
    // Static + unit-testable.
    static float EnforceBuyAboveSellRate(float EffBuyMultiplier, float EffSellRate);

    // Resolve the buyer/seller's standing tier toward this vendor's faction (Neutral when no controller / no standing
    // component / unset VendorFaction — i.e. reputation pricing is a no-op by default). Reads the COND_OwnerOnly standing
    // on the player state, so it's valid on both server (authoritative) and the owning client (display).
    EMythicStandingTier ResolvePatronTier(AMythicPlayerController *Patron) const;

    // The single source of truth for reputation-adjusted rates (so the buy>sell guard can't be forgotten at a call site):
    // resolve the patron's tier once, scale the base buy markup / sell rate, and (buy only) floor the buy multiplier above
    // the same-tier sell rate. Used by both the display accessors and the authoritative Server_Execute paths.
    float ResolveEffectiveBuyMultiplier(AMythicPlayerController *Buyer) const;
    float ResolveEffectiveSellRate(AMythicPlayerController *Seller) const;

    // True iff this vendor can pay out (has a currency definition assigned). UI can hide the Sell tab when false.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    bool CanVendorBuyFromPlayers() const { return CurrencyItemDefinition != nullptr; }

    // True iff this vendor offers repair (a blacksmith). UI hides the Repair option when false.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    bool CanVendorRepair() const { return bCanRepair; }

    // The currency cost to fully repair Item at this vendor (0 if it has no durability, is already full, or this vendor
    // doesn't repair). For the shop UI to show the price / gray out an unaffordable repair.
    UFUNCTION(BlueprintPure, Category = "Vendor")
    int32 GetRepairCostForItem(UMythicItemInstance *Item) const;

    // SERVER: buy Quantity units of StockSlotIndex for Buyer. The pure MythicTrade::PlanBuy decides the outcome + quantity
    // (clamped to stock, affordability, and the buyer's free space); this charges the currency and delivers the goods.
    // Returns the full trade plan (result reason + units traded + coins) so the caller can fire the player callout.
    // Authority + access are pre-checked by the calling PC RPC.
    FMythicTradePlan Server_ExecuteBuy(AMythicPlayerController *Buyer, int32 StockSlotIndex, int32 Quantity);

    // SERVER: sell Quantity units from Seller's PlayerSlotIndex in PlayerInventory to this vendor. MythicTrade::PlanSell
    // decides the outcome; this removes the goods (absorbed into stock when bAbsorbSoldItems + space, else consumed) and
    // pays proceeds in CurrencyItemDefinition. Returns the trade plan.
    FMythicTradePlan Server_ExecuteSell(AMythicPlayerController *Seller, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex, int32 Quantity);

    // SERVER: repair the item in Payer's PlayerSlotIndex for currency. MythicTrade::PlanRepair decides the outcome
    // (NothingToRepair when already full / no durability, InsufficientFunds when too poor); this charges the cost and
    // restores durability via the item's UDurabilityFragment::ServerRepair. Returns the trade plan. Requires bCanRepair.
    FMythicTradePlan Server_ExecuteRepair(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex);

    // SERVER: repair ALL damaged repairable items in PlayerInventory, cheapest-first within the player's budget
    // (MythicTrade::ComputeRepairAllPlan). Charges the total once + restores each. Returns a plan: Quantity = items
    // repaired, TotalPrice = total charged, Result = Success / NothingToRepair / InsufficientFunds. Requires bCanRepair.
    FMythicTradePlan Server_ExecuteRepairAll(AMythicPlayerController *Payer, UMythicInventoryComponent *PlayerInventory);

protected:
    // Price multiplier applied to an item's Value when a player BUYS from this vendor (1.0 = at value, >1.0 = margin).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0"))
    float BuyPriceMultiplier = 1.25f;

    // Fraction of an item's Value this vendor PAYS when a player SELLS to it (0.5 = half value). Clamped [0,1] by the
    // sale-price decision so a vendor never pays above value (no buy-low-sell-high money pump at sane knobs).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SellRate = 0.4f;

    // The currency item this vendor mints to pay sale proceeds (the project "gold" definition). MUST be assigned for
    // selling; when null this vendor only sells goods (player sells are rejected — honest, never a fake payout).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor")
    TObjectPtr<UItemDefinition> CurrencyItemDefinition = nullptr;

    // Which faction's standing this vendor prices on (the buyer's reputation toward THIS faction selects the tier
    // multiplier). Unset (default invalid) → reputation pricing is inert (Neutral tier → 1.0). Makes the otherwise
    // debug-only EMythicStandingTier classification matter to the economy.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Reputation")
    FMythicFactionId VendorFaction;

    // Per-tier price multipliers (all 1.0 by default → no effect). See FVendorReputationPricing.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Vendor|Reputation")
    FVendorReputationPricing ReputationPricing;

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
};
