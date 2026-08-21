
#include "MythicQuestJournalComponent.h"

#include "Mythic.h"
#include "Net/UnrealNetwork.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/PlayerController.h"

#include "Narrative/MythicQuestDefinition.h"
#include "Narrative/MythicStorylineDefinition.h"
#include "Narrative/MythicQuestOutcome.h"
#include "Narrative/MythicNarrativeStateComponent.h"
#include "Narrative/MythicNarrativeGrant.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Player/MythicPlayerState.h"
#include "World/LivingWorld/MythicWorldStateSubsystem.h"

UMythicQuestJournalComponent::UMythicQuestJournalComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

FText UMythicQuestJournalComponent::FindQuestTitleForTask(const UObjectiveDefinition *Task) const {
    if (!Task) {
        return FText::GetEmpty();
    }
    for (const FMythicQuestJournalEntry &Entry : Quests) {
        if (Entry.Quest && Entry.Quest->Tasks.Contains(Task)) {
            return Entry.Quest->JournalTitle;
        }
    }
    return FText::GetEmpty();
}

void UMythicQuestJournalComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicQuestJournalComponent, Quests, COND_OwnerOnly);
}

void UMythicQuestJournalComponent::BeginPlay() {
    Super::BeginPlay();
    EnsureBoundToTracker();
}

void UMythicQuestJournalComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (bBoundToTracker) {
        if (UObjectiveTracker *Tracker = ResolveTracker()) {
            Tracker->OnObjectivesChanged.RemoveDynamic(this, &UMythicQuestJournalComponent::HandleObjectivesChanged);
        }
        bBoundToTracker = false;
    }
    Super::EndPlay(EndPlayReason);
}


EMythicQuestState UMythicQuestJournalComponent::DeriveQuestState(const TArray<EMythicTaskState> &TaskStates,
                                                                 const TArray<bool> &TaskRequiredMask,
                                                                 bool bExclusiveLockTripped) {
    bool bAnyRequiredFailed = false;
    bool bAllRequiredComplete = true;
    bool bAnyStarted = false;
    int32 NumRequired = 0;

    for (int32 i = 0; i < TaskStates.Num(); ++i) {
        const EMythicTaskState S = TaskStates[i];
        if (S != EMythicTaskState::NotStarted) {
            bAnyStarted = true;
        }
        const bool bRequired = TaskRequiredMask.IsValidIndex(i) ? TaskRequiredMask[i] : true;
        if (bRequired) {
            ++NumRequired;
            if (S == EMythicTaskState::Failed) {
                bAnyRequiredFailed = true;
            }
            if (S != EMythicTaskState::Complete) {
                bAllRequiredComplete = false;
            }
        }
    }

    if (bExclusiveLockTripped || bAnyRequiredFailed) {
        return EMythicQuestState::Failed;
    }
    if (NumRequired > 0 && bAllRequiredComplete) {
        return EMythicQuestState::Completed;
    }
    return bAnyStarted ? EMythicQuestState::Active : EMythicQuestState::NotStarted;
}


void UMythicQuestJournalComponent::ServerStartQuest(UMythicQuestDefinition *Quest) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Quest) {
        return;
    }
    EnsureBoundToTracker();
    if (StartQuestInternal(Quest)) {
        RecomputeQuests();
    }
}

void UMythicQuestJournalComponent::ServerStartStoryline(UMythicStorylineDefinition *Storyline) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority() || !Storyline) {
        return;
    }
    EnsureBoundToTracker();
    const FGameplayTagContainer Owned = GatherOwnedTags();
    if (!FMythicStoryCondition::Evaluate(Storyline->ArcGate, Owned)) {
        return;
    }
    ActiveStorylines.AddUnique(Storyline);
    for (const TObjectPtr<UMythicQuestDefinition> &Q : Storyline->Quests) {
        if (Q && !FindEntry(Q)) {
            if (StartQuestInternal(Q.Get())) {
                break;
            }
        }
    }
    RecomputeQuests();
}

bool UMythicQuestJournalComponent::StartQuestInternal(UMythicQuestDefinition *Quest) {
    if (!Quest || FindEntry(Quest)) {
        return false;
    }
    const FGameplayTagContainer Owned = GatherOwnedTags();
    if (!FMythicStoryCondition::Evaluate(Quest->UnlockConditions, Owned)) {
        return false;
    }
    FMythicQuestJournalEntry Entry;
    Entry.Quest = Quest;
    Entry.State = EMythicQuestState::NotStarted;
    Quests.Add(Entry);

    if (UObjectiveTracker *Tracker = ResolveTracker()) {
        for (const TObjectPtr<UObjectiveDefinition> &Task : Quest->Tasks) {
            if (Task) {
                Tracker->ServerAddObjective(Task.Get());
            }
        }
    }
    UE_LOG(Myth, Log, TEXT("QuestJournal: started quest '%s' (%d tasks)"), *GetNameSafe(Quest), Quest->Tasks.Num());
    return true;
}


void UMythicQuestJournalComponent::HandleObjectivesChanged() {
    RecomputeQuests();
}

void UMythicQuestJournalComponent::RecomputeQuests() {
    const AActor *Owner = GetOwner();
    if (bRecomputing || !Owner || !Owner->HasAuthority()) {
        return;
    }
    bRecomputing = true;

    bool bChangedAny = true;
    int32 Safety = 0;
    while (bChangedAny && Safety++ < 64) {
        bChangedAny = false;
        TArray<UMythicQuestDefinition *> NewlyCompleted;

        for (int32 i = 0; i < Quests.Num(); ++i) {
            FMythicQuestJournalEntry &Entry = Quests[i];
            if (!Entry.Quest || Entry.State == EMythicQuestState::Completed || Entry.State == EMythicQuestState::Failed) {
                continue;
            }
            const EMythicQuestState NewState = ComputeQuestState(Entry.Quest);
            if (NewState != Entry.State) {
                Entry.State = NewState;
                bChangedAny = true;
            }
            if (NewState == EMythicQuestState::Completed) {
                NewlyCompleted.Add(Entry.Quest);
            }
        }

        for (UMythicQuestDefinition *Q : NewlyCompleted) {
            ApplyQuestCompleted(Q);
            bChangedAny = true;
        }
    }

    bRecomputing = false;

    OnQuestsChanged.Broadcast();
}

void UMythicQuestJournalComponent::OnRep_Quests() {
    OnQuestsChanged.Broadcast();
}

EMythicQuestState UMythicQuestJournalComponent::ComputeQuestState(const UMythicQuestDefinition *Quest) const {
    if (!Quest) {
        return EMythicQuestState::NotStarted;
    }
    const UObjectiveTracker *Tracker = ResolveTracker();
    const FGameplayTagContainer Owned = GatherOwnedTags();
    const bool bLock = !Quest->ExclusiveLockTags.IsEmpty() && Owned.HasAny(Quest->ExclusiveLockTags);

    TArray<EMythicTaskState> TaskStates;
    TArray<bool> RequiredMask;
    TaskStates.Reserve(Quest->Tasks.Num());
    RequiredMask.Reserve(Quest->Tasks.Num());
    for (const TObjectPtr<UObjectiveDefinition> &Task : Quest->Tasks) {
        if (!Task) {
            continue;
        }
        EMythicTaskState S = EMythicTaskState::NotStarted;
        FObjectiveProgress Prog;
        if (Tracker && Tracker->FindObjectiveProgress(Task.Get(), Prog)) {
            S = Prog.bCompleted ? EMythicTaskState::Complete : EMythicTaskState::Active;
        }
        TaskStates.Add(S);
        RequiredMask.Add(!Task->bOptional);
    }
    return DeriveQuestState(TaskStates, RequiredMask, bLock);
}

void UMythicQuestJournalComponent::ApplyQuestCompleted(UMythicQuestDefinition *Quest) {
    if (!Quest) {
        return;
    }
    APlayerController *PC = ResolvePlayerController();
    UMythicNarrativeStateComponent *Ledger = ResolveLedger();

    if (PC) {
        Quest->Rewards.Give(PC);
    }
    FMythicNarrativeGrant::RouteGrants(this, Ledger, Quest->GrantStoryTagsOnComplete);

    const FGameplayTagContainer Owned = GatherOwnedTags();
    const int32 OutcomeIdx = FMythicQuestOutcome::ResolveQuestOutcome(Quest->Outcomes, Owned);
    if (Quest->Outcomes.IsValidIndex(OutcomeIdx)) {
        const FMythicQuestOutcome &Outcome = Quest->Outcomes[OutcomeIdx];
        if (PC) {
            Outcome.Rewards.Give(PC);
        }
        FMythicNarrativeGrant::RouteGrants(this, Ledger, Outcome.GrantStoryTags);
        UE_LOG(Myth, Log, TEXT("QuestJournal: quest '%s' completed → outcome[%d] '%s'"), *GetNameSafe(Quest),
               OutcomeIdx, *Outcome.OutcomeTag.ToString());
    }
    else {
        UE_LOG(Myth, Log, TEXT("QuestJournal: quest '%s' completed (no outcome matched)"), *GetNameSafe(Quest));
    }

    AdvanceStorylines(Quest);
}

void UMythicQuestJournalComponent::AdvanceStorylines(UMythicQuestDefinition *JustCompleted) {
    for (const TObjectPtr<UMythicStorylineDefinition> &Arc : ActiveStorylines) {
        if (!Arc || !Arc->Quests.Contains(JustCompleted)) {
            continue;
        }
        for (const TObjectPtr<UMythicQuestDefinition> &Q : Arc->Quests) {
            if (Q && !FindEntry(Q)) {
                if (StartQuestInternal(Q.Get())) {
                    break;
                }
            }
        }
        if (!CompletedStorylines.Contains(Arc) && IsArcComplete(Arc.Get())) {
            GrantArcRewards(Arc.Get());
            CompletedStorylines.Add(Arc);
        }
    }
}

bool UMythicQuestJournalComponent::IsArcComplete(const UMythicStorylineDefinition *Arc) const {
    if (!Arc) {
        return false;
    }
    bool bAnyRequired = false;
    for (const TObjectPtr<UMythicQuestDefinition> &Q : Arc->Quests) {
        if (!Q || Q->bIsOptional) {
            continue;
        }
        bAnyRequired = true;
        const FMythicQuestJournalEntry *E = FindEntry(Q.Get());
        if (!E || E->State != EMythicQuestState::Completed) {
            return false;
        }
    }
    return bAnyRequired;
}

void UMythicQuestJournalComponent::GrantArcRewards(const UMythicStorylineDefinition *Arc) {
    if (!Arc) {
        return;
    }
    if (APlayerController *PC = ResolvePlayerController()) {
        Arc->Rewards.Give(PC);
    }
    FMythicNarrativeGrant::RouteGrants(this, ResolveLedger(), Arc->GrantStoryTagsOnComplete);
    UE_LOG(Myth, Log, TEXT("QuestJournal: storyline '%s' arc completed — arc rewards granted"), *GetNameSafe(Arc));
}


FSerializedQuestJournalEntry UMythicQuestJournalComponent::MakeSerializedEntry(const FMythicQuestJournalEntry &Entry) {
    FSerializedQuestJournalEntry Data;
    Data.QuestPath = Entry.Quest ? FSoftObjectPath(Entry.Quest) : FSoftObjectPath();
    Data.State = static_cast<uint8>(Entry.State);
    return Data;
}

void UMythicQuestJournalComponent::GetSerializableJournal(TArray<FSerializedQuestJournalEntry> &OutQuests,
                                                          TArray<FSoftObjectPath> &OutActiveStorylines,
                                                          TArray<FSoftObjectPath> &OutCompletedStorylines) const {
    OutQuests.Reset();
    for (const FMythicQuestJournalEntry &E : Quests) {
        if (!E.Quest) {
            continue;
        }
        OutQuests.Add(MakeSerializedEntry(E));
    }
    OutActiveStorylines.Reset();
    for (const TObjectPtr<UMythicStorylineDefinition> &Arc : ActiveStorylines) {
        if (Arc) {
            OutActiveStorylines.Add(FSoftObjectPath(Arc.Get()));
        }
    }
    OutCompletedStorylines.Reset();
    for (const TObjectPtr<UMythicStorylineDefinition> &Arc : CompletedStorylines) {
        if (Arc) {
            OutCompletedStorylines.Add(FSoftObjectPath(Arc.Get()));
        }
    }
}

void UMythicQuestJournalComponent::RestoreQuests(const TArray<FSerializedQuestJournalEntry> &InQuests,
                                                 const TArray<FSoftObjectPath> &InActiveStorylines,
                                                 const TArray<FSoftObjectPath> &InCompletedStorylines) {
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (InQuests.Num() == 0 && InActiveStorylines.Num() == 0 && InCompletedStorylines.Num() == 0) {
        return;
    }

    Quests.Reset();
    for (const FSerializedQuestJournalEntry &Data : InQuests) {
        UMythicQuestDefinition *Def = Cast<UMythicQuestDefinition>(Data.QuestPath.TryLoad());
        if (!Def) {
            UE_LOG(Myth, Warning, TEXT("QuestJournal::RestoreQuests: failed to load quest asset %s; skipped."),
                   *Data.QuestPath.ToString());
            continue;
        }
        FMythicQuestJournalEntry Entry;
        Entry.Quest = Def;
        Entry.State = static_cast<EMythicQuestState>(Data.State);
        Quests.Add(Entry);
    }

    ActiveStorylines.Reset();
    for (const FSoftObjectPath &Path : InActiveStorylines) {
        if (UMythicStorylineDefinition *Arc = Cast<UMythicStorylineDefinition>(Path.TryLoad())) {
            ActiveStorylines.AddUnique(Arc);
        }
    }
    CompletedStorylines.Reset();
    for (const FSoftObjectPath &Path : InCompletedStorylines) {
        if (UMythicStorylineDefinition *Arc = Cast<UMythicStorylineDefinition>(Path.TryLoad())) {
            CompletedStorylines.Add(Arc);
        }
    }

    EnsureBoundToTracker();

    UE_LOG(Myth, Log, TEXT("QuestJournal::RestoreQuests: restored %d quest(s), %d active / %d completed storyline(s) on %s."),
           Quests.Num(), ActiveStorylines.Num(), CompletedStorylines.Num(), *GetNameSafe(GetOwner()));
}


EMythicQuestState UMythicQuestJournalComponent::GetQuestState(const UMythicQuestDefinition *Quest) const {
    if (const FMythicQuestJournalEntry *E = FindEntry(Quest)) {
        return E->State;
    }
    return EMythicQuestState::NotStarted;
}

const FMythicQuestJournalEntry *UMythicQuestJournalComponent::FindEntry(const UMythicQuestDefinition *Quest) const {
    if (!Quest) {
        return nullptr;
    }
    for (const FMythicQuestJournalEntry &E : Quests) {
        if (E.Quest == Quest) {
            return &E;
        }
    }
    return nullptr;
}

void UMythicQuestJournalComponent::EnsureBoundToTracker() {
    if (bBoundToTracker) {
        return;
    }
    const AActor *Owner = GetOwner();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }
    if (UObjectiveTracker *Tracker = ResolveTracker()) {
        Tracker->OnObjectivesChanged.AddDynamic(this, &UMythicQuestJournalComponent::HandleObjectivesChanged);
        bBoundToTracker = true;
    }
}

APlayerController *UMythicQuestJournalComponent::ResolvePlayerController() const {
    if (const APlayerState *PS = Cast<APlayerState>(GetOwner())) {
        if (APlayerController *PC = PS->GetPlayerController()) {
            return PC;
        }
        return Cast<APlayerController>(PS->GetOwner());
    }
    return Cast<APlayerController>(GetOwner());
}

UMythicNarrativeStateComponent *UMythicQuestJournalComponent::ResolveLedger() const {
    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        return PS->GetNarrativeState();
    }
    if (const APlayerController *PC = ResolvePlayerController()) {
        if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(PC->PlayerState)) {
            return PS->GetNarrativeState();
        }
    }
    return nullptr;
}

UObjectiveTracker *UMythicQuestJournalComponent::ResolveTracker() const {
    if (const APlayerController *PC = ResolvePlayerController()) {
        return PC->FindComponentByClass<UObjectiveTracker>();
    }
    return nullptr;
}

FGameplayTagContainer UMythicQuestJournalComponent::GatherOwnedTags() const {
    FGameplayTagContainer Owned;
    if (const UMythicNarrativeStateComponent *Ledger = ResolveLedger()) {
        Owned.AppendTags(Ledger->GetOwnedTags());
    }
    if (const UWorld *World = GetWorld()) {
        if (const UMythicWorldStateSubsystem *WorldState = World->GetSubsystem<UMythicWorldStateSubsystem>()) {
            Owned.AppendTags(WorldState->GetWorldFlags());
        }
    }
    return Owned;
}
