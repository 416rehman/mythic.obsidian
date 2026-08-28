#include "Objectives/ObjectiveTracker.h"

#include "Objectives/ObjectiveDefinition.h"
#include "Mythic.h"
#include "GAS/MythicTags_GAS.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Player/MythicPlayerState.h"
#include "Narrative/MythicNarrativeStateComponent.h"
#include "Narrative/MythicNarrativeGrant.h"
#include "World/LivingWorld/MythicWorldStateSubsystem.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "Narrative/MythicQuestJournalComponent.h"
#include "Narrative/MythicQuestDefinition.h"

namespace {
FText QuestTitleForTask(const APlayerController *PC, const UObjectiveDefinition *Task) {
    const AMythicPlayerState *PS = PC ? Cast<AMythicPlayerState>(PC->PlayerState) : nullptr;
    const UMythicQuestJournalComponent *Journal = PS ? PS->GetQuestJournal() : nullptr;
    return (Journal && Task) ? Journal->FindQuestTitleForTask(Task) : FText::GetEmpty();
}
}

UObjectiveTracker::UObjectiveTracker() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UObjectiveTracker::BeginPlay() {
    Super::BeginPlay();

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner());
    UAbilitySystemComponent *ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
    if (!ASC) {
        UE_LOG(Myth, Warning, TEXT("ObjectiveTracker: no ASC on %s at BeginPlay; kill objectives won't advance."),
               *GetNameSafe(GetOwner()));
        return;
    }

    BoundASC = ASC;
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.Definition && !Prog.Definition->IsHarvestObjective()) {
            EnsureSubscribedToTag(Prog.Definition->TriggerEventTag);
        }
    }
}

void UObjectiveTracker::EnsureSubscribedToTag(const FGameplayTag &Tag) {
    if (!BoundASC || !Tag.IsValid() || BoundEventHandles.Contains(Tag)) {
        return;
    }
    const FDelegateHandle Handle = BoundASC->GenericGameplayEventCallbacks.FindOrAdd(Tag).AddUObject(
        this, &UObjectiveTracker::HandleGameplayEvent);
    BoundEventHandles.Add(Tag, Handle);
}

void UObjectiveTracker::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (BoundASC) {
        for (const TPair<FGameplayTag, FDelegateHandle> &Pair : BoundEventHandles) {
            if (Pair.Value.IsValid()) {
                BoundASC->GenericGameplayEventCallbacks.FindOrAdd(Pair.Key).Remove(Pair.Value);
            }
        }
    }
    BoundEventHandles.Empty();
    BoundASC = nullptr;

    Super::EndPlay(EndPlayReason);
}

void UObjectiveTracker::ComputeObjectiveProgress(int32 CurrentCount, bool bCountByMagnitude, float EventMagnitude,
                                                 int32 RequiredCount, int32 &OutNewCount, bool &OutJustCompleted) {
    const int32 Advance = bCountByMagnitude ? FMath::Max(1, FMath::RoundToInt(EventMagnitude)) : 1;
    int32 NewCount = CurrentCount + Advance;
    OutJustCompleted = (NewCount >= RequiredCount);
    if (OutJustCompleted) {
        NewCount = RequiredCount;
    }
    OutNewCount = NewCount;
}

EObjectiveOfferResult UObjectiveTracker::ResolveObjectiveOfferResult(const TArray<FObjectiveProgress> &TrackedObjectives,
                                                                     const UObjectiveDefinition *Definition,
                                                                     FObjectiveProgress &OutProgress,
                                                                     const FGameplayTagContainer &OwnedStoryTags) {
    OutProgress = FObjectiveProgress();
    if (!Definition || !Definition->TriggerEventTag.IsValid() || Definition->RequiredCount <= 0) {
        return EObjectiveOfferResult::Invalid;
    }

    for (const FObjectiveProgress &Prog : TrackedObjectives) {
        if (Prog.Definition == Definition) {
            OutProgress = Prog;
            return Prog.bCompleted ? EObjectiveOfferResult::AlreadyCompleted : EObjectiveOfferResult::AlreadyActive;
        }
    }

    if (!AreObjectivePrerequisitesMet(Definition->PrerequisiteObjectives, TrackedObjectives)) {
        return EObjectiveOfferResult::PrerequisitesNotMet;
    }

    if (!FMythicStoryCondition::Evaluate(Definition->Precondition, OwnedStoryTags)) {
        return EObjectiveOfferResult::PreconditionNotMet;
    }

    OutProgress.Definition = const_cast<UObjectiveDefinition *>(Definition);
    return EObjectiveOfferResult::Assigned;
}

bool UObjectiveTracker::AreObjectivePrerequisitesMet(const TArray<TObjectPtr<UObjectiveDefinition>> &Prerequisites,
                                                     const TArray<FObjectiveProgress> &TrackedObjectives) {
    for (const TObjectPtr<UObjectiveDefinition> &Prereq : Prerequisites) {
        if (!Prereq) {
            continue;
        }
        bool bPrereqCompleted = false;
        for (const FObjectiveProgress &Prog : TrackedObjectives) {
            if (Prog.Definition == Prereq && Prog.bCompleted) {
                bPrereqCompleted = true;
                break;
            }
        }
        if (!bPrereqCompleted) {
            return false;
        }
    }
    return true;
}

void UObjectiveTracker::CollectAssignableNextObjectives(const TArray<TObjectPtr<UObjectiveDefinition>> &CandidateNext,
                                                        const TArray<FObjectiveProgress> &TrackedObjectives,
                                                        TArray<UObjectiveDefinition *> &OutAssignable,
                                                        const FGameplayTagContainer &OwnedStoryTags) {
    for (const TObjectPtr<UObjectiveDefinition> &Next : CandidateNext) {
        if (!Next) {
            continue;
        }
        FObjectiveProgress Scratch;
        if (ResolveObjectiveOfferResult(TrackedObjectives, Next, Scratch, OwnedStoryTags) == EObjectiveOfferResult::Assigned) {
            OutAssignable.AddUnique(Next);
        }
    }
}

FGameplayTag UObjectiveTracker::DeriveAchievedOutcome(const UObjectiveDefinition *Def, const FGameplayTag &CompletingEventTag,
                                                      const FGameplayTagContainer &CompletingPayloadTags) {
    if (!Def) {
        return FGameplayTag();
    }
    for (const FMythicObjectiveBranch &Branch : Def->OutcomeBranches) {
        const FGameplayTag &OutcomeTag = Branch.OutcomeTag;
        if (!OutcomeTag.IsValid()) {
            continue;
        }
        if (CompletingEventTag.MatchesTag(OutcomeTag) || CompletingPayloadTags.HasTag(OutcomeTag)) {
            return OutcomeTag;
        }
    }
    return FGameplayTag();
}

EMythicObjectiveOutcome UObjectiveTracker::ClassifyOutcome(const FGameplayTag &CompletingEventTag) {
    if (CompletingEventTag.MatchesTag(GAS_EVENT_KILL)) {
        return EMythicObjectiveOutcome::Killed;
    }
    if (CompletingEventTag.MatchesTag(GAS_EVENT_TALKED_TO_NPC)) {
        return EMythicObjectiveOutcome::Spared;
    }
    return EMythicObjectiveOutcome::Completed;
}

FMythicObjectiveBranchResult UObjectiveTracker::SelectBranchForOutcome(const TArray<FMythicObjectiveBranch> &Branches,
                                                                       FGameplayTag AchievedOutcome,
                                                                       const TArray<FObjectiveProgress> &TrackedObjectives,
                                                                       const FGameplayTagContainer &OwnedStoryTags) {
    FMythicObjectiveBranchResult Result;
    if (!AchievedOutcome.IsValid()) {
        return Result;
    }
    for (const FMythicObjectiveBranch &Branch : Branches) {
        if (!Branch.OutcomeTag.IsValid() || Branch.OutcomeTag != AchievedOutcome) {
            continue;
        }
        Result.bMatched = true;
        CollectAssignableNextObjectives(Branch.NextObjectives, TrackedObjectives, Result.Assignable, OwnedStoryTags);
        Result.GrantStoryTags = Branch.GrantStoryTags;
        for (const TObjectPtr<UObjectiveDefinition> &Sibling : Branch.CancelSiblings) {
            if (Sibling) {
                Result.CancelSiblings.AddUnique(Sibling.Get());
            }
        }
        return Result;
    }
    return Result;
}

UMythicNarrativeStateComponent *UObjectiveTracker::ResolveNarrativeComponent() const {
    const APlayerController *PC = Cast<APlayerController>(GetOwner());
    const AMythicPlayerState *PS = PC ? Cast<AMythicPlayerState>(PC->PlayerState) : nullptr;
    return PS ? PS->GetNarrativeState() : nullptr;
}

FGameplayTagContainer UObjectiveTracker::GatherOwnedStoryTags() const {
    FGameplayTagContainer Owned;
    if (const UMythicNarrativeStateComponent *Narrative = ResolveNarrativeComponent()) {
        Owned.AppendTags(Narrative->GetOwnedTags());
    }
    if (const UWorld *World = GetWorld()) {
        if (const UMythicWorldStateSubsystem *WorldState = World->GetSubsystem<UMythicWorldStateSubsystem>()) {
            Owned.AppendTags(WorldState->GetWorldFlags());
        }
    }
    return Owned;
}

FText UObjectiveTracker::BuildObjectiveNotificationText(const FText &DisplayText, EObjectiveNotifyCategory Category,
                                                        EObjectiveOfferResult OfferResult, int32 Current, int32 Required,
                                                        bool bRewardSucceeded, bool bRewardDroppedNearby) {
    const FString Text = DisplayText.ToString();
    switch (Category) {
    case EObjectiveNotifyCategory::Assignment:
        if (OfferResult == EObjectiveOfferResult::OutOfRange) {
            return FText::FromString(FString::Printf(TEXT("Objective Out of Range: %s"), *Text));
        }
        if (OfferResult == EObjectiveOfferResult::PrerequisitesNotMet ||
            OfferResult == EObjectiveOfferResult::PreconditionNotMet) {
            return FText::FromString(FString::Printf(TEXT("Objective Locked: %s"), *Text));
        }
        if (OfferResult == EObjectiveOfferResult::Invalid) {
            return FText::FromString(FString::Printf(TEXT("Objective Unavailable: %s"), *Text));
        }
        return FText::FromString(FString::Printf(TEXT("Objective Assigned: %s"), *Text));
    case EObjectiveNotifyCategory::Duplicate:
        if (OfferResult == EObjectiveOfferResult::AlreadyCompleted) {
            return FText::FromString(FString::Printf(TEXT("Objective Already Completed: %s"), *Text));
        }
        return FText::FromString(FString::Printf(TEXT("Objective Already Active: %s %d/%d"), *Text, Current, Required));
    case EObjectiveNotifyCategory::Progress:
        return FText::FromString(FString::Printf(TEXT("%s %d/%d"), *Text, Current, Required));
    case EObjectiveNotifyCategory::Completed:
        return FText::FromString(FString::Printf(TEXT("Objective Complete: %s"), *Text));
    case EObjectiveNotifyCategory::RewardResult:
        if (!bRewardSucceeded) {
            return FText::FromString(TEXT("Reward Delivery Failed"));
        }
        return FText::FromString(bRewardDroppedNearby ? TEXT("Reward Dropped Nearby") : TEXT("Rewards Granted"));
    default:
        return DisplayText;
    }
}

void UObjectiveTracker::HandleGameplayEvent(const FGameplayEventData *Payload) {
    if (!Payload || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    APlayerController *PC = Cast<APlayerController>(GetOwner());
    if (!PC) {
        return;
    }

    int32 NotifyIndex = 0;
    TArray<FMythicPendingObjectiveCompletion> PendingCompletions;
    for (FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.bCompleted || !Prog.Definition
            || Prog.Definition->IsHarvestObjective()) {
            continue;
        }
        if (Prog.Definition->TriggerEventTag != Payload->EventTag) {
            continue;
        }
        if (Prog.Definition->IsDeliveryObjective()) {
            continue;
        }
        const FGameplayTag &RequiredTag = Prog.Definition->RequiredPayloadTag;
        if (RequiredTag.IsValid() && !Payload->TargetTags.HasTag(RequiredTag)) {
            continue;
        }

        AdvanceObjectiveProgress(Prog, Prog.Definition->bCountByEventMagnitude, Payload->EventMagnitude, PC,
                                 Payload->EventTag, Payload->TargetTags, PendingCompletions, NotifyIndex);
    }

    const bool bAnyCompleted = PendingCompletions.Num() > 0;
    ProcessChainAdvance(PC, PendingCompletions, NotifyIndex);
    if (bAnyCompleted) {
        OnObjectivesChanged.Broadcast();
    }
}

void UObjectiveTracker::ApplySharedKillCredit(const FGameplayEventData &Payload) {
    HandleGameplayEvent(&Payload);
}

bool UObjectiveTracker::MatchesHarvestableDefinition(
    const UObjectiveDefinition *Objective,
    const UMythicHarvestableDefinition *HarvestableDefinition) {
    return Objective && HarvestableDefinition
        && Objective->RequiredHarvestableDefinition == HarvestableDefinition;
}

void UObjectiveTracker::ApplyHarvestCompletionCredit(
    const UMythicHarvestableDefinition &HarvestableDefinition,
    const int32 CreditCount) {
    ConsumeHarvestCompletionCredit(HarvestableDefinition, CreditCount);
}

EMythicHarvestQuestCreditConsumeResult
UObjectiveTracker::ConsumeHarvestCompletionCredit(
    const UMythicHarvestableDefinition &HarvestableDefinition,
    const int32 CreditCount) {
    if (CreditCount <= 0 || !GetOwner() || !GetOwner()->HasAuthority()) {
        return EMythicHarvestQuestCreditConsumeResult::Rejected;
    }
    APlayerController *PC = Cast<APlayerController>(GetOwner());
    if (!PC) {
        return EMythicHarvestQuestCreditConsumeResult::Rejected;
    }

    int32 NotifyIndex = 0;
    TArray<FMythicPendingObjectiveCompletion> PendingCompletions;
    bool bAdvancedAny = false;
    for (FObjectiveProgress &Progress : ActiveObjectives) {
        if (Progress.bCompleted
            || !MatchesHarvestableDefinition(
                Progress.Definition, &HarvestableDefinition)) {
            continue;
        }
        bAdvancedAny = true;
        AdvanceObjectiveProgress(
            Progress, true, static_cast<float>(CreditCount), PC,
            FGameplayTag(), FGameplayTagContainer(), PendingCompletions,
            NotifyIndex);
    }

    ProcessChainAdvance(PC, PendingCompletions, NotifyIndex);
    if (bAdvancedAny) {
        OnObjectivesChanged.Broadcast();
    }
    return bAdvancedAny
        ? EMythicHarvestQuestCreditConsumeResult::ConsumedMatched
        : EMythicHarvestQuestCreditConsumeResult::ConsumedNoMatch;
}

void UObjectiveTracker::AdvanceObjectiveProgress(FObjectiveProgress &Prog, bool bCountByMagnitude, float Magnitude,
                                                 APlayerController *PC, const FGameplayTag &CompletingEventTag,
                                                 const FGameplayTagContainer &CompletingPayloadTags,
                                                 TArray<FMythicPendingObjectiveCompletion> &PendingCompletions,
                                                 int32 &NotifyIndex) {
    if (!Prog.Definition) {
        return;
    }
    int32 NewCount = Prog.CurrentCount;
    bool bJustCompleted = false;
    ComputeObjectiveProgress(Prog.CurrentCount, bCountByMagnitude, Magnitude, Prog.Definition->RequiredCount, NewCount,
                             bJustCompleted);
    Prog.CurrentCount = NewCount;
    if (bJustCompleted) {
        Prog.bCompleted = true;
        Prog.CompletedTimeSeconds = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        if (!Prog.Definition->GrantStoryTagsOnComplete.IsEmpty()) {
            FMythicNarrativeGrant::RouteGrants(this, ResolveNarrativeComponent(), Prog.Definition->GrantStoryTagsOnComplete);
        }
        FMythicPendingObjectiveCompletion Pending;
        Pending.Definition = Prog.Definition;
        Pending.AchievedOutcome = DeriveAchievedOutcome(Prog.Definition, CompletingEventTag, CompletingPayloadTags);
        PendingCompletions.Add(Pending);
        UE_LOG(Myth, Verbose, TEXT("ObjectiveTracker: '%s' completed as outcome=%d (achievedTag=%s)"),
               *Prog.Definition->DisplayText.ToString(), static_cast<int32>(ClassifyOutcome(CompletingEventTag)),
               *Pending.AchievedOutcome.ToString());
        const bool bRewardSucceeded = Prog.Definition->Rewards.Give(PC);
        UE_LOG(Myth, Log, TEXT("ObjectiveTracker: objective '%s' completed (%d/%d); rewards granted to %s."),
               *Prog.Definition->DisplayText.ToString(), Prog.CurrentCount, Prog.Definition->RequiredCount,
               *GetNameSafe(PC));
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
            MythicPC->ClientNotifyObjectiveResult(Prog.Definition->GetCalloutText(true),
                                                  EObjectiveNotifyCategory::RewardResult,
                                                  EObjectiveOfferResult::Assigned,
                                                  Prog.CurrentCount, Prog.Definition->RequiredCount,
                                                  bRewardSucceeded, false, NotifyIndex);
        }
    }
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        MythicPC->ClientNotifyObjective(Prog.Definition->GetCalloutText(bJustCompleted), Prog.CurrentCount,
                                        Prog.Definition->RequiredCount, bJustCompleted, NotifyIndex++,
                                        QuestTitleForTask(PC, Prog.Definition));
    }
}

void UObjectiveTracker::ProcessChainAdvance(APlayerController *PC,
                                            TArray<FMythicPendingObjectiveCompletion> &PendingCompletions,
                                            int32 &NotifyIndex) {
    if (PendingCompletions.Num() == 0) {
        return;
    }
    AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC);
    UMythicNarrativeStateComponent *Narrative = ResolveNarrativeComponent();

    for (const FMythicPendingObjectiveCompletion &Pending : PendingCompletions) {
        UObjectiveDefinition *Def = Pending.Definition;
        if (!Def) {
            continue;
        }
        const FGameplayTagContainer Owned = GatherOwnedStoryTags();

        const FMythicObjectiveBranchResult Branch =
            SelectBranchForOutcome(Def->OutcomeBranches, Pending.AchievedOutcome, ActiveObjectives, Owned);

        TArray<UObjectiveDefinition *> ToAssign;
        if (Branch.bMatched) {
            ToAssign = Branch.Assignable;
            FMythicNarrativeGrant::RouteGrants(this, Narrative, Branch.GrantStoryTags);
            for (UObjectiveDefinition *Sibling : Branch.CancelSiblings) {
                ServerAbandonObjective_Implementation(Sibling);
            }
        }
        else {
            CollectAssignableNextObjectives(Def->NextObjectives, ActiveObjectives, ToAssign, Owned);
        }

        for (UObjectiveDefinition *Next : ToAssign) {
            FObjectiveProgress OutProg;
            if (ServerTryAddObjective(Next, OutProg) == EObjectiveOfferResult::Assigned && MythicPC) {
                MythicPC->ClientNotifyObjective(Next->GetCalloutText(false), OutProg.CurrentCount, Next->RequiredCount,
 false, NotifyIndex++, QuestTitleForTask(PC, Next));
            }
        }
    }
}

int32 UObjectiveTracker::ComputeDeliverConsumeCount(int32 CurrentCount, int32 RequiredCount, int32 Available) {
    return FMath::Clamp(RequiredCount - CurrentCount, 0, FMath::Max(0, Available));
}

void UObjectiveTracker::ServerTurnInDeliveriesTo(const FGameplayTag &NpcTag, UMythicInventoryComponent *PlayerInventory) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !PlayerInventory || !NpcTag.IsValid()) {
        return;
    }
    APlayerController *PC = Cast<APlayerController>(GetOwner());
    if (!PC) {
        return;
    }

    int32 NotifyIndex = 0;
    TArray<FMythicPendingObjectiveCompletion> PendingCompletions;
    for (FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.bCompleted || !Prog.Definition || !Prog.Definition->IsDeliveryObjective()) {
            continue;
        }
        if (!NpcTag.MatchesTag(Prog.Definition->DeliverToNpcTag)) {
            continue;
        }
        UItemDefinition *Wanted = Prog.Definition->DeliverItem;
        const int32 Available = PlayerInventory->GetItemCount(Wanted);
        const int32 Consume = ComputeDeliverConsumeCount(Prog.CurrentCount, Prog.Definition->RequiredCount, Available);
        if (Consume <= 0) {
            continue;
        }
        PlayerInventory->ServerRemoveItemByDefinition(Wanted, Consume);
        AdvanceObjectiveProgress(Prog, true, static_cast<float>(Consume), PC, FGameplayTag(),
                                 FGameplayTagContainer(), PendingCompletions, NotifyIndex);
    }

    const bool bAnyCompleted = PendingCompletions.Num() > 0;
    ProcessChainAdvance(PC, PendingCompletions, NotifyIndex);
    if (bAnyCompleted) {
        OnObjectivesChanged.Broadcast();
    }
}

bool UObjectiveTracker::HasObjective(const UObjectiveDefinition *Definition) const {
    if (!Definition) {
        return false;
    }
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.Definition == Definition) {
            return true;
        }
    }
    return false;
}

bool UObjectiveTracker::FindObjectiveProgress(const UObjectiveDefinition *Def, FObjectiveProgress &OutProgress) const {
    if (!Def) {
        return false;
    }
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.Definition == Def) {
            OutProgress = Prog;
            return true;
        }
    }
    return false;
}

void UObjectiveTracker::SaveObjectives(TArray<FSerializedObjectiveData> &OutData) const {
    OutData.Reset();
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (!Prog.Definition) {
            continue;
        }
        FSerializedObjectiveData Data;
        Data.ObjectiveAsset = FSoftObjectPath(Prog.Definition);
        Data.CurrentCount = Prog.CurrentCount;
        Data.bCompleted = Prog.bCompleted;
        OutData.Add(Data);
    }
}

void UObjectiveTracker::RestoreObjectives(const TArray<FSerializedObjectiveData> &InData) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    ActiveObjectives.Reset();
    for (const FSerializedObjectiveData &Data : InData) {
        UObjectiveDefinition *Def = Cast<UObjectiveDefinition>(Data.ObjectiveAsset.TryLoad());
        if (!Def) {
            UE_LOG(Myth, Warning, TEXT("ObjectiveTracker::RestoreObjectives: failed to load objective asset %s; skipped."),
                   *Data.ObjectiveAsset.ToString());
            continue;
        }
        FObjectiveProgress Prog;
        Prog.Definition = Def;
        Prog.CurrentCount = Data.CurrentCount;
        Prog.bCompleted = Data.bCompleted;
        ActiveObjectives.Add(Prog);

        if (!Def->IsHarvestObjective()) {
            EnsureSubscribedToTag(Def->TriggerEventTag);
        }
    }
    UE_LOG(Myth, Log, TEXT("ObjectiveTracker::RestoreObjectives: restored %d objective(s) on %s."),
           ActiveObjectives.Num(), *GetNameSafe(GetOwner()));
}

void UObjectiveTracker::ServerAddObjective(UObjectiveDefinition *Definition) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Definition) {
        return;
    }
    if (HasObjective(Definition)) {
        return;
    }
    FObjectiveProgress Prog;
    Prog.Definition = Definition;
    ActiveObjectives.Add(Prog);

    if (!Definition->IsHarvestObjective()) {
        EnsureSubscribedToTag(Definition->TriggerEventTag);
    }

    UE_LOG(Myth, Log, TEXT("ObjectiveTracker: assigned objective '%s' (need %d x %s) to %s."),
           *Definition->DisplayText.ToString(), Definition->RequiredCount,
           *Definition->TriggerEventTag.ToString(), *GetNameSafe(GetOwner()));
}

bool UObjectiveTracker::CanRepeatObjective(bool bRepeatable, float CompletedTimeSeconds, float NowSeconds, float RepeatCooldownSeconds) {
    if (!bRepeatable) {
        return false;
    }
    if (RepeatCooldownSeconds <= 0.0f) {
        return true;
    }
    return (NowSeconds - CompletedTimeSeconds) >= RepeatCooldownSeconds;
}

EObjectiveOfferResult UObjectiveTracker::ServerTryAddObjective(UObjectiveDefinition *Definition, FObjectiveProgress &OutProgress) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        OutProgress = FObjectiveProgress();
        return EObjectiveOfferResult::Invalid;
    }

    const FGameplayTagContainer OwnedStoryTags = GatherOwnedStoryTags();
    const EObjectiveOfferResult Result = ResolveObjectiveOfferResult(ActiveObjectives, Definition, OutProgress, OwnedStoryTags);

    if (Result == EObjectiveOfferResult::AlreadyCompleted && Definition && Definition->bRepeatable) {
        const float Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
        for (FObjectiveProgress &Existing : ActiveObjectives) {
            if (Existing.Definition == Definition && Existing.bCompleted &&
                CanRepeatObjective(true, Existing.CompletedTimeSeconds, Now, Definition->RepeatCooldownSeconds)) {
                Existing.CurrentCount = 0;
                Existing.bCompleted = false;
                Existing.CompletedTimeSeconds = 0.0f;
                if (!Definition->IsHarvestObjective()) {
                    EnsureSubscribedToTag(Definition->TriggerEventTag);
                }
                OutProgress = Existing;
                UE_LOG(Myth, Log, TEXT("ObjectiveTracker: RE-assigned repeatable objective '%s' to %s."),
                       *Definition->DisplayText.ToString(), *GetNameSafe(GetOwner()));
                return EObjectiveOfferResult::Assigned;
            }
        }
    }

    if (Result != EObjectiveOfferResult::Assigned) {
        return Result;
    }

    ActiveObjectives.Add(OutProgress);

    if (!Definition->IsHarvestObjective()) {
        EnsureSubscribedToTag(Definition->TriggerEventTag);
    }

    UE_LOG(Myth, Log, TEXT("ObjectiveTracker: assigned objective '%s' (need %d x %s) to %s."),
           *Definition->DisplayText.ToString(), Definition->RequiredCount,
           *Definition->TriggerEventTag.ToString(), *GetNameSafe(GetOwner()));
    return Result;
}

void UObjectiveTracker::ServerAbandonObjective_Implementation(UObjectiveDefinition *Def) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Def) {
        return;
    }

    int32 FoundIndex = INDEX_NONE;
    for (int32 i = 0; i < ActiveObjectives.Num(); ++i) {
        if (ActiveObjectives[i].Definition == Def) {
            FoundIndex = i;
            break;
        }
    }

    if (FoundIndex == INDEX_NONE) {
        return;
    }

    if (ActiveObjectives[FoundIndex].bCompleted) {
        return;
    }

    FText ObjectiveName = Def->DisplayText;

    const FGameplayTag &Tag = Def->TriggerEventTag;
    ActiveObjectives.RemoveAt(FoundIndex);

    bool bTagStillUsed = false;
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.Definition && Prog.Definition->TriggerEventTag == Tag) {
            bTagStillUsed = true;
            break;
        }
    }
    if (!bTagStillUsed && BoundASC) {
        FDelegateHandle *Handle = BoundEventHandles.Find(Tag);
        if (Handle && Handle->IsValid()) {
            BoundASC->GenericGameplayEventCallbacks.FindOrAdd(Tag).Remove(*Handle);
        }
        BoundEventHandles.Remove(Tag);
    }

    UE_LOG(Myth, Log, TEXT("ObjectiveTracker: abandoned objective '%s' on %s."),
           *ObjectiveName.ToString(), *GetNameSafe(GetOwner()));

    ClientNotifyObjectiveAbandoned(ObjectiveName);
}

TArray<FObjectiveSummary> UObjectiveTracker::GetActiveObjectiveSummaries() const {
    TArray<FObjectiveSummary> Summaries;
    Summaries.Reserve(ActiveObjectives.Num());

    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        FObjectiveSummary Summary;
        Summary.DisplayText = Prog.Definition ? Prog.Definition->DisplayText : FText::GetEmpty();
        Summary.CurrentCount = Prog.CurrentCount;
        Summary.RequiredCount = Prog.Definition ? Prog.Definition->RequiredCount : 0;
        Summary.ProgressFraction = Summary.RequiredCount > 0
            ? static_cast<float>(Summary.CurrentCount) / static_cast<float>(Summary.RequiredCount)
            : 0.0f;
        Summary.bCompleted = Prog.bCompleted;
        Summary.QuestName = Prog.Definition ? Prog.Definition->QuestName : FText::GetEmpty();
        Summary.bOptional = Prog.Definition ? Prog.Definition->bOptional : false;
        Summaries.Add(Summary);
    }

    return Summaries;
}

int32 UObjectiveTracker::GetActiveCount() const {
    int32 Count = 0;
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (!Prog.bCompleted) {
            ++Count;
        }
    }
    return Count;
}

int32 UObjectiveTracker::GetCompletedCount() const {
    int32 Count = 0;
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.bCompleted) {
            ++Count;
        }
    }
    return Count;
}

void UObjectiveTracker::ClientNotifyObjectiveAbandoned_Implementation(const FText& ObjectiveName) {
    UE_LOG(Myth, Log, TEXT("ObjectiveTracker: objective '%s' abandoned."), *ObjectiveName.ToString());
}

void UObjectiveTracker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UObjectiveTracker, ActiveObjectives, COND_OwnerOnly);
}

void UObjectiveTracker::OnRep_ActiveObjectives() {
    OnObjectivesChanged.Broadcast();
}
