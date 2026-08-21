
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "GameplayTagContainer.h"
#include "Subsystem/SaveSystem/Character/SavedQuestJournal.h"
#include "MythicQuestJournalComponent.generated.h"

class UMythicQuestDefinition;
class UMythicStorylineDefinition;
class UObjectiveDefinition;
class UObjectiveTracker;
class UMythicNarrativeStateComponent;
class APlayerController;

UENUM(BlueprintType)
enum class EMythicTaskState : uint8 {
    NotStarted,
    Active,
    Complete,
    Failed
};

UENUM(BlueprintType)
enum class EMythicQuestState : uint8 {
    NotStarted,
    Active,
    Completed,
    Failed
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnQuestsChanged);

USTRUCT(BlueprintType)
struct FMythicQuestJournalEntry {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    TObjectPtr<UMythicQuestDefinition> Quest = nullptr;

    UPROPERTY(BlueprintReadOnly, Category = "Quest")
    EMythicQuestState State = EMythicQuestState::NotStarted;
};

UCLASS(ClassGroup = (Mythic), meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicQuestJournalComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicQuestJournalComponent();

    static EMythicQuestState DeriveQuestState(const TArray<EMythicTaskState> &TaskStates,
                                              const TArray<bool> &TaskRequiredMask,
                                              bool bExclusiveLockTripped);

    // SERVER: start tracking a quest. No-op off authority, if already tracked, or if its UnlockConditions fail against
    // the player's owned tags. Adds a journal entry, assigns each Task to the ObjectiveTracker, then recomputes.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ServerStartQuest(UMythicQuestDefinition *Quest);

    // SERVER: start a storyline. No-op off authority or if its ArcGate fails. Registers the arc and starts its first
    // eligible quest; subsequent quests auto-advance as each completes.
    UFUNCTION(BlueprintCallable, Category = "Quest")
    void ServerStartStoryline(UMythicStorylineDefinition *Storyline);

    // Current rolled-up state of a quest (NotStarted if not tracked).
    UFUNCTION(BlueprintPure, Category = "Quest")
    EMythicQuestState GetQuestState(const UMythicQuestDefinition *Quest) const;

    const TArray<FMythicQuestJournalEntry> &GetQuests() const { return Quests; }

    FText FindQuestTitleForTask(const class UObjectiveDefinition *Task) const;

    // BlueprintPure copy of the same list, for UI that cannot take a const-ref getter.
    UFUNCTION(BlueprintPure, Category = "Quest")
    TArray<FMythicQuestJournalEntry> GetJournalEntries() const { return Quests; }

    // Journal-changed signal for UI. Fires on BOTH sides: server-side after a recompute actually moved something, and
    // client-side from OnRep_Quests. A listen-server host never receives its own OnRep, so both paths are required.
    UPROPERTY(BlueprintAssignable, Category = "Quest")
    FMythicOnQuestsChanged OnQuestsChanged;


    void GetSerializableJournal(TArray<FSerializedQuestJournalEntry> &OutQuests,
                                TArray<FSoftObjectPath> &OutActiveStorylines,
                                TArray<FSoftObjectPath> &OutCompletedStorylines) const;

    void RestoreQuests(const TArray<FSerializedQuestJournalEntry> &InQuests,
                       const TArray<FSoftObjectPath> &InActiveStorylines,
                       const TArray<FSoftObjectPath> &InCompletedStorylines);

    static FSerializedQuestJournalEntry MakeSerializedEntry(const FMythicQuestJournalEntry &Entry);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

protected:
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    UFUNCTION()
    void HandleObjectivesChanged();

    void RecomputeQuests();

    EMythicQuestState ComputeQuestState(const UMythicQuestDefinition *Quest) const;

    void ApplyQuestCompleted(UMythicQuestDefinition *Quest);

    void AdvanceStorylines(UMythicQuestDefinition *JustCompleted);

    APlayerController *ResolvePlayerController() const;
    UMythicNarrativeStateComponent *ResolveLedger() const;
    UObjectiveTracker *ResolveTracker() const;

    FGameplayTagContainer GatherOwnedTags() const;

    const FMythicQuestJournalEntry *FindEntry(const UMythicQuestDefinition *Quest) const;

    bool IsArcComplete(const UMythicStorylineDefinition *Arc) const;

    void GrantArcRewards(const UMythicStorylineDefinition *Arc);

    // Tracked quests. Owner-only so quest progress stays private to its owner (mirrors the tracker/ledger).
    UPROPERTY(ReplicatedUsing = OnRep_Quests, BlueprintReadOnly, Category = "Quest")
    TArray<FMythicQuestJournalEntry> Quests;

    UFUNCTION()
    void OnRep_Quests();

    UPROPERTY()
    TArray<TObjectPtr<UMythicStorylineDefinition>> ActiveStorylines;

    UPROPERTY()
    TSet<TObjectPtr<UMythicStorylineDefinition>> CompletedStorylines;

private:
    void EnsureBoundToTracker();

    bool StartQuestInternal(UMythicQuestDefinition *Quest);

    bool bRecomputing = false;
    bool bBoundToTracker = false;
};
