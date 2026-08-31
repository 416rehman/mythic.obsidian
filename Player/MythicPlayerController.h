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
#include "GAS/Combat/MythicCombatPresentationProjection.h"
#include "GAS/Feedback/MythicCombatTextTypes.h"
#include "UI/HUD/MythicHudNotice.h"
#include "World/Harvesting/MythicHarvestTypes.h"
#include "World/Entity/MythicEntityPresentationTypes.h"
#include "MythicPlayerController.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FMythicHudNoticeRaised, const FMythicHudNotice &, Notice);

class UMythicItemInstance;
class UMythicHarvestFocusComponent;
class UMythicContextActionDefinition;
class UMythicContextActionProjectionPolicy;
class UMythicEntityActionGrantComponent;
class UMythicEntityAttentionSubsystem;
class UMythicEntityCombatPresentationComponent;
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

    /** Offensive power used by the player's damage calculations. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float Power = 0.0f;

    /** Base damage dealt by one ordinary hit. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float DamagePerHit = 0.0f;

    /** Attack-speed bonus fraction over normal cadence; zero means normal speed. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float AttackSpeed = 0.0f;

    /** Critical-hit chance expressed as a fraction. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float CritChance = 0.0f;

    /** Critical-hit damage bonus fraction before configured diminishing returns. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float CritDamage = 0.0f;

    /** Armor used to mitigate incoming damage. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float Armor = 0.0f;

    /** Dodge chance expressed as a fraction. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float DodgeChance = 0.0f;

    /** Maximum energy-shield capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MaxShield = 0.0f;

    /** Energy shield restored per second. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float ShieldRegenRate = 0.0f;

    /** Health restored per second. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float HealthRegenRate = 0.0f;

    /** Maximum health capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MaxHealth = 0.0f;

    /** Maximum stamina capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MaxStamina = 0.0f;

    /** Stamina restored per second. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float StaminaRegenRate = 0.0f;

    /** Cooldown reduction expressed as a fraction. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float CooldownReduction = 0.0f;

    /** Additive proficiency-XP bonus fraction; zero grants no bonus. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float ProficiencyXPBonus = 0.0f;

    /** Movement-speed multiplier, where 1.0 is the base speed. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    float MovementSpeedMultiplier = 1.0f;

    /** Canonical player level derived from proficiency progression. */
    UPROPERTY(BlueprintReadOnly, Category = "Player Stats")
    int32 PlayerLevel = 0;
};

USTRUCT(BlueprintType)
struct FMythicPendingDeploy {
    GENERATED_BODY()

    /** Inventory that currently holds the placeable item. */
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    TWeakObjectPtr<UMythicInventoryComponent> Inventory;

    /** Item instance being deployed. */
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    TWeakObjectPtr<UMythicItemInstance> Item;

    /** Slot containing the placeable item. */
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    int32 SlotIndex = INDEX_NONE;

    /** Validated world transform for the deployed actor. */
    UPROPERTY(BlueprintReadOnly, Category = "Placeable")
    FTransform SpawnTransform;
};

UCLASS(Config=Game)
class AMythicPlayerController : public ACommonPlayerController, public IAbilitySystemInterface, public IInventoryProviderInterface {
    GENERATED_BODY()

protected:
    /** Authoritative component that owns player and skill proficiency progression. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Proficiency")
    class UProficiencyComponent *ProficiencyComponent;

    /** Primary inventory used for carried items and player-facing inventory screens. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Inventory")
    class UMythicInventoryComponent *InventoryComponent;

    /** Per-player quest tracker that consumes objective events and grants completion rewards. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Objectives")
    class UObjectiveTracker *ObjectiveTracker;

    /** Applies authoritative weather, season, and time hazards to this player. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Environment")
    class UMythicEnvironmentHazardComponent *EnvironmentHazard;

    /** Replicates the server-built World Chronicle feed to this owning client. */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "World Chronicle")
    class UMythicChronicleRelayComponent *ChronicleRelay;

    /**
     * Controller-owned nonreplicated focus service used only by the owning client; Blueprint may render its immutable
     * DTO/delegate, it never authorizes or mutates harvesting, missing local input/assets fail closed, and its sweep
     * distances/intervals use the centimeter/second units documented by Mythic Harvest Settings.
     */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Harvest|Focus")
    TObjectPtr<UMythicHarvestFocusComponent> HarvestFocusComponent;

    /**
     * Server security, cadence, lease, and query-budget policy for owner-only focused-subject action offers. A missing
     * or invalid policy fails closed; game-mode controller classes should reference one cooked policy directly.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Presentation|Context Actions")
    TObjectPtr<UMythicContextActionProjectionPolicy> ContextActionProjectionPolicy;

    /**
     * Server range, aim, line-of-sight, cadence, lease, and disclosure rules for the owning viewer's focused combat
     * assessment. Raw client stats are never accepted; authority samples only the exact registry-resolved subject.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World Presentation|Combat")
    FMythicCombatPresentationProjectionPolicy CombatPresentationProjectionPolicy;

public:
    AMythicPlayerController();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    /** Returns every inventory currently controlled by this player in deterministic traversal order. */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual TArray<UMythicInventoryComponent *> GetAllInventoryComponents() const override;

    /** Returns the ability-system component that owns this player's learned schematic state. */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual UAbilitySystemComponent *GetSchematicsASC() const override;

    /** Resolves the canonical inventory responsible for carrying the supplied item type. */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    virtual UMythicInventoryComponent *GetInventoryForItemType(const FGameplayTag &ItemType) const override;

    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnUnPossess() override;
    virtual void OnRep_PlayerState() override;
    virtual void SeamlessTravelTo(APlayerController *NewPC) override;
    virtual void PreClientTravel(const FString &PendingURL,
                                 ETravelType TravelType,
                                 bool bIsSeamlessTravel) override;

    /** Blueprint event raised on the owning client after this controller possesses its player pawn. */
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
    /** Returns the authoritative proficiency component used for player-level and skill progression. */
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    class UProficiencyComponent *GetProficiencyComponent() const;

    /** Returns the player's primary inventory used for identity checks and UI binding. */
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UMythicInventoryComponent *GetInventoryComponent() const { return InventoryComponent; }

    /** Returns the player's objective and quest tracker. */
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    class UObjectiveTracker *GetObjectiveTracker() const { return ObjectiveTracker; }

    /** Returns the canonical player level derived from proficiency progression. */
    UFUNCTION(BlueprintPure, Category = "Progression")
    int32 GetPlayerLevel() const;

    /** Returns normalized progress toward the next player level. */
    UFUNCTION(BlueprintPure, Category = "Progression")
    float GetPlayerLevelProgress() const;

    /** Returns data-driven summaries of every player proficiency for UI presentation. */
    UFUNCTION(BlueprintCallable, Category = "Progression")
    TArray<FProficiencySummary> GetProficiencySummaries() const;

    /** Returns a snapshot of combat-relevant player attributes. */
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
    /** Buys stock into the player's inventory after server-side vendor, range, and currency validation. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorBuy(AMythicVendor *Vendor, int32 StockSlotIndex, int32 Quantity);

    /** Sells an owned inventory item to the open vendor and credits the validated proceeds. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorSell(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex, int32 Quantity);

    /** Buys listed stock from an accessible teammate stall and transfers payment to the stall owner. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Stall")
    void ServerStallBuy(AMythicPlayerStall *Stall, int32 StallSlotIndex, int32 Quantity);

    void RecordVendorAcquaintance(const AMythicVendor *Vendor, const struct FMythicTradePlan &Plan);

    /** Repairs one owned durable item after validating vendor access, range, ownership, and currency. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorRepair(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory, int32 PlayerSlotIndex);

    /** Repairs owned damaged items cheapest-first within the player's validated vendor budget. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerVendorRepairAll(AMythicVendor *Vendor, UMythicInventoryComponent *PlayerInventory);

    // Buy back a recently-sold instance from the vendor's buyback ring (indexed by BuybackIndex) at the buyback price —
    // the price it was sold for × the vendor's small markup. Same authorization as buy (vendor open + in range). The
    // vendor hands back the SAME instance (rolled affixes / item level / durability intact, never a re-mint); on a hard
    // reject (unaffordable / already gone / no room) the standard trade-result callout is surfaced.
    /** Buys back the exact recently sold item instance after validating access, price, and inventory capacity. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerBuyback(AMythicVendor *Vendor, int32 BuybackIndex);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Itemization")
    void ServerSetItemJunk(UMythicItemInstance *Item, bool bJunk);

    // Sell EVERY junk-flagged (manual OR auto), sellable item across the player's own inventories to Vendor, reusing the
    // per-item vendor sell path (AMythicVendor::Server_ExecuteSell) — so it's exactly-once with no dupe/loss, and the
    // vendor re-validates each item. Server-authoritative; the vendor must be open + in range (same gate as a manual
    // sell). Bounded: a single forward pass over the slots (equipment/currency/non-takeable slots are skipped by the
    // junk predicate). A successful sale already floats the "+N <currency>" callout per item, so no extra feedback here.
    /** Sells every eligible junk-marked item through the server-validated per-item vendor path. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Vendor")
    void ServerSellAllJunk(AMythicVendor *Vendor);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Gift")
    void ServerOfferGift(AMythicPlayerController *Recipient, UMythicInventoryComponent *SourceInv, int32 SourceSlotIndex, int32 Quantity = 0);

    UFUNCTION(Client, Reliable, Category = "Gift")
    void ClientReceiveGiftOffer(AMythicPlayerController *Giver, const FText &ItemName);

    UFUNCTION(Server, Reliable, Category = "Gift")
    void ServerRespondGift(bool bAccept);

    /** Blueprint event raised on the recipient client so UI can accept or decline a pending gift. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Gift")
    void OnGiftOffered(AMythicPlayerController *Giver, const FText &ItemName);

    UFUNCTION(Client, Reliable, Category = "Gift")
    void ClientNotifyGiftResult(const FText &Message, FLinearColor Color);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Placeable")
    void ServerDeployPlaceable(UMythicInventoryComponent *Inventory, int32 SlotIndex, FVector AimOrigin, FVector AimDirection);

    UFUNCTION(Client, Reliable, Category = "Placeable")
    void ClientNotifyDeployRejected(const FText &Reason);

    /** Blueprint event raised on the owning client when a placeable deployment is rejected. */
    UFUNCTION(BlueprintImplementableEvent, Category = "Placeable")
    void OnDeployRejected(const FText &Reason);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Interaction")
    void ServerInteractPrimary(AActor *Interactable);

    /**
     * Requests one contextual entity action using an opaque embodiment key and exact authority lease nonce shown locally.
     * Authority resolves the current subject, consumes only the nonce-bound issuing provider entry, and verifies the
     * focus/range/LOS and provider rules again; stale, pooled, hidden, or ambiguous requests cannot mutate the domain.
     */
    UFUNCTION(BlueprintCallable, Server, Reliable, Category = "World Presentation|Context Actions")
    void ServerExecuteContextAction(FMythicEntityPresentationInstance Subject, FGameplayTag ActionTag,
                                    int64 ObservedOfferRevision);

    /**
     * Starts authority timing for one authored hold action after resolving and validating the exact current offer.
     * This grants no execution right by itself; completion must arrive after the canonical duration and revalidate.
     */
    UFUNCTION(Server, Reliable, Category = "World Presentation|Context Actions")
    void ServerBeginContextActionHold(FMythicEntityPresentationInstance Subject,
                                      FGameplayTag ActionTag,
                                      int64 ObservedOfferRevision);

    /** Cancels the matching in-flight authority hold without mutating the action provider or consuming its grant. */
    UFUNCTION(Server, Reliable, Category = "World Presentation|Context Actions")
    void ServerCancelContextActionHold(FMythicEntityPresentationInstance Subject,
                                       FGameplayTag ActionTag,
                                       int64 ObservedOfferRevision);

    /**
     * Nominates the owning LocalPlayer attention service's current exact focus for server-owned contextual projections.
     * The client sends no combat inputs, action tags, text, definitions, or provider pointers; invalid clears all focus
     * leases immediately.
     */
    void RequestContextActionOfferRefresh(FMythicEntityPresentationInstance Subject);

    /**
     * Receives only an opaque focus instance from this owned controller. Authority independently rate-limits and
     * validates combat and action projections, resolves the exact registry generation, and rebuilds owner-only leases.
     */
    UFUNCTION(Server, Reliable, Category = "World Presentation|Context Actions")
    void ServerRequestContextActionOfferRefresh(FMythicEntityPresentationInstance Subject);

    /** Delivers a safe Context.Action.Reason.* rejection category to the owning client without private world truth. */
    UFUNCTION(Client, Reliable, Category = "World Presentation|Context Actions")
    void ClientNotifyContextActionRejected(FGameplayTag SafeReasonTag);

    /** Presentation hook for a server-rejected contextual action; UI resolves the safe reason tag to localized copy. */
    UFUNCTION(BlueprintImplementableEvent, Category = "World Presentation|Context Actions")
    void OnContextActionRejected(FGameplayTag SafeReasonTag);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Dialogue")
    void ServerRequestNpcDialogue(AMythicNPCCharacter *NPC);

    UFUNCTION(Client, Reliable, Category = "Dialogue")
    void ClientReceiveNpcDialogue(AMythicNPCCharacter *NPC, const FText &Line);

    /** Opens an already-authorized NPC service surface only on this controller's owning client. */
    UFUNCTION(Client, Reliable, Category = "Trade")
    void ClientOpenNpcTrade(AMythicNPCCharacter *NPC);

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

    /**
     * Delivers one bounded, server-resolved harvest outcome to the owning client. Unreliable is deliberate because
     * prompts/progress are transient and durable node state arrives independently through spatial replication.
     */
    UFUNCTION(Client, Unreliable, Category = "Harvesting")
    void ClientReceiveHarvestFeedback(const FMythicHarvestClientFeedback &Feedback);

    /**
     * Blueprint presentation event for an already-authoritative harvest outcome; implementations may animate UI,
     * audio, and VFX but cannot mutate work, eligibility, rewards, durability, proficiency, or node lifecycle.
     */
    UFUNCTION(BlueprintImplementableEvent, Category = "Harvesting")
    void OnHarvestFeedback(const FMythicHarvestClientFeedback &Feedback);

    /** Presents the rare shield-break callout; absorbed magnitudes use the unreliable resolved-combat-text batch. */
    UFUNCTION(Client, Reliable, Category = "Combat")
    void ClientShowShieldBroken();

    /** Presents transient dodge feedback without blocking authoritative reliable traffic. */
    UFUNCTION(Client, Unreliable, Category = "Combat")
    void ClientShowDodge();

    /** Adds one resolved result to this owning player's server-side, next-frame combat-text batch. */
    void QueueResolvedCombatText(const FMythicResolvedCombatTextEvent &Event);

    /**
     * Delivers a bounded batch of server-resolved combat numbers to this owning client. Unreliable is deliberate:
     * combat text is transient presentation and must never back-pressure authoritative combat during an ARPG burst.
     */
    UFUNCTION(Client, Unreliable, Category = "Combat")
    void ClientReceiveResolvedCombatTextBatch(const TArray<FMythicResolvedCombatTextEvent> &Events);

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

    /** Blueprint event raised on the owning client to present an item-reward celebration. */
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

    /** Requests a server-validated, paid reroll of every unlocked affix on one owned item. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Forge")
    void ServerRerollItemAffixes(UMythicItemInstance *Item);

    /** Requests a server-validated crafting lock change for one affix on an owned item. */
    UFUNCTION(BlueprintCallable, Server, Reliable, WithValidation, Category = "Forge")
    void ServerSetItemAffixLocked(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked);

    static bool IsWithinStationRange(float DistSq, float RangeSq);

    /**
     * The first time a status is inflicted on this player, name it and say what it does. Sent once per status per
     * character — the codex glossary is what remembers, so it survives a reload and never fires twice.
     */
    UFUNCTION(Client, Reliable, Category = "Status")
    void ClientNotifyStatusLearned(const FText &StatusName, const FText &Description, FLinearColor Accent);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Fast Travel")
    void ServerFastTravel(int32 SettlementId);

    static bool CanFastTravel(const TSet<int32> &Discovered, int32 SettlementId, bool bBlocked);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Fast Travel")
    void ServerFastTravelToPOI(int32 POIId);

    UFUNCTION(Client, Reliable, Category = "Fast Travel")
    void ClientNotifyFastTravelRefused(const FText &Reason);

    /** Returns whether current encumbrance blocks both settlement and point-of-interest fast travel. */
    UFUNCTION(BlueprintPure, Category = "Fast Travel")
    bool IsOverloadedForFastTravel() const;

private:
    struct FPendingContextActionHold {
        FMythicEntityPresentationInstance Subject;
        FGameplayTag ActionTag;
        uint32 OfferRevision = 0;
        double AuthorityStartSeconds = -DBL_MAX;
        float RequiredDurationSeconds = 0.0f;

        bool IsActive() const {
            return Subject.IsValid() && ActionTag.IsValid()
                && AuthorityStartSeconds >= 0.0
                && RequiredDurationSeconds > 0.0f;
        }

        bool Matches(const FMythicEntityPresentationInstance &InSubject,
                     const FGameplayTag InActionTag,
                     const uint32 InOfferRevision) const {
            return IsActive() && Subject == InSubject
                && ActionTag == InActionTag
                && OfferRevision == InOfferRevision;
        }

        void Reset() { *this = FPendingContextActionHold(); }
    };

    void BindContextActionAttention();
    void UnbindContextActionAttention();
    void HandleContextActionFocusChanged(
        const FMythicEntityPresentationInstance &PreviousSubject,
        const FMythicEntityPresentationInstance &NewSubject);
    void AuthorityRefreshContextActionOffers();
    void ScheduleContextActionOfferRefresh(float DelaySeconds);
    UMythicEntityActionGrantComponent *ResolveEntityActionGrantComponent() const;
    double GetContextActionRequestClockSeconds() const;
    double GetContextActionLeaseClockSeconds() const;
    bool ResolveAndValidateContextActionRequest(
        const FMythicEntityPresentationInstance &Subject,
        FGameplayTag ActionTag, uint32 ObservedOfferRevision,
        UMythicEntityActionGrantComponent *&OutGrantComponent,
        UObject *&OutProvider,
        UMythicContextActionDefinition *&OutDefinition,
        AActor *&OutSubjectActor,
        uint32 &OutProviderSourceRevision,
        FGameplayTag &OutFailureReason);
    void ResetPendingContextActionHold();
    void EnterContextActionAuthorityBarrier();
    void AuthoritySetCombatPresentationFocus(
        FMythicEntityPresentationInstance Subject);
    void AuthorityRefreshFocusedCombatPresentation();
    void ScheduleCombatPresentationRefresh(float DelaySeconds);
    void ClearAuthorityCombatPresentationFocus();
    UMythicEntityCombatPresentationComponent *
        ResolveEntityCombatPresentationComponent() const;
    double GetCombatPresentationRequestClockSeconds() const;
    double GetCombatPresentationLeaseClockSeconds() const;

    /** Local split-screen-safe attention service whose focus edge drives this controller's action and combat leases. */
    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicEntityAttentionSubsystem> BoundContextActionAttentionSubsystem;

    FDelegateHandle ContextActionFocusChangedHandle;
    FTimerHandle ContextActionOfferRefreshTimerHandle;
    FMythicEntityPresentationInstance AuthorityRequestedContextActionSubject;
    double LastContextActionProjectionSeconds = -DBL_MAX;
    bool bContextActionPolicyWarningEmitted = false;
    FPendingContextActionHold PendingContextActionHold;

    FTimerHandle CombatPresentationRefreshTimerHandle;
    FMythicEntityPresentationInstance AuthorityRequestedCombatPresentationSubject;
    double LastCombatPresentationClientRequestSeconds = -DBL_MAX;
    uint32 CombatPresentationSourceRevision = 0;
    bool bCombatPresentationPolicyWarningEmitted = false;

    /** Highest positive authority feedback sequence already presented by this controller; zero accepts first state. */
    int64 LastPresentedHarvestFeedbackSequence = 0;

    /** Flushes the pending combat-text batch at most once per server frame, or immediately when the batch is full. */
    void FlushResolvedCombatTextQueue();

    /** Resolved presentation events waiting for the next unreliable owner RPC. */
    UPROPERTY(Transient)
    TArray<FMythicResolvedCombatTextEvent> PendingResolvedCombatText;

    FTimerHandle ResolvedCombatTextFlushTimer;

    static constexpr int32 MaxResolvedCombatTextBatchSize = 32;

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

    /** Maximum simultaneous placeables owned by this player; zero leaves deployment uncapped. */
    UPROPERTY(EditDefaultsOnly, Category = "Placeable")
    int32 MaxDeployedPlaceables = 0;

    UPROPERTY()
    TArray<TWeakObjectPtr<AActor>> DeployedPlaceables;

    void CheckZoneEntry();

    FTimerHandle ZoneCheckTimerHandle;
    int32 LastSettlementId = INDEX_NONE;

    uint8 LastTerritoryFactionIndex = 0xFF;

    TSet<int32> DiscoveredSettlements;

    /** Seconds between authoritative checks for settlement and territory changes. */
    UPROPERTY(EditAnywhere, Category = "Zone", meta = (ClampMin = "0.1"))
    float ZoneCheckInterval = 1.0f;
};
