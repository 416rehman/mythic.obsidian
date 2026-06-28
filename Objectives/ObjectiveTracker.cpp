#include "Objectives/ObjectiveTracker.h"

#include "Objectives/ObjectiveDefinition.h"
#include "Mythic.h"
#include "GAS/MythicTags_GAS.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h" // objective progress/completion callout
#include "Itemization/Inventory/MythicInventoryComponent.h" // turn-in: GetItemCount + ServerRemoveItemByDefinition
#include "Itemization/Inventory/ItemDefinition.h"           // DeliverItem definition for the turn-in
#include "Net/UnrealNetwork.h"

UObjectiveTracker::UObjectiveTracker() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UObjectiveTracker::BeginPlay() {
    Super::BeginPlay();

    // Server-only: bind to the player's ASC kill event. (Mirrors UProficiencyComponent's authority-gated
    // ASC resolution at BeginPlay — the ASC is reliably present on the PlayerController by this point.)
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(GetOwner());
    UAbilitySystemComponent *ASC = ASI ? ASI->GetAbilitySystemComponent() : nullptr;
    if (!ASC) {
        // Soft-guard rather than checkf: without an ASC the player simply tracks no kill objectives this life.
        UE_LOG(Myth, Warning, TEXT("ObjectiveTracker: no ASC on %s at BeginPlay; kill objectives won't advance."),
               *GetNameSafe(GetOwner()));
        return;
    }

    // Subscribe to every distinct trigger tag our CURRENT objectives use (objectives are usually assigned later via
    // ServerAddObjective, which subscribes on demand — this covers any present at init, e.g. restored from a save).
    // Each event fires on the owning player's own ASC, so every callback is this player's action by construction.
    BoundASC = ASC;
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.Definition) {
            EnsureSubscribedToTag(Prog.Definition->TriggerEventTag);
        }
    }
}

void UObjectiveTracker::EnsureSubscribedToTag(const FGameplayTag &Tag) {
    if (!BoundASC || !Tag.IsValid() || BoundEventHandles.Contains(Tag)) {
        return; // no ASC yet, invalid tag, or already listening for this tag
    }
    const FDelegateHandle Handle = BoundASC->GenericGameplayEventCallbacks.FindOrAdd(Tag).AddUObject(
        this, &UObjectiveTracker::HandleGameplayEvent);
    BoundEventHandles.Add(Tag, Handle);
}

void UObjectiveTracker::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    // Unbind every per-tag handle so delegates don't dangle / double-bind across pawn or PlayerState reuse (the
    // ASC lives on the persistent PlayerState).
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
    // Occurrence count by default (+1, correct for kills); quantity-bearing events advance by the rounded magnitude
    // when the objective opts in, floored at 1 so a qualifying event always makes progress.
    const int32 Advance = bCountByMagnitude ? FMath::Max(1, FMath::RoundToInt(EventMagnitude)) : 1;
    int32 NewCount = CurrentCount + Advance;
    OutJustCompleted = (NewCount >= RequiredCount);
    if (OutJustCompleted) {
        NewCount = RequiredCount; // clamp (magnitude can overshoot) for clean N/N display
    }
    OutNewCount = NewCount;
}

EObjectiveOfferResult UObjectiveTracker::ResolveObjectiveOfferResult(const TArray<FObjectiveProgress> &TrackedObjectives,
                                                                     const UObjectiveDefinition *Definition,
                                                                     FObjectiveProgress &OutProgress) {
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

    // Chain gate: a later step won't assign until every prerequisite step is COMPLETED in this player's tracked set.
    if (!AreObjectivePrerequisitesMet(Definition->PrerequisiteObjectives, TrackedObjectives)) {
        return EObjectiveOfferResult::PrerequisitesNotMet;
    }

    OutProgress.Definition = const_cast<UObjectiveDefinition *>(Definition);
    return EObjectiveOfferResult::Assigned;
}

bool UObjectiveTracker::AreObjectivePrerequisitesMet(const TArray<TObjectPtr<UObjectiveDefinition>> &Prerequisites,
                                                     const TArray<FObjectiveProgress> &TrackedObjectives) {
    for (const TObjectPtr<UObjectiveDefinition> &Prereq : Prerequisites) {
        if (!Prereq) {
            continue; // ignore a null prerequisite entry (designer slop) — not a blocker
        }
        bool bPrereqCompleted = false;
        for (const FObjectiveProgress &Prog : TrackedObjectives) {
            if (Prog.Definition == Prereq && Prog.bCompleted) {
                bPrereqCompleted = true;
                break;
            }
        }
        if (!bPrereqCompleted) {
            return false; // a prerequisite is untracked or not yet complete → the chain step stays gated
        }
    }
    return true;
}

void UObjectiveTracker::CollectAssignableNextObjectives(const TArray<TObjectPtr<UObjectiveDefinition>> &CandidateNext,
                                                        const TArray<FObjectiveProgress> &TrackedObjectives,
                                                        TArray<UObjectiveDefinition *> &OutAssignable) {
    for (const TObjectPtr<UObjectiveDefinition> &Next : CandidateNext) {
        if (!Next) {
            continue;
        }
        FObjectiveProgress Scratch;
        // Reuse the single offer-decision: a next step is assignable iff it is not already tracked and its own
        // prerequisites are met. An already-active/completed step → not Assigned → skipped (so a chain cycle can't loop).
        if (ResolveObjectiveOfferResult(TrackedObjectives, Next, Scratch) == EObjectiveOfferResult::Assigned) {
            OutAssignable.AddUnique(Next); // a step reachable from two completing parents (or listed twice) assigns once
        }
    }
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
        if (OfferResult == EObjectiveOfferResult::PrerequisitesNotMet) {
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
    // Server-only (bound on authority; the event itself fires only on authority). Defensive re-checks.
    if (!Payload || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    APlayerController *PC = Cast<APlayerController>(GetOwner());
    if (!PC) {
        return;
    }

    // Advance every active, non-completed objective whose trigger tag matches this event. In-place mutation of a
    // plain replicated TArray<FStruct> replicates on the next net update (same pattern as FactionStanding).
    int32 NotifyIndex = 0; // stack offset so 2+ same-tag objectives advancing on one event don't overlap their floaters
    // Chain auto-advance: a completing objective's NextObjectives are collected here and assigned AFTER the loop —
    // never mid-iteration, since ServerTryAddObjective mutates ActiveObjectives (which would invalidate this range-for).
    TArray<TObjectPtr<UObjectiveDefinition>> PendingNextSteps;
    for (FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.bCompleted || !Prog.Definition) {
            continue;
        }
        if (Prog.Definition->TriggerEventTag != Payload->EventTag) {
            continue;
        }
        // Delivery objectives are advanced ONLY by handing the item to their NPC (ServerTurnInDeliveriesTo), never by a
        // GAS event — skip them here so a default TriggerEventTag (Kill) can't silently advance a turn-in objective.
        if (Prog.Definition->IsDeliveryObjective()) {
            continue;
        }
        // Optional payload-tag filter: one trigger family (e.g. GAS.Event.Item.Acquired) can serve type-specific
        // objectives ("collect N wood") by matching the item's ItemType carried in the event's TargetTags.
        const FGameplayTag &RequiredTag = Prog.Definition->RequiredPayloadTag;
        if (RequiredTag.IsValid() && !Payload->TargetTags.HasTag(RequiredTag)) {
            continue;
        }

        AdvanceObjectiveProgress(Prog, Prog.Definition->bCountByEventMagnitude, Payload->EventMagnitude, PC,
                                 PendingNextSteps, NotifyIndex);
    }

    ProcessChainAdvance(PC, PendingNextSteps, NotifyIndex);
}

void UObjectiveTracker::AdvanceObjectiveProgress(FObjectiveProgress &Prog, bool bCountByMagnitude, float Magnitude,
                                                 APlayerController *PC,
                                                 TArray<TObjectPtr<UObjectiveDefinition>> &PendingNextSteps,
                                                 int32 &NotifyIndex) {
    if (!Prog.Definition) {
        return;
    }
    // Advance this objective. The count math (magnitude floor, completion threshold, overshoot clamp) is the single
    // source of truth in ComputeObjectiveProgress (unit-tested) — the clamp now happens inside it.
    int32 NewCount = Prog.CurrentCount;
    bool bJustCompleted = false;
    ComputeObjectiveProgress(Prog.CurrentCount, bCountByMagnitude, Magnitude, Prog.Definition->RequiredCount, NewCount,
                             bJustCompleted);
    Prog.CurrentCount = NewCount;
    if (bJustCompleted) {
        Prog.bCompleted = true;
        // Queue this step's chain successors for assignment after the loop (deferred — ServerTryAddObjective mutates
        // ActiveObjectives, which would invalidate the caller's range-for).
        PendingNextSteps.Append(Prog.Definition->NextObjectives);
        // Grant rewards on the server via the canonical reward holder (builds correct XP/item-level contexts).
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
    // Player-facing callout (server → owning client): "<Objective> N/M" each step, "Objective Complete: <Objective>"
    // on the finishing step — makes the otherwise-silent quest loop legible. (A persistent tracker HUD is logged.)
    if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
        // On the finishing step show the authored completion line (CompletedText) if any; otherwise the progress line.
        MythicPC->ClientNotifyObjective(Prog.Definition->GetCalloutText(bJustCompleted), Prog.CurrentCount,
                                        Prog.Definition->RequiredCount, bJustCompleted, NotifyIndex++);
    }
}

void UObjectiveTracker::ProcessChainAdvance(APlayerController *PC,
                                            TArray<TObjectPtr<UObjectiveDefinition>> &PendingNextSteps,
                                            int32 &NotifyIndex) {
    // Deferred chain advance: assign each newly-unlocked next step (its prerequisites are now met and it isn't already
    // tracked) and announce it to the owning client — so a quest chain flows forward without re-talking the giver.
    if (PendingNextSteps.Num() == 0) {
        return;
    }
    TArray<UObjectiveDefinition *> ToAssign;
    CollectAssignableNextObjectives(PendingNextSteps, ActiveObjectives, ToAssign);
    AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC);
    for (UObjectiveDefinition *Next : ToAssign) {
        FObjectiveProgress OutProg;
        if (ServerTryAddObjective(Next, OutProg) == EObjectiveOfferResult::Assigned && MythicPC) {
            MythicPC->ClientNotifyObjective(Next->GetCalloutText(false), OutProg.CurrentCount, Next->RequiredCount,
                                            /*bCompleted*/ false, NotifyIndex++);
        }
    }
}

int32 UObjectiveTracker::ComputeDeliverConsumeCount(int32 CurrentCount, int32 RequiredCount, int32 Available) {
    // Remaining needed, clamped to what the player has. Negatives (over-complete / negative stock) floor at 0.
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
    TArray<TObjectPtr<UObjectiveDefinition>> PendingNextSteps;
    // Advance loop never mutates ActiveObjectives (consume + AdvanceObjectiveProgress touch the inventory + this Prog +
    // the local PendingNextSteps only); the chain assignment that DOES mutate it runs after the loop (mirrors the GAS path).
    for (FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.bCompleted || !Prog.Definition || !Prog.Definition->IsDeliveryObjective()) {
            continue;
        }
        // The talking NPC's identity must satisfy this objective's required receiver (hierarchical, mirrors the talk path).
        if (!NpcTag.MatchesTag(Prog.Definition->DeliverToNpcTag)) {
            continue;
        }
        UItemDefinition *Wanted = Prog.Definition->DeliverItem;
        const int32 Available = PlayerInventory->GetItemCount(Wanted);
        const int32 Consume = ComputeDeliverConsumeCount(Prog.CurrentCount, Prog.Definition->RequiredCount, Available);
        if (Consume <= 0) {
            continue; // player carries none (or the objective is already satisfied) — nothing to hand in
        }
        // Take exactly what we credit, THEN advance by that amount — never advance more than was actually consumed.
        PlayerInventory->ServerRemoveItemByDefinition(Wanted, Consume);
        AdvanceObjectiveProgress(Prog, /*bCountByMagnitude*/ true, static_cast<float>(Consume), PC, PendingNextSteps,
                                 NotifyIndex);
    }

    ProcessChainAdvance(PC, PendingNextSteps, NotifyIndex);
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

void UObjectiveTracker::SaveObjectives(TArray<FSerializedObjectiveData> &OutData) const {
    OutData.Reset();
    for (const FObjectiveProgress &Prog : ActiveObjectives) {
        if (!Prog.Definition) {
            continue; // an unresolved definition has no stable asset path to persist
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
        return; // server-authoritative; the restored list replicates to the owning client (COND_OwnerOnly)
    }
    // The save IS the authoritative set — rebuild from it (idempotent if called twice). Existing per-tag event
    // subscriptions are kept (EnsureSubscribedToTag is idempotent; any now-unused binding is harmless and EndPlay
    // clears all of them). Replicates as an in-place TArray change.
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

        // Re-arm the trigger subscription so a restored INCOMPLETE objective keeps advancing. No-op if BoundASC isn't
        // resolved yet (pre-BeginPlay load) — BeginPlay then subscribes from the restored ActiveObjectives. Completed
        // objectives don't strictly need it, but subscribing is idempotent and HandleGameplayEvent skips them.
        EnsureSubscribedToTag(Def->TriggerEventTag);
    }
    UE_LOG(Myth, Log, TEXT("ObjectiveTracker::RestoreObjectives: restored %d objective(s) on %s."),
           ActiveObjectives.Num(), *GetNameSafe(GetOwner()));
}

void UObjectiveTracker::ServerAddObjective(UObjectiveDefinition *Definition) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || !Definition) {
        return;
    }
    // Idempotent: don't re-add (or reset) an objective the player already tracks — lets a quest-giver be
    // re-interacted without consequence.
    if (HasObjective(Definition)) {
        return;
    }
    FObjectiveProgress Prog;
    Prog.Definition = Definition;
    ActiveObjectives.Add(Prog);

    // Start listening for THIS objective's trigger event (no-op if already bound for that tag, or pre-ASC-init).
    EnsureSubscribedToTag(Definition->TriggerEventTag);

    UE_LOG(Myth, Log, TEXT("ObjectiveTracker: assigned objective '%s' (need %d x %s) to %s."),
           *Definition->DisplayText.ToString(), Definition->RequiredCount,
           *Definition->TriggerEventTag.ToString(), *GetNameSafe(GetOwner()));
}

EObjectiveOfferResult UObjectiveTracker::ServerTryAddObjective(UObjectiveDefinition *Definition, FObjectiveProgress &OutProgress) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        OutProgress = FObjectiveProgress();
        return EObjectiveOfferResult::Invalid;
    }

    const EObjectiveOfferResult Result = ResolveObjectiveOfferResult(ActiveObjectives, Definition, OutProgress);
    if (Result != EObjectiveOfferResult::Assigned) {
        return Result;
    }

    ActiveObjectives.Add(OutProgress);

    // Start listening for THIS objective's trigger event (no-op if already bound for that tag, or pre-ASC-init).
    EnsureSubscribedToTag(Definition->TriggerEventTag);

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

    // completed objectives cannot be abandoned
    if (ActiveObjectives[FoundIndex].bCompleted) {
        return;
    }

    FText ObjectiveName = Def->DisplayText;

    // unsubscribe the event tag if no other active objective uses it
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
