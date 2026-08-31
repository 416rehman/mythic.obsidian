#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Subsystem/SaveSystem/Character/SavedObjective.h"
#include "ObjectiveDefinition.h"
#include "ObjectiveTracker.generated.h"

class UObjectiveDefinition;
class UAbilitySystemComponent;
class UMythicNarrativeStateComponent;
class UMythicHarvestableDefinition;
struct FGameplayEventData;
struct FMythicObjectiveBranch;
enum class EMythicObjectiveOutcome : uint8;

/** Native terminal result of consuming typed harvest credit for exactly-once receipt delivery. */
enum class EMythicHarvestQuestCreditConsumeResult : uint8 {
    Rejected,
    ConsumedMatched,
    ConsumedNoMatch,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnObjectivesChanged);

struct FMythicObjectiveBranchResult {
    bool bMatched = false;
    TArray<UObjectiveDefinition *> Assignable;
    FGameplayTagContainer GrantStoryTags;
    TArray<UObjectiveDefinition *> CancelSiblings;
};

struct FMythicPendingObjectiveCompletion {
    TObjectPtr<UObjectiveDefinition> Definition = nullptr;
    FGameplayTag AchievedOutcome;
};

USTRUCT(BlueprintType)
struct FObjectiveSummary {
    GENERATED_BODY()

    /** Player-facing objective text for the tracker row. */
    UPROPERTY(BlueprintReadOnly)
    FText DisplayText;

    /** Current authoritative progress count. */
    UPROPERTY(BlueprintReadOnly)
    int32 CurrentCount = 0;

    /** Count required to complete the objective. */
    UPROPERTY(BlueprintReadOnly)
    int32 RequiredCount = 0;

    /** Clamped current-to-required progress ratio for UI presentation. */
    UPROPERTY(BlueprintReadOnly)
    float ProgressFraction = 0.0f;

    /** Whether the objective has completed. */
    UPROPERTY(BlueprintReadOnly)
    bool bCompleted = false;

    // The quest this objective belongs to (empty = standalone) — the tracker groups objectives under one quest header.
    /** Player-facing quest heading; empty denotes a standalone objective. */
    UPROPERTY(BlueprintReadOnly)
    FText QuestName;

    // Optional (secondary) objective — the tracker shows it dimmed.
    /** Whether this is a secondary objective that does not gate quest completion. */
    UPROPERTY(BlueprintReadOnly)
    bool bOptional = false;
};

/** Exhaustive server result returned when Blueprint requests one objective offer. */
UENUM(BlueprintType)
enum class EObjectiveOfferResult : uint8 {
    Assigned,
    AlreadyActive,
    AlreadyCompleted,
    NoOffer,
    OutOfRange,
    PrerequisitesNotMet,
    PreconditionNotMet,
    Invalid
};

/** Presentation category for one immutable objective notification delivered to Blueprint. */
UENUM(BlueprintType)
enum class EObjectiveNotifyCategory : uint8 {
    Assignment,
    Duplicate,
    Progress,
    Completed,
    RewardResult
};

USTRUCT(BlueprintType)
struct FObjectiveProgress {
    GENERATED_BODY()

    /** Direct definition that owns the authored rules for this tracked objective. */
    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UObjectiveDefinition> Definition = nullptr;

    /** Current server-authoritative progress count. */
    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    int32 CurrentCount = 0;

    /** Whether this tracked objective has completed. */
    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    bool bCompleted = false;

    // Server world-time (seconds) at which this objective completed — drives the bRepeatable re-accept cooldown
    // (CanRepeatObjective). 0 until completed. Runtime-only (not saved; resets on reload — see UObjectiveDefinition).
    /** Server world time at completion, or zero until completion; used for session repeat cooldowns. */
    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    float CompletedTimeSeconds = 0.0f;
};

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UObjectiveTracker : public UActorComponent {
    GENERATED_BODY()

public:
    UObjectiveTracker();

    // SERVER: assign an objective to this player (authority-gated; mirrors FactionStanding's server gate).
    // Idempotent — a no-op if this player already has the objective (active or completed), so a quest-giver NPC
    // can be talked to repeatedly without re-adding or resetting the quest.
    /** Authority-gated, idempotently adds an eligible objective to this player's tracker. */
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    void ServerAddObjective(UObjectiveDefinition *Definition);

    EObjectiveOfferResult ServerTryAddObjective(UObjectiveDefinition *Definition, FObjectiveProgress &OutProgress);

    /**
     * Evaluates an offer without mutating objective state. Authority producers use this to decide whether an NPC may
     * disclose a viewer-private quest opportunity; repeatable cooldowns use current server time.
     */
    EObjectiveOfferResult EvaluateObjectiveOffer(
        const UObjectiveDefinition *Definition,
        FObjectiveProgress &OutProgress) const;

    // True if this player already tracks the given objective (active or completed). Used to gate re-offers.
    /** Returns whether this tracker already contains the objective as active or completed. */
    UFUNCTION(BlueprintPure, Category = "Objectives")
    bool HasObjective(const UObjectiveDefinition *Definition) const;

    bool FindObjectiveProgress(const UObjectiveDefinition *Def, FObjectiveProgress &OutProgress) const;

    const TArray<FObjectiveProgress> &GetActiveObjectives() const { return ActiveObjectives; }

    // Server-side task-completion signal — see FMythicOnObjectivesChanged. BlueprintAssignable so UI/systems can react.
    /** Broadcasts after replicated or authoritative objective state changes so UI and gameplay listeners can refresh. */
    UPROPERTY(BlueprintAssignable, Category = "Objectives")
    FMythicOnObjectivesChanged OnObjectivesChanged;

    static EObjectiveOfferResult ResolveObjectiveOfferResult(const TArray<FObjectiveProgress> &TrackedObjectives,
                                                             const UObjectiveDefinition *Definition,
                                                             FObjectiveProgress &OutProgress,
                                                             const FGameplayTagContainer &OwnedStoryTags = FGameplayTagContainer());

    static bool AreObjectivePrerequisitesMet(const TArray<TObjectPtr<UObjectiveDefinition>> &Prerequisites,
                                             const TArray<FObjectiveProgress> &TrackedObjectives);

    static void CollectAssignableNextObjectives(const TArray<TObjectPtr<UObjectiveDefinition>> &CandidateNext,
                                                const TArray<FObjectiveProgress> &TrackedObjectives,
                                                TArray<UObjectiveDefinition *> &OutAssignable,
                                                const FGameplayTagContainer &OwnedStoryTags = FGameplayTagContainer());

    static FGameplayTag DeriveAchievedOutcome(const UObjectiveDefinition *Def, const FGameplayTag &CompletingEventTag,
                                              const FGameplayTagContainer &CompletingPayloadTags);

    static EMythicObjectiveOutcome ClassifyOutcome(const FGameplayTag &CompletingEventTag);

    static FMythicObjectiveBranchResult SelectBranchForOutcome(const TArray<FMythicObjectiveBranch> &Branches,
                                                               FGameplayTag AchievedOutcome,
                                                               const TArray<FObjectiveProgress> &TrackedObjectives,
                                                               const FGameplayTagContainer &OwnedStoryTags = FGameplayTagContainer());

    static FText BuildObjectiveNotificationText(const FText &DisplayText, EObjectiveNotifyCategory Category,
                                                EObjectiveOfferResult OfferResult, int32 Current, int32 Required,
                                                bool bRewardSucceeded, bool bRewardDroppedNearby);

    void ApplySharedKillCredit(const FGameplayEventData &Payload);

    /**
     * Advances only objectives with an exact direct harvestable-definition match after an authoritative node commit.
     * Native server code owns invocation; invalid/nonpositive input is inert and no gameplay tag or string is accepted.
     */
    void ApplyHarvestCompletionCredit(
        const UMythicHarvestableDefinition &HarvestableDefinition,
        int32 CreditCount);

    /**
     * Consumes one typed harvest entitlement. A valid authority invocation is terminal even when no active objective
     * matches, allowing the caller to persist a no-match receipt instead of banking credit for a future quest.
     */
    EMythicHarvestQuestCreditConsumeResult ConsumeHarvestCompletionCredit(
        const UMythicHarvestableDefinition &HarvestableDefinition,
        int32 CreditCount);

    /** Pure direct-reference match used by the typed harvest channel and native automation. */
    static bool MatchesHarvestableDefinition(
        const UObjectiveDefinition *Objective,
        const UMythicHarvestableDefinition *HarvestableDefinition);

    static void ComputeObjectiveProgress(int32 CurrentCount, bool bCountByMagnitude, float EventMagnitude,
                                         int32 RequiredCount, int32 &OutNewCount, bool &OutJustCompleted);

    static int32 ComputeDeliverConsumeCount(int32 CurrentCount, int32 RequiredCount, int32 Available);

    static bool CanRepeatObjective(bool bRepeatable, float CompletedTimeSeconds, float NowSeconds, float RepeatCooldownSeconds);

    void ServerTurnInDeliveriesTo(const FGameplayTag &NpcTag, class UMythicInventoryComponent *PlayerInventory);

    /**
     * Returns whether talking to this NPC tag can currently advance a talk objective or consume at least one relevant
     * delivery item. This read-only authority query never exposes another player's objective state.
     */
    bool CanAdvanceNpcInteraction(
        const FGameplayTag &NpcTag,
        const class UMythicInventoryComponent *PlayerInventory) const;

    void SaveObjectives(TArray<FSerializedObjectiveData> &OutData) const;

    void RestoreObjectives(const TArray<FSerializedObjectiveData> &InData);

    // server-authoritative: abandon an active, non-completed objective
    /** Reliably asks authority to abandon an active, incomplete objective. */
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Objectives")
    void ServerAbandonObjective(UObjectiveDefinition *Def);

    // builds UI-ready summaries from all active objectives
    /** Builds UI-ready summaries for every currently tracked objective. */
    UFUNCTION(BlueprintPure, Category = "Objectives")
    TArray<FObjectiveSummary> GetActiveObjectiveSummaries() const;

    // number of non-completed active objectives
    /** Returns the number of tracked objectives that have not completed. */
    UFUNCTION(BlueprintPure, Category = "Objectives")
    int32 GetActiveCount() const;

    // number of completed objectives
    /** Returns the number of completed objectives retained by this tracker. */
    UFUNCTION(BlueprintPure, Category = "Objectives")
    int32 GetCompletedCount() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    void HandleGameplayEvent(const FGameplayEventData *Payload);

    void AdvanceObjectiveProgress(FObjectiveProgress &Prog, bool bCountByMagnitude, float Magnitude,
                                  class APlayerController *PC, const FGameplayTag &CompletingEventTag,
                                  const FGameplayTagContainer &CompletingPayloadTags,
                                  TArray<FMythicPendingObjectiveCompletion> &PendingCompletions, int32 &NotifyIndex);

    void ProcessChainAdvance(class APlayerController *PC,
                             TArray<FMythicPendingObjectiveCompletion> &PendingCompletions, int32 &NotifyIndex);

    UMythicNarrativeStateComponent *ResolveNarrativeComponent() const;

    FGameplayTagContainer GatherOwnedStoryTags() const;

    // Active + completed objectives for this player. Owner-only so quest progress stays private to its owner.
    // ReplicatedUsing, so the OWNING CLIENT gets the same OnObjectivesChanged signal the server fires locally. Without
    // it a remote client received full objective data and was never told it had arrived, which is why quest UI could
    // only ever work on a listen-server host.
    /** Owner-only replicated active and completed objective state used by the player's quest UI. */
    UPROPERTY(ReplicatedUsing = OnRep_ActiveObjectives, BlueprintReadOnly, Category = "Objectives")
    TArray<FObjectiveProgress> ActiveObjectives;

    UFUNCTION()
    void OnRep_ActiveObjectives();

private:
    UPROPERTY()
    UAbilitySystemComponent *BoundASC = nullptr;

    void EnsureSubscribedToTag(const FGameplayTag &Tag);

    TMap<FGameplayTag, FDelegateHandle> BoundEventHandles;

    UFUNCTION(Client, Reliable)
    void ClientNotifyObjectiveAbandoned(const FText& ObjectiveName);
};
