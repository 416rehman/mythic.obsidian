// Per-player objective/quest tracker — subscribes to GAS kill events, advances active objectives, grants rewards.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Subsystem/SaveSystem/Character/SavedObjective.h"
#include "ObjectiveTracker.generated.h"

class UObjectiveDefinition;
class UAbilitySystemComponent;
struct FGameplayEventData;

USTRUCT(BlueprintType)
struct FObjectiveSummary {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly)
    FText DisplayText;

    UPROPERTY(BlueprintReadOnly)
    int32 CurrentCount = 0;

    UPROPERTY(BlueprintReadOnly)
    int32 RequiredCount = 0;

    UPROPERTY(BlueprintReadOnly)
    float ProgressFraction = 0.0f;

    UPROPERTY(BlueprintReadOnly)
    bool bCompleted = false;
};

UENUM(BlueprintType)
enum class EObjectiveOfferResult : uint8 {
    Assigned,
    AlreadyActive,
    AlreadyCompleted,
    NoOffer,
    OutOfRange,
    PrerequisitesNotMet, // a prior step in this objective's chain isn't complete yet — assignment is gated
    Invalid
};

UENUM(BlueprintType)
enum class EObjectiveNotifyCategory : uint8 {
    Assignment,
    Duplicate,
    Progress,
    Completed,
    RewardResult
};

/** Per-player runtime progress toward one objective. Mirrors the faction-standing entry (small replicated struct
 *  in a TArray — no TMap). */
USTRUCT(BlueprintType)
struct FObjectiveProgress {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UObjectiveDefinition> Definition = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    int32 CurrentCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    bool bCompleted = false;
};

/**
 * Hosted on AMythicPlayerController. On the server it binds to the player's ASC GAS.Event.Kill callback (the same
 * attributed event combat already emits on the killer's ASC), advances every active matching objective, and on
 * reaching the required count grants the objective's rewards via FRewardsToGive::Give and latches it complete. The
 * objective list replicates COND_OwnerOnly so only the owning client sees its own quest progress (for UI).
 *
 * Scope: combat AND item-acquisition triggers (with optional payload-tag + magnitude objective filters), reward-on-
 * complete, owner-only replication, and save persistence (SaveObjectives/RestoreObjectives via the character save).
 * Multi-step chains, branching, prerequisites, other non-combat verbs (reach-location, talk-to-NPC), and the
 * persistent tracker UI are follow-ups.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UObjectiveTracker : public UActorComponent {
    GENERATED_BODY()

public:
    UObjectiveTracker();

    // SERVER: assign an objective to this player (authority-gated; mirrors FactionStanding's server gate).
    // Idempotent — a no-op if this player already has the objective (active or completed), so a quest-giver NPC
    // can be talked to repeatedly without re-adding or resetting the quest.
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    void ServerAddObjective(UObjectiveDefinition *Definition);

    // SERVER/internal authority method, not a client RPC. Callers must derive Definition from server-owned state.
    EObjectiveOfferResult ServerTryAddObjective(UObjectiveDefinition *Definition, FObjectiveProgress &OutProgress);

    // True if this player already tracks the given objective (active or completed). Used to gate re-offers.
    UFUNCTION(BlueprintPure, Category = "Objectives")
    bool HasObjective(const UObjectiveDefinition *Definition) const;

    static EObjectiveOfferResult ResolveObjectiveOfferResult(const TArray<FObjectiveProgress> &TrackedObjectives,
                                                             const UObjectiveDefinition *Definition,
                                                             FObjectiveProgress &OutProgress);

    // Pure: every prerequisite definition must appear COMPLETED in the player's tracked set. Empty prerequisites →
    // always met (no chain). A null prerequisite entry is ignored (designer slop, not a blocker); a prerequisite that
    // is untracked or tracked-but-incomplete fails the gate. Static so the chain rule is unit-testable without a tracker.
    static bool AreObjectivePrerequisitesMet(const TArray<TObjectPtr<UObjectiveDefinition>> &Prerequisites,
                                             const TArray<FObjectiveProgress> &TrackedObjectives);

    // Pure: of CandidateNext, which objectives are newly assignable given TrackedObjectives — i.e. those for which
    // ResolveObjectiveOfferResult would return Assigned (not already active/completed, prerequisites met). Null entries
    // skipped; deduped. This is the chain-advance set: when an objective completes, its NextObjectives run through here.
    static void CollectAssignableNextObjectives(const TArray<TObjectPtr<UObjectiveDefinition>> &CandidateNext,
                                                const TArray<FObjectiveProgress> &TrackedObjectives,
                                                TArray<UObjectiveDefinition *> &OutAssignable);

    static FText BuildObjectiveNotificationText(const FText &DisplayText, EObjectiveNotifyCategory Category,
                                                EObjectiveOfferResult OfferResult, int32 Current, int32 Required,
                                                bool bRewardSucceeded, bool bRewardDroppedNearby);

    // Pure: one objective-advance step. Adds the advance (the event magnitude rounded + floored at 1 when the
    // objective counts by magnitude, else +1) to CurrentCount, clamps to RequiredCount on completion (magnitude can
    // overshoot), and reports whether THIS step completed it. Assumes the objective is not already complete (the
    // caller skips completed ones). Static so the quest-progression arithmetic is unit-testable without a live ASC/event.
    static void ComputeObjectiveProgress(int32 CurrentCount, bool bCountByMagnitude, float EventMagnitude,
                                         int32 RequiredCount, int32 &OutNewCount, bool &OutJustCompleted);

    // SERVER (save): snapshot every tracked objective (definition soft-path + count + completed) for the character save.
    void SaveObjectives(TArray<FSerializedObjectiveData> &OutData) const;

    // SERVER (load): replace the tracked set from a save, re-subscribing each restored objective's trigger tag so an
    // incomplete one keeps advancing. Authority-gated. Idempotent (rebuilds from InData).
    void RestoreObjectives(const TArray<FSerializedObjectiveData> &InData);

    // server-authoritative: abandon an active, non-completed objective
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Objectives")
    void ServerAbandonObjective(UObjectiveDefinition *Def);

    // builds UI-ready summaries from all active objectives
    UFUNCTION(BlueprintPure, Category = "Objectives")
    TArray<FObjectiveSummary> GetActiveObjectiveSummaries() const;

    // number of non-completed active objectives
    UFUNCTION(BlueprintPure, Category = "Objectives")
    int32 GetActiveCount() const;

    // number of completed objectives
    UFUNCTION(BlueprintPure, Category = "Objectives")
    int32 GetCompletedCount() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    // SERVER: GAS gameplay-event callback bound to GAS.Event.Kill on the owning player's ASC.
    void HandleGameplayEvent(const FGameplayEventData *Payload);

    // Active + completed objectives for this player. Owner-only so quest progress stays private to its owner.
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Objectives")
    TArray<FObjectiveProgress> ActiveObjectives;

private:
    // The ASC we bound on (the PlayerState's ASC, reused across pawns).
    UPROPERTY()
    UAbilitySystemComponent *BoundASC = nullptr;

    // SERVER: subscribe HandleGameplayEvent to a given trigger tag's gameplay event on BoundASC (idempotent).
    void EnsureSubscribedToTag(const FGameplayTag &Tag);

    // One bound handle per DISTINCT objective TriggerEventTag, so objectives keyed to any emitted GAS.Event.*
    // (Kill / Death / Dmg.Delivered / Heal.* …) advance — not just kills. EndPlay unbinds each.
    TMap<FGameplayTag, FDelegateHandle> BoundEventHandles;

    // client RPC notifying the player that an objective was abandoned
    UFUNCTION(Client, Reliable)
    void ClientNotifyObjectiveAbandoned(const FText& ObjectiveName);
};
