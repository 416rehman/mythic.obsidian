// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "CommonPlayerController.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "MythicPlayerController.generated.h"

struct FTrackedDestructibleData;
class UMythicItemInstance;
class UItemDefinition;
class AMythicConversionStation;
class AMythicStorageContainer;
class AMythicNPCCharacter;

class UMythicCheatManager;

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

    // Crafting Component
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Crafting")
    class UCraftingComponent *CraftingComponent;

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
    virtual void OnRep_PlayerState() override;

    // Client-side event when the player is possessed
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic")
    void OnPossessedOnClient();

protected:
    virtual void SetupInputComponent() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual void PostProcessInput(const float DeltaTime, const bool bGamePaused) override;

    // Login with the saved credentials if passed in via command line arguments. Otherwise, start the login process.
    // I.e Before this function is called, you could display a "Logging in..." message to the user.
    void Login(int32 LocalUserNum);

    // Handle Login Response. This is called when the login process is complete.
    void CB_LoginResponse(int32 LocalUserNum, bool bWasSuccessful, const FUniqueNetId &UserId, const FString &Error);

    // Delegate for handling the login response
    FDelegateHandle LoginDelegateHandle;

public:
    UFUNCTION(BlueprintCallable, Category = "Proficiency")
    const UProficiencyComponent *GetProficiencyComponent() const;

    // The player's primary inventory component (used by the storage move RPC's identity check + UI binding).
    UFUNCTION(BlueprintCallable, Category = "Inventory")
    UMythicInventoryComponent *GetInventoryComponent() const { return InventoryComponent; }

    // ---- Conversion station RPCs (client-owned PC -> trusted sender identity) ----
    // Registers this player as an instigator of the station (range + station-use gate enforced server-side).
    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerOpenConversionStation(AMythicConversionStation *Station);

    // Requests a manual conversion of the given recipe (qty cycles) at the station.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerConversionRequestStart(AMythicConversionStation *Station, FGameplayTag RecipeId, int32 Quantity);

    // Cancels one of this player's jobs at the station (refunds reserved inputs).
    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerConversionCancelJob(AMythicConversionStation *Station, int32 JobId);

    // Toggles auto-repeat on this player's active continuous job at the station.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Conversion")
    void ServerConversionSetAutoRepeat(AMythicConversionStation *Station, bool bRepeat);

    // ---- Storage container RPCs (client-owned PC -> trusted sender identity) ----
    // Registers this player as having the container open (range-gated server-side).
    UFUNCTION(Server, Reliable, WithValidation, Category = "Storage")
    void ServerOpenStorageContainer(AMythicStorageContainer *Container);

    // Moves an item between two inventories the player is allowed to act on (own inventory <-> an open,
    // in-range container). Both directions. Server-authoritative, identity- + range- + take-rule-gated.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Storage")
    void ServerMoveItemBetweenInventories(UMythicInventoryComponent *Source, int32 SourceSlot, UMythicInventoryComponent *Target, int32 TargetSlot);

    // ---- NPC dialogue (client-owned PC -> server picks the contextual line from authoritative brain state) ----
    // The NPC brain's dialogue context (Faction/Role/pressure) is server-only + non-replicated, so the line MUST
    // be chosen server-side; it round-trips back to the requesting client for display.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Dialogue")
    void ServerRequestNpcDialogue(AMythicNPCCharacter *NPC);

    UFUNCTION(Client, Reliable, Category = "Dialogue")
    void ClientReceiveNpcDialogue(AMythicNPCCharacter *NPC, const FText &Line);

    // ---- Party recruit ----
    // Recruit a (recruitable) NPC into the local player's party. Server-authoritative (the party subsystem is
    // server-side); routed from the NPC's primary-interact verb. Re-validates range/eligibility + dup-gates, and
    // falls through to dialogue when the NPC is already a companion.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Party")
    void ServerRecruitNpc(AMythicNPCCharacter *NPC);

    UFUNCTION(Client, Reliable, Category = "Party")
    void ClientReceiveRecruitResult(AMythicNPCCharacter *NPC, bool bSucceeded);

    // SERVER: assign this NPC's QuestOffer (if any) to the player, range-validated + idempotent. Single source (Rule 3)
    // for the talk AND recruit verbs so a recruitable quest-giver still hands out its quest.
    void OfferNpcQuestIfAny(AMythicNPCCharacter *NPC);

    // ---- Gathering feedback ----
    // Intermediate "N left" gather progress floater at the node — cosmetic + droppable, so kept OFF the reliable
    // channel (Unreliable). Fired per hit; server (the resource manager is authority-only) → the gathering client.
    UFUNCTION(Client, Unreliable, Category = "Gathering")
    void ClientShowGatherProgress(FVector Location, int32 HitsRemaining);

    // Terminal "Depleted!" gather callout — the one that matters, so delivered RELIABLY (fired once when the node yields).
    UFUNCTION(Client, Reliable, Category = "Gathering")
    void ClientShowGatherDepleted(FVector Location);

    // ---- Shield combat feedback ----
    // Float a blue "absorbed N" number (+ a "Shield Broken!" beat when bBroke) over the player when their shield eats
    // damage. Fired from the AUTHORITY's real-damage path (MythicAttributeSet_Defense::PostGameplayEffectExecute) — a
    // shield-absorbed hit never reaches Health (so no damage cue fires), and driving this from the server's damage
    // branch (not a client OnRep delta) means a MaxShield re-clamp can't masquerade as absorbed damage.
    UFUNCTION(Client, Reliable, Category = "Combat")
    void ClientShowShieldAbsorbed(int32 Absorbed, bool bBroke);

    // Float a "DODGE" callout over the player when their DodgeChance negates a hit. A dodge negates the hit BEFORE the
    // damage cue runs (no number would otherwise show), so the authority damage execution pushes this to the dodging
    // player. Mirrors the shield-absorbed callout.
    UFUNCTION(Client, Reliable, Category = "Combat")
    void ClientShowDodge();

    // ---- Progression feedback ----
    // Float a "<Skill> Lv N" callout (+ a "<Milestone> unlocked!" beat when a key milestone is crossed) over the player
    // on a proficiency level-up. Server (the ProficiencyComponent reward path, authority-gated) → the owning client.
    UFUNCTION(Client, Reliable, Category = "Progression")
    void ClientNotifyProficiencyLevel(const FText &ProfName, int32 NewLevel, const FText &MilestoneName);

    // ---- Objective feedback ----
    // Float "<Objective> N/M" on each step, or "Objective Complete: <Objective>" on the finishing step, over the player.
    // Server (the ObjectiveTracker, authority-only) → the owning client. (A persistent tracker HUD is a logged follow-up.)
    UFUNCTION(Client, Reliable, Category = "Objectives")
    void ClientNotifyObjective(const FText &DisplayText, int32 Current, int32 Required, bool bCompleted, int32 StackIndex);

    // ---- Loot pickup feedback ----
    // Float "+N <ItemName>" (or "<ItemName>" when N==1) over the player, tinted by item rarity, on a GENUINE pickup/add.
    // Server (the inventory authority add path: AddItem / PickupItem) → the owning client. RarityColor is resolved
    // server-side from UItemDefinition::GetRarityColor (single source). Mirrors ClientNotifyObjective.
    UFUNCTION(Client, Reliable, Category = "Itemization")
    void ClientNotifyLootPickup(const FText &ItemName, int32 Quantity, FLinearColor RarityColor);

    // ---- Companion loss feedback ----
    // Float "<Name> has left your party" (grey) / "<Name> turns on you!" (red) over the departing companion — closing
    // the asymmetry where the recruit is celebrated but the loss is silent. Server (the party subsystem) → owning client.
    UFUNCTION(Client, Reliable, Category = "Party")
    void ClientNotifyCompanionDeparted(const FText &Name, FVector Location);

    UFUNCTION(Client, Reliable, Category = "Party")
    void ClientNotifyCompanionBetrayed(const FText &Name, FVector Location);

    // ---- NPC trade (barter) ----
    // Execute one of the merchant NPC's designer-authored barter offers: server validates range + the offer +
    // that the player has the cost items, removes the cost, and mints the reward. Item-for-item (no currency in
    // v1). Opening the trade UI is client-local (offers are class data); only this execution round-trips.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Trade")
    void ServerExecuteBarterOffer(AMythicNPCCharacter *NPC, int32 OfferIndex);

    // ---- Forge: affix reroll / lock ----
    // Drive the (iter-51) AffixesFragment reroll/lock backend behind a server-authoritative, ownership-validated
    // verb: the item must live in one of THIS player's own inventories (same ownership model as item moves), so a
    // player cannot reroll or lock a third party's gear. The reroll/lock themselves are also authority-gated in the
    // fragment. Cost-gating is deferred (no currency model) — this is the no-cost Forge verb.
    UFUNCTION(Server, Reliable, WithValidation, Category = "Forge")
    void ServerRerollItemAffixes(UMythicItemInstance *Item);

    UFUNCTION(Server, Reliable, WithValidation, Category = "Forge")
    void ServerSetItemAffixLocked(UMythicItemInstance *Item, int32 AffixIndex, bool bLocked);

    // ---- Zone-entry feedback ("Welcome to <settlement>") ----
    // Float a "Welcome to <Name>" callout when the player crosses into a new settlement. The server polls the player's
    // cell on a timer (a position-based check has no delegate to bind), maps cell -> governing settlement, and on a
    // change of the stable SettlementId fires this RPC to the owning client. Implements the zone-entry detection
    // AMythicSettlement documents (MythicSettlement.h) but that nothing ever wired.
    UFUNCTION(Client, Reliable, Category = "Zone")
    void ClientNotifyZoneEntry(const FText &SettlementName);

private:
    // Server-side per-player poll: maps the pawn's cell -> governing settlement and fires ClientNotifyZoneEntry on a
    // change (INDEX_NONE = wilderness/none). Game thread; CopySettlementAtCell is SimulationLock-guarded (snapshot copy).
    void CheckZoneEntry();

    FTimerHandle ZoneCheckTimerHandle;
    int32 LastSettlementId = INDEX_NONE; // settlement the player was last in (INDEX_NONE = wilderness / none)

    // How often (seconds) the server re-checks which settlement the player occupies. Designer-tunable, not a magic literal.
    UPROPERTY(EditAnywhere, Category = "Zone", meta = (ClampMin = "0.1"))
    float ZoneCheckInterval = 1.0f;
};
