
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
struct FGameplayEventData;
struct FMythicObjectiveBranch;
enum class EMythicObjectiveOutcome : uint8;

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

    // The quest this objective belongs to (empty = standalone) — the tracker groups objectives under one quest header.
    UPROPERTY(BlueprintReadOnly)
    FText QuestName;

    // Optional (secondary) objective — the tracker shows it dimmed.
    UPROPERTY(BlueprintReadOnly)
    bool bOptional = false;
};

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

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    TObjectPtr<UObjectiveDefinition> Definition = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    int32 CurrentCount = 0;

    UPROPERTY(BlueprintReadOnly, Category = "Objective")
    bool bCompleted = false;

    // Server world-time (seconds) at which this objective completed — drives the bRepeatable re-accept cooldown
    // (CanRepeatObjective). 0 until completed. Runtime-only (not saved; resets on reload — see UObjectiveDefinition).
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
    UFUNCTION(BlueprintCallable, Category = "Objectives")
    void ServerAddObjective(UObjectiveDefinition *Definition);

    EObjectiveOfferResult ServerTryAddObjective(UObjectiveDefinition *Definition, FObjectiveProgress &OutProgress);

    // True if this player already tracks the given objective (active or completed). Used to gate re-offers.
    UFUNCTION(BlueprintPure, Category = "Objectives")
    bool HasObjective(const UObjectiveDefinition *Definition) const;

    bool FindObjectiveProgress(const UObjectiveDefinition *Def, FObjectiveProgress &OutProgress) const;

    const TArray<FObjectiveProgress> &GetActiveObjectives() const { return ActiveObjectives; }

    // Server-side task-completion signal — see FMythicOnObjectivesChanged. BlueprintAssignable so UI/systems can react.
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

    static void ComputeObjectiveProgress(int32 CurrentCount, bool bCountByMagnitude, float EventMagnitude,
                                         int32 RequiredCount, int32 &OutNewCount, bool &OutJustCompleted);

    static int32 ComputeDeliverConsumeCount(int32 CurrentCount, int32 RequiredCount, int32 Available);

    static bool CanRepeatObjective(bool bRepeatable, float CompletedTimeSeconds, float NowSeconds, float RepeatCooldownSeconds);

    void ServerTurnInDeliveriesTo(const FGameplayTag &NpcTag, class UMythicInventoryComponent *PlayerInventory);

    void SaveObjectives(TArray<FSerializedObjectiveData> &OutData) const;

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
