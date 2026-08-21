// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "CommonPlayerController.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "AI/NPCs/MythicSocialVerbs.h"
#include "AI/Party/MythicPartyTypes.h"
#include "UI/HUD/MythicHudNotice.h"
#include "MythicPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicHudNoticeRaised, const FMythicHudNotice &, Notice);

struct FTrackedDestructibleData;
class UMythicItemInstance;
class UItemDefinition;
class AMythicConversionStation;
class AMythicStorageContainer;
class AMythicVendor;
class AMythicPlayerStall;
enum class EMythicTradeResult : uint8;
class AMythicNPCCharacter;

class UMythicCheatManager;

USTRUCT(BlueprintType)
struct FPlayerStatsSummary {
    GENERATED_BODY()

    // power stat for offense
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float Power = 0.0f;

    // base damage per hit
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float DamagePerHit = 0.0f;

    // attack speed multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float AttackSpeed = 0.0f;

    // critical hit chance fraction
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float CritChance = 0.0f;

    // critical hit damage multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float CritDamage = 0.0f;

    // armor stat for damage reduction
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float Armor = 0.0f;

    // dodge chance fraction
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float DodgeChance = 0.0f;

    // maximum energy shield
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MaxShield = 0.0f;

    // regeneration rate of energy shield per second
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float ShieldRegenRate = 0.0f;

    // regeneration rate of health per second
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float HealthRegenRate = 0.0f;

    // maximum health pool
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MaxHealth = 0.0f;

    // maximum stamina pool
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MaxStamina = 0.0f;

    // regeneration rate of stamina per second
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float StaminaRegenRate = 0.0f;

    // cooldown reduction fraction
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float CooldownReduction = 0.0f;

    // proficiency experience bonus multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float ProficiencyXPBonus = 0.0f;

    // bonus speed multiplier while sprinting
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float BonusSprintSpeed = 0.0f;

    // canonical player level derived from proficiencies
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    int32 PlayerLevel = 0;
};

USTRUCT(BlueprintType)
struct FMythicPendingDeploy {
    GENERATED_BODY()

    // inventory component holding the placeable item
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    TWeakObjectPtr<UMythicInventoryComponent> Inventory;

    // item instance of the placeable
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    TWeakObjectPtr<UMythicItemInstance> Item;

    // slot index in the inventory
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    int32 SlotIndex = INDEX_NONE;

    // world transform to spawn the placeable at
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    FTransform SpawnTransform;
};

UCLASS(Config=Game)
class AMythicPlayerController : public ACommonPlayerController, public IAbilitySystemInterface, public IInventoryProviderInterface {
    GENERATED_BODY()

protected:
    // Proficiency Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proficiency")
    class UProficiencyComponent *ProficiencyComponent;

    // Inventory Components
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    class UMythicInventoryComponent *InventoryComponent;

    // Per-player quest/objective tracker (subscribes to GAS kill events, grants rewards on completion).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objectives")
    class UObjectiveTracker *ObjectiveTracker;

    // Per-player environmental hazard component (applies weather/season/time GameplayEffects to the player).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
    class UMythicEnvironmentHazardComponent *EnvironmentHazard;

    // Per-player World Chronicle relay (replicates the server-built world-news feed to the owning client).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Chronicle")
    class UMythicChronicleRelayComponent *ChronicleRelay;

public:
    AMythicPlayerController();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const override;

    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnUnPossess() override;
    virtual void OnRep_PlayerState() override;

    // Client-side event when the player is possessed
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic")
    void OnPossessedOnClient();

protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

    void Login(int32 LocalUserNum);

    void CB_LoginResponse(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error);

    FDelegateHandle LoginDelegateHandle;

public:
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    class UProficiencyComponent *GetProficiencyComponent() const;

    // The player's primary inventory component (used by the storage move RPC's identity check + UI binding).
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UMythicInventoryComponent *GetInventoryComponent() const { return InventoryComponent; }

    // The player's objective/quest tracker.
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    class UObjectiveTracker *GetObjectiveTracker() const { return ObjectiveTracker; }

    // returns the canonical player level derived from proficiency progress
    UFUNCTION(BlueprintPure, Category = "Progression")
    int32 GetPlayerLevel() const;

    // returns the progress fraction toward the next player level
    UFUNCTION(BlueprintPure, Category = "Progression")
    float GetPlayerLevelProgress() const;

    // returns summaries of all proficiencies for UI consumption
    UFUNCTION(BlueprintCallable, Category = "Progression")
    TArray<FProficiencySummary> GetProficiencySummaries() const;

    // aggregates combat-relevant attributes into a single struct
    UFUNCTION(BlueprintCallable, Category = "Progression")
    FPlayerStatsSummary GetPlayerStats() const;

    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerOpenConversionStation(AMythicConversionStation *Station);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerConversionRequestStart(AMythicConversionStation *Station, FGameplayTag RecipeId, int32 Quantity);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerConversionCancelJob(AMythicConversionStation *Station, int32 JobId);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerConversionSetAutoRepeat(AMythicConversionStation *Station, bool bRepeat);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Storage")
    void ServerMoveItemBetweenInventories(UMythicInventoryComponent *Source, int32 SourceSlot, UMythicInventoryComponent *Target, int32 TargetSlot);

    /**
     * Total currency this player carries, summed across every inventory they own. This is the wallet balance a trade
     * or HUD widget shows. BlueprintPure because the UI needs it every frame it draws a price, and there was
     * previously no Blueprint-reachable way to ask "how much gold do I have".
     */
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetCarriedCurrency() const;

    // ---- Vendor RPCs (client-owned PC -> server-authoritative currency-gated trade) ----
    // Buy Quantity units of the vendor's StockSlotIndex into the player's inventory; charges the player's currency.
    // Authorized exactly like a container access (the player must have THIS vendor open + be in range).
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorBuy(AMythicVendor *Vendor, int32 StockSlotIndex, int32 Quantity);

    // Sell Quantity units of PlayerSlotIndex (in one of the player's OWN inventories) to the vendor; pays proceeds.
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorSell(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex, int32 Quantity);

    // Buy Quantity units of StallSlotIndex from a TEAMMATE's player stall at its listed price. Coins move from the
    // buyer into the stall owner's till (never minted). Authorized exactly like a vendor buy: the stall's inventory
    // must be open + in range via CanPlayerAccessInventory (AMythicPlayerStall derives from AMythicStorageContainer).
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Stall")
    void ServerStallBuy(AMythicPlayerStall *Stall, int32 StallSlotIndex, int32 Quantity);

    void RecordVendorAcquaintance(const AMythicVendor *Vendor, const struct FMythicTradePlan &Plan);

    // Repair the durable item in PlayerSlotIndex (one of the player's OWN inventories) at the vendor (blacksmith) for
    // currency. Authorized like sell (vendor open + in range, source is the player's own).
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorRepair(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex);

    // Repair ALL damaged items in one of the player's OWN inventories at the vendor (a "Repair All" convenience), charging
    // cheapest-first within the player's budget. Same authorization as single repair (vendor open + in range, own inventory).
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorRepairAll(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory);

    // Buy back a recently-sold instance from the vendor's buyback ring (indexed by BuybackIndex) at the buyback price —
    // the price it was sold for × the vendor's small markup. Same authorization as buy (vendor open + in range). The
    // vendor hands back the SAME instance (rolled affixes / item level / durability intact, never a re-mint); on a hard
    // reject (unaffordable / already gone / no room) the standard trade-result callout is surfaced.
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerBuyback(AMythicVendor *Vendor, int32 BuybackIndex);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Itemization")
    void ServerSetItemJunk(UMythicItemInstance *Item, bool bJunk);

    // Sell EVERY junk-flagged (manual OR auto), sellable item across the player's own inventories to Vendor, reusing the
    // per-item vendor sell path (AMythicVendor::Server_ExecuteSell) — so it's exactly-once with no dupe/loss, and the
    // vendor re-validates each item. Server-authoritative; the vendor must be open + in range (same gate as a manual
    // sell). Bounded: a single forward pass over the slots (equipment/currency/non-takeable slots are skipped by the
    // junk predicate). A successful sale already floats the "+N <currency>" callout per item, so no extra feedback here.
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerSellAllJunk(AMythicVendor *Vendor);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Gift")
    void ServerOfferGift(AMythicPlayerController *Recipient, UMythicInventoryComponent *SourceInv, int32 SourceSlotIndex, int32 Quantity = 0);

    UFUNCTION(Client, Reliable, Category = "Gift")
    void ClientReceiveGiftOffer(AMythicPlayerController *Giver, const FText &ItemName);

    UFUNCTION(Server, Reliable, Category = "Gift")
    void ServerRespondGift(bool bAccept);

    // RECIPIENT client BP hook: a gift was offered. The gift-prompt widget implements this to show Accept/Decline and call
    // ServerRespondGift. (ClientReceiveGiftOffer also floats a beat so the offer isn't silent before the widget is wired.)
    UFUNCTION(BlueprintImplementableEvent, Category = "Gift")
    void OnGiftOffered(AMythicPlayerController *Giver, const FText &ItemName);

    UFUNCTION(Client, Reliable, Category = "Gift")
    void ClientNotifyGiftResult(const FText &Message, FLinearColor Color);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Placeable")
    void ServerDeployPlaceable(UMythicInventoryComponent *Inventory, int32 SlotIndex, FVector AimOrigin, FVector AimDirection);

    UFUNCTION(Client, Reliable, Category = "Placeable")
    void ClientNotifyDeployRejected(const FText &Reason);

    // client-side display hook for ClientNotifyDeployRejected (BP shows the toast / plays the deny sound).
    UFUNCTION(BlueprintImplementableEvent, Category = "Placeable")
    void OnDeployRejected(const FText &Reason);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Interaction")
    void ServerInteractPrimary(AActor *Interactable);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Dialogue")
    void ServerRequestNpcDialogue(AMythicNPCCharacter *NPC);

    UFUNCTION(Client, Reliable, Category = "Dialogue")
    void ClientReceiveNpcDialogue(AMythicNPCCharacter *NPC, const FText &Line);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Social")
    void ServerPerformSocialVerb(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb);

    UFUNCTION(Client, Reliable, Category = "Social")
    void ClientReceiveSocialReaction(AMythicNPCCharacter *NPC, EMythicSocialVerb Verb, EMythicSocialReaction Reaction, const FText &Line);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Party")
    void ServerRecruitNpc(AMythicNPCCharacter *NPC);

    UFUNCTION(Client, Reliable, Category = "Party")
    void ClientReceiveRecruitResult(AMythicNPCCharacter *NPC, bool bSucceeded);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Party")
    void ServerIssueCompanionOrder(AMythicNPCCharacter *Companion, EMythicCompanionOrder Order, AActor *OrderTarget);

    void OfferNpcQuestIfAny(AMythicNPCCharacter *NPC);

    UFUNCTION(Client, Unreliable, Category = "Gathering")
    void ClientShowGatherProgress(FVector Location, int32 HitsRemaining);

    UFUNCTION(Client, Reliable, Category = "Gathering")
    void ClientShowGatherDepleted(FVector Location);

    UFUNCTION(Client, Reliable, Category = "Combat")
    void ClientShowShieldAbsorbed(int32 Absorbed, bool bBroke);

    UFUNCTION(Client, Reliable, Category = "Combat")
    void ClientShowDodge();

    UFUNCTION(Client, Reliable, Category = "Combat")
    void ClientNotifyExhausted(bool bExhausted);

    UFUNCTION(Client, Reliable, Category = "Progression")
    void ClientNotifyProficiencyLevel(const FText &ProfName, int32 NewLevel, const FText &MilestoneName);

    UFUNCTION(Client, Reliable, Category = "Objectives")
    void ClientNotifyObjective(const FText &DisplayText, int32 Current, int32 Required, bool bCompleted, int32 StackIndex,
                               const FText &QuestTitle);

    UFUNCTION(Client, Reliable, Category = "Objectives")
    void ClientNotifyObjectiveResult(const FText &DisplayText, EObjectiveNotifyCategory Category, EObjectiveOfferResult OfferResult, int32 Current, int32 Required, bool bRewardSucceeded, bool bRewardDroppedNearby, int32 StackIndex);

    UFUNCTION(Client, Reliable, Category = "Itemization")
    void ClientNotifyLootPickup(const FText &ItemName, int32 Quantity, FLinearColor RarityColor);

    UFUNCTION(Client, Reliable, Category = "Itemization")
    void ClientNotifyRewardCelebration(UItemDefinition *ItemDef, int32 Quantity);

    // client-side display hook for ClientNotifyRewardCelebration (BP plays the reward fanfare banner/particles/sound).
    UFUNCTION(BlueprintImplementableEvent, Category = "Itemization")
    void OnRewardCelebration(UItemDefinition *ItemDef, int32 Quantity);

    // ---- HUD notices ----
    /**
     * The single stream every HUD callout arrives on. The feed, banner and objective tracker each subscribe and take
     * the kinds they present, so adding an event to the HUD means raising a notice — not building another widget.
     */
    UPROPERTY(BlueprintAssignable, Category = "Mythic|HUD")
    FMythicHudNoticeRaised OnHudNotice;

    /** Raise a HUD notice on this (owning) client. Safe to call from anywhere client-side. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|HUD")
    void RaiseHudNotice(const FMythicHudNotice &Notice);

    // ---- Open container / vendor ----
    /**
     * The container or vendor this player most recently opened, client-side. Set on the interact that opens it, so a
     * trade or storage screen can bind to the right actor without the Blueprint having to hand it over.
     */
    UPROPERTY(BlueprintReadOnly, Transient, Category = "Mythic|Interaction")
    TWeakObjectPtr<AActor> ActiveContainer;

    UFUNCTION(Client, Reliable, Category = "Itemization")
    void ClientNotifyTradeResult(EMythicTradeResult Result);

    UFUNCTION(Client, Reliable, Category = "Itemization")
    void ClientNotifyItemDurability(const FText &ItemName, EMythicItemDurabilityBeat Beat);

    void NotifyItemAcquired(const UItemDefinition *ItemDef, int32 Quantity);

    void NotifyItemUsed(const UItemDefinition *ItemDef, int32 Quantity);

    void NotifyItemEquipped(const UItemDefinition *ItemDef);

    void NotifyTalkedToNPC(const FGameplayTag &NpcTag);

    UFUNCTION(Client, Reliable, Category = "Party")
    void ClientNotifyCompanionDeparted(const FText &Name, FVector Location);

    UFUNCTION(Client, Reliable, Category = "Party")
    void ClientNotifyCompanionBetrayed(const FText &Name, FVector Location);

    UFUNCTION(Client, Reliable, Category = "Environment")
    void ClientNotifyEnvironmentHazard(const FText &HazardName, bool bOnset);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Trade")
    void ServerExecuteBarterOffer(AMythicNPCCharacter *NPC, int32 OfferIndex);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Forge")
    void ServerRerollItemAffixes(UMythicItemInstance *Item);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Forge")
    void ServerSetItemAffixLocked(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked);

    static bool IsWithinStationRange(float DistSq, float RangeSq);

    UFUNCTION(Client, Reliable, Category = "Zone")
    void ClientNotifyZoneEntry(const FText &SettlementName);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Fast Travel")
    void ServerFastTravel(int32 SettlementId);

    static bool CanFastTravel(const TSet<int32> &Discovered, int32 SettlementId, bool bBlocked);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Fast Travel")
    void ServerFastTravelToPOI(int32 POIId);

    UFUNCTION(Client, Reliable, Category = "Fast Travel")
    void ClientNotifyFastTravelRefused(const FText &Reason);

    // True when this player is carrying enough to be denied fast travel. Shared by BOTH travel paths so a hauler can't
    // dodge the walk home by picking a landmark instead of a settlement. Always false when encumbrance is disabled.
    UFUNCTION(BlueprintPure, Category = "Fast Travel")
    bool IsOverloadedForFastTravel() const;

private:
    bool CanPlayerAccessInventory(UMythicInventoryComponent *Inventory) const;

    TWeakObjectPtr<AMythicPlayerController> PendingGiftGiver;
    TWeakObjectPtr<UMythicInventoryComponent> PendingGiftSourceInv;
    TWeakObjectPtr<UMythicItemInstance> PendingGiftItem;
    int32 PendingGiftSourceSlot = INDEX_NONE;
    int32 PendingGiftQuantity = 0;
    FTimerHandle PendingGiftTimerHandle;
    bool HasPendingGift() const { return PendingGiftGiver.IsValid() && PendingGiftItem.IsValid(); }
    void ClearPendingGift();
    void OnPendingGiftExpired();
    bool IsWithinGiftRange(const AMythicPlayerController *Other) const;

    void HandleDeployClassLoaded(FSoftObjectPath ClassPath, FMythicPendingDeploy Pending);

    void FinishDeployPlaceable(UClass *DeployedClass, const FMythicPendingDeploy &Pending);

    static bool CanDeployMore(int32 CurrentValidCount, int32 MaxAllowed);

    // Per-player cap on simultaneously-deployed placeables. 0 (default) = unlimited (byte-identical to the prior
    // behaviour); set > 0 to cap base-building spam / structure count. Enforced server-side in FinishDeployPlaceable.
    UPROPERTY(EditDefaultsOnly, Category = "Placeable")
    int32 MaxDeployedPlaceables = 0;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> DeployedPlaceables;

    void CheckZoneEntry();

    FTimerHandle ZoneCheckTimerHandle;
    int32 LastSettlementId = INDEX_NONE;

    uint8 LastTerritoryFactionIndex = 0xFF;

    TSet<int32> DiscoveredSettlements;

    // How often (seconds) the server re-checks which settlement the player occupies. Designer-tunable, not a magic literal.
    UPROPERTY(EditAnywhere, Category = "Zone", meta = (ClampMin = "0.1"))
    float ZoneCheckInterval = 1.0f;
};
