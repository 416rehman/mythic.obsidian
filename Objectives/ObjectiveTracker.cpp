#include "Objectives/ObjectiveTracker.h"

#include "Objectives/ObjectiveDefinition.h"
#include "Mythic.h"
#include "GAS/MythicTags_GAS.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h" // objective progress/completion callout
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
    for (FObjectiveProgress &Prog : ActiveObjectives) {
        if (Prog.bCompleted || !Prog.Definition) {
            continue;
        }
        if (Prog.Definition->TriggerEventTag != Payload->EventTag) {
            continue;
        }

        ++Prog.CurrentCount;
        const bool bJustCompleted = (Prog.CurrentCount >= Prog.Definition->RequiredCount);
        if (bJustCompleted) {
            Prog.bCompleted = true;
            // Grant rewards on the server via the canonical reward holder (builds correct XP/item-level contexts).
            Prog.Definition->Rewards.Give(PC);
            UE_LOG(Myth, Log, TEXT("ObjectiveTracker: objective '%s' completed (%d/%d); rewards granted to %s."),
                   *Prog.Definition->DisplayText.ToString(), Prog.CurrentCount, Prog.Definition->RequiredCount,
                   *GetNameSafe(PC));
        }
        // Player-facing callout (server → owning client): "<Objective> N/M" each step, "Objective Complete: <Objective>"
        // on the finishing step — makes the otherwise-silent quest loop legible. (A persistent tracker HUD is logged.)
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(PC)) {
            MythicPC->ClientNotifyObjective(Prog.Definition->DisplayText, Prog.CurrentCount,
                                            Prog.Definition->RequiredCount, bJustCompleted, NotifyIndex++);
        }
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

void UObjectiveTracker::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UObjectiveTracker, ActiveObjectives, COND_OwnerOnly);
}
