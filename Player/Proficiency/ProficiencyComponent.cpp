// 


#include "ProficiencyComponent.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Mythic.h"
#include "ProficiencyDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Rewards/AttributeReward.h"
#include "Player/MythicPlayerController.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicTags_GAS.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Engine/World.h"

void FProficiency::GenerateTrack() {
    const int NumKeyMilestones = this->Definition->KeyMilestones.Num();
    const int NumGoals = this->Definition->AttributeGoals.Num();
    const int MaxLevel = this->Definition->MaxLevel;

    // Ensure there are milestones, goals, and max level defined
    if (NumGoals <= 0 || NumKeyMilestones <= 0 || MaxLevel <= 0) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing required data"));
        return;
    }

    // Initialize proficiency track with MaxLevel elements
    this->Track.SetNum(MaxLevel);

    // Rewards per goal. Clamp to >=1: with NumGoals > MaxLevel the integer MaxLevel/NumGoals truncates to 0, and the
    // per-level `Goal / GoalRewardSplitCount` below would then divide by zero (server FPE). 1 = each goal's full Goal.
    const int GoalRewardSplitCount = FMath::Max(1, MaxLevel / NumGoals);

    // Milestone interval (how often key milestones appear)
    const int MilestoneInterval = MaxLevel / NumKeyMilestones;
    int milestones_added = 0;

    // Reverse loop to ensure the last level always has a key milestone
    for (int32 i = MaxLevel - 1; i >= 0; --i) {
        // Calculate the level for the current milestone
        if (milestones_added < NumKeyMilestones) {
            const int milestone_level = MaxLevel - 1 - (milestones_added * MilestoneInterval);

            // If the current level matches the milestone level, set the key milestone
            if (i == milestone_level) {
                this->Track[i] = this->Definition->KeyMilestones[NumKeyMilestones - milestones_added - 1];
                milestones_added++;
            }
        }

        // Distribute rewards for each goal
        const auto &ChosenGoal = this->Definition->AttributeGoals[i % NumGoals];

        // Create the attribute reward for this level
        UAttributeReward *AttributeReward = NewObject<UAttributeReward>();
        AttributeReward->Attribute = ChosenGoal.Attribute;
        AttributeReward->Modifier = ChosenGoal.Modifier;
        AttributeReward->Magnitude = ChosenGoal.Goal / GoalRewardSplitCount;

        // Add the reward to the level's milestone if it doesn't already exist
        if (!this->Track[i].Rewards.Contains(AttributeReward)) {
            this->Track[i].Rewards.Add(AttributeReward);
        }
    }
}

void FProficiency::Instantiate() {
    if (!this->Definition) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Definition"));
        return;
    }

    GenerateTrack();
}

void UProficiencyComponent::OnAttributeChanged(const FOnAttributeChangeData &OnAttributeChangeData) {
    auto NewValue = OnAttributeChangeData.NewValue;
    auto OldValue = OnAttributeChangeData.OldValue;

    // Get the Proficiency that depends on this progression attribute
    auto Proficiency = this->Proficiencies.FindByPredicate([&OnAttributeChangeData](const FProficiency &Proficiency) {
        return Proficiency.ProgressAttribute == OnAttributeChangeData.Attribute;
    });
    if (!Proficiency) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Proficiency"));
        return;
    }

    // If XP overshot MaxXP, clamp the EFFECTIVE value used for level/reward computation but do NOT early-return: the old
    // early-return granted nothing because the clamp's re-entrant callback then saw NewLevel<=OldLevel and silently
    // dropped the FINAL level-up's rewards (the most important one — hitting max). We grant the crossed levels against
    // the clamped value, then write the clamp back at the very end (its re-fire harmlessly no-ops at the cap).
    bool bClampToMax = false;
    if (Proficiency->MaxXP > 0.0f && NewValue > Proficiency->MaxXP) {
        UE_LOG(Myth, Log, TEXT("Proficiency %s: XP %f exceeds MaxXP %f. Clamping (after granting crossed-level rewards)."),
               *Proficiency->Definition->Name.ToString(), NewValue, Proficiency->MaxXP);
        NewValue = Proficiency->MaxXP;
        bClampToMax = true;
    }

    // Skip during restore - we handle rewards separately (but still write the clamp if the value overshot).
    if (bIsRestoring) {
        if (bClampToMax) {
            ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
        }
        return;
    }

    // Calculate levels from XP
    int32 OldLevel = UProficiencyDefinition::CalcLevelAtXP(OldValue, Proficiency->Definition);
    int32 NewLevel = UProficiencyDefinition::CalcLevelAtXP(NewValue, Proficiency->Definition);

    // No level change - nothing to do (but still write the clamp if the raw value overshot the cap).
    if (NewLevel <= OldLevel) {
        if (bClampToMax) {
            ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
        }
        return;
    }

    APlayerController *Owner = Cast<APlayerController>(this->GetOwner());
    if (!Owner) {
        UE_LOG(Myth, Error, TEXT("Proficiency: Missing Owner"));
        return;
    }

    auto Context = FRewardContext(Owner);

    // Give rewards for NEW levels only (OldLevel to NewLevel-1). Also capture the highest KEY-milestone name crossed
    // (most levels have an empty Name; only authored key milestones carry one) for the player-facing callout.
    FText MilestoneName;
    for (int32 Level = OldLevel; Level < NewLevel && Level < Proficiency->Track.Num(); ++Level) {
        auto &Milestone = Proficiency->Track[Level];
        UE_LOG(Myth, Log, TEXT("%s Proficiency Reward: Level %d: %s"),
               *Proficiency->Definition->Name.ToString(), Level + 1, *Milestone.Name.ToString());

        if (!Milestone.Name.IsEmpty()) {
            MilestoneName = Milestone.Name;
        }
        for (auto Reward : Milestone.Rewards) {
            if (Reward) {
                Reward->Give(Context);
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("%s Proficiency: XP: %.0f -> %.0f (Level %d -> %d)"),
           *Proficiency->Definition->Name.ToString(), OldValue, NewValue, OldLevel, NewLevel);

    // Player-facing callout (finally surfaces the 12-skill progression). Only the SERVER fires the client RPC: the
    // owning client's OnRep-driven re-fire of OnAttributeChanged is gated out by !HasAuthority, and the server-side
    // bIsRestoring check above suppresses the save-restore replay — so the floater appears exactly once, on real XP gain.
    if (Owner->HasAuthority()) {
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(Owner)) {
            MythicPC->ClientNotifyProficiencyLevel(Proficiency->Definition->Name, NewLevel, MilestoneName);
        }
    }

    // Crossed-level rewards are granted — now write the clamp back. Its re-fire of OnAttributeChanged no-ops (at MaxXP
    // the new clamp check is false and NewLevel<=OldLevel), so there is no double-grant.
    if (bClampToMax) {
        ASC->SetNumericAttributeBase(Proficiency->ProgressAttribute, Proficiency->MaxXP);
    }
}

// Ensures the progression attribute set is spawned and binds the change delegate.
void UProficiencyComponent::ConfigureProgressionAttribute(FProficiency &Proficiency) {
    auto Def = Proficiency.Definition;
    if (!Def) {
        return;
    }

    // Get the attribute set class from the attribute
    auto HasAttribute = ASC->HasAttributeSetForAttribute(Proficiency.ProgressAttribute);
    if (!HasAttribute) {
        // Owner-Outered per-instance attribute set (engine idiom) — NOT the shared class CDO (GetDefaultObject), which
        // AddSpawnedAttribute would register as this ASC's live set; later base-value writes would then corrupt that
        // attribute-set's defaults for EVERY actor/ASC of the class (same bug class as R21-H1 in AttributeReward).
        UAttributeSet *AttributeSet = NewObject<UAttributeSet>(ASC->GetOwner(), Proficiency.ProgressAttribute.GetAttributeSetClass());
        ASC->AddSpawnedAttribute(AttributeSet);
        UE_LOG(Myth, Warning, TEXT("Proficiency: AttributeSet for Attribute %s granted because it wasn't"), *Proficiency.ProgressAttribute.AttributeName)
    }

    // Register the attribute change callback. RemoveAll(this) first so re-running ApplyLoadedProficiencies
    // (BeginPlay AND CharacterData::Deserialize on load-on-join, plus any in-session reload) can't stack duplicate
    // this-bound delegates on the ASC's attribute — duplicates would fire OnAttributeChanged N times per level-up
    // and grant each milestone's rewards N times. The delegate lives on the (persistent) ASC, not on FProficiency.
    ASC->GetGameplayAttributeValueChangeDelegate(Proficiency.ProgressAttribute).RemoveAll(this);
    ASC->GetGameplayAttributeValueChangeDelegate(Proficiency.ProgressAttribute).AddUObject(this, &UProficiencyComponent::OnAttributeChanged);

    // Cache the MaxXP for clamping logic
    Proficiency.MaxXP = ceil(UProficiencyDefinition::CalcCumulativeXPForLevel(Def->MaxLevel, Def));

    UE_LOG(Myth, Log, TEXT("Proficiency: Bound to %s (MaxXP: %.1f)"), *Proficiency.ProgressAttribute.AttributeName, Proficiency.MaxXP);
}

// Reapply only CanReapplyOnLoad rewards (attributes, abilities) for levels up to given level
void UProficiencyComponent::ReapplyRewardsForLevel(FProficiency &Proficiency, int32 TargetLevel) {
    if (!Proficiency.Definition) {
        return;
    }

    APlayerController *Owner = Cast<APlayerController>(GetOwner());
    if (!Owner) {
        return;
    }

    // APPLY-ONLY: the CDO reset that makes the relative (Additive/Multiplicative) AttributeRewards idempotent across
    // repeated loads now lives ONCE in ApplyLoadedProficiencies, as a single union reset across ALL proficiencies
    // performed BEFORE any reapply. That is required because two proficiencies can share a goal attribute: a per-call
    // reset (the old design) would let proficiency B's reset wipe proficiency A's just-applied contribution, leaving
    // CDO + B instead of CDO + A + B. The sole caller is ApplyLoadedProficiencies; do NOT reintroduce a reset here.
    auto Context = FRewardContext(Owner);
    int32 ReappliedCount = 0;

    // Level=1 (see the reset loop above): Track[0] is the spawn-default level-1 reward the live earn path never grants.
    for (int32 Level = 1; Level < TargetLevel && Level < Proficiency.Track.Num(); ++Level) {
        auto &Milestone = Proficiency.Track[Level];
        for (auto Reward : Milestone.Rewards) {
            if (Reward && Reward->CanReapplyOnLoad()) {
                Reward->Give(Context);
                ReappliedCount++;
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("Proficiency %s: Reapplied %d rewards for %d levels"),
           *Proficiency.Definition->Name.ToString(), ReappliedCount, TargetLevel);
}

// Called when the game starts
void UProficiencyComponent::BeginPlay() {
    Super::BeginPlay();

    // Cold-start apply: at BeginPlay the proficiencies have no SavedXP yet (a save loads later), so this seeds
    // the tracks at Level 0. The load-on-join path re-runs ApplyLoadedProficiencies after Deserialize stages XP.
    ApplyLoadedProficiencies();
}

void UProficiencyComponent::ApplyLoadedProficiencies() {
    // if  not authority, we don't need to do anything
    auto Owner = GetOwner();
    if (!Owner->HasAuthority()) {
        return;
    }

    // Get ASC from owner using their ASC interface
    ASC = Cast<IAbilitySystemInterface>(GetOwner())->GetAbilitySystemComponent();
    checkf(ASC, TEXT("The parent actor of the ProficiencyComponent must implement IAbilitySystemInterface"));

    // Two-phase load so multi-proficiency restore is additive-consistent with the live earn path (#7).
    //
    // PHASE 1 — per proficiency: Instantiate, set bIsRestoring iff SavedXP>0, ConfigureProgressionAttribute, clamp,
    // SetNumericAttributeBase(ProgressAttribute, SavedXP), compute the restored Level, and accumulate the UNION of
    // every goal attribute contributed across [1, Level) into one TSet. bIsRestoring guards each restore's
    // ProgressAttribute write so OnAttributeChanged does not double-grant milestones / pop the floater. It is kept SET
    // across BOTH phases (cleared only after Phase 2) so the reapply Give() calls stay suppressed exactly as the old
    // single-loop did — a designer could author a goal attribute equal to a ProgressAttribute, and a Phase-2 Give to
    // it must not re-enter OnAttributeChanged with the flag cleared (red-team requiredChange).
    //
    // Then reset each unioned goal attribute back to its class CDO default EXACTLY ONCE — not once per proficiency.
    // The old per-proficiency reset let a second proficiency wipe the first's just-applied contribution when they
    // shared a goal attribute (final = CDO + B; should be CDO + A + B). The Level>=1 floor (iter-54 #6) is kept here
    // identically to the reapply pass so reset and reapply stay symmetric: Track[0] is never reset nor reapplied.
    //
    // PHASE 2 — per restored proficiency: ReapplyRewardsForLevel (now apply-only) re-Gives CanReapplyOnLoad rewards
    // over [1, Level) on the union-reset baseline -> CDO + A + B.
    TArray<TPair<FProficiency *, int32>> ToReapply;
    TSet<FGameplayAttribute> ResetAttrs;

    for (auto &Proficiency : this->Proficiencies) {
        Proficiency.Instantiate();

        // If loaded from save, restore XP
        int32 Level = 0;
        if (Proficiency.SavedXP > 0.0f) {
            // Calculate level from saved XP
            Level = UProficiencyDefinition::CalcLevelAtXP(Proficiency.SavedXP, Proficiency.Definition);

            UE_LOG(Myth, Log, TEXT("Proficiency %s: Restoring XP=%.1f, Level=%d"),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Level);

            // Set restoring flag to skip OnAttributeChanged rewards (kept set through Phase 2, cleared at the end).
            bIsRestoring = true;
        }

        // Configure attribute - may trigger callback but bIsRestoring skips it
        ConfigureProgressionAttribute(Proficiency);

        // Clamp SavedXP if it exceeds MaxXP (e.g. if definitions changed)
        if (Proficiency.MaxXP > 0.0f && Proficiency.SavedXP > Proficiency.MaxXP) {
            UE_LOG(Myth, Warning, TEXT("Proficiency %s: SavedXP %.1f exceeds MaxXP %.1f. Clamping."),
                   *Proficiency.Definition->Name.ToString(), Proficiency.SavedXP, Proficiency.MaxXP);
            Proficiency.SavedXP = Proficiency.MaxXP;
        }

        ASC->SetNumericAttributeBase(Proficiency.ProgressAttribute, Proficiency.SavedXP);

        // Accumulate the union of goal attributes this track contributes over [1, Level). Level>=1 floor mirrors the
        // live earn path (CalcLevelAtXP floors at 1; OnAttributeChanged never grants Track[0]). At cold start every
        // Level==0, so this loop body never runs, the union stays empty, and the reset pass below is a no-op.
        for (int32 TrackLevel = 1; TrackLevel < Level && TrackLevel < Proficiency.Track.Num(); ++TrackLevel) {
            for (const auto &Reward : Proficiency.Track[TrackLevel].Rewards) {
                const UAttributeReward *AttrReward = Cast<UAttributeReward>(Reward);
                if (!AttrReward || !AttrReward->Attribute.IsValid()) {
                    continue;
                }
                ResetAttrs.Add(AttrReward->Attribute);
            }
        }

        // Defer the reapply to Phase 2 so the union reset runs first. Level==0 proficiencies are skipped: their
        // reapply loop would be empty anyway, so omitting them is behavior-preserving.
        if (Level > 0) {
            ToReapply.Emplace(&Proficiency, Level);
        }
    }

    // Single union reset: hard-set each contributed goal attribute back to its AttributeSet CDO default ONCE, before
    // any reapply, so relative AttributeRewards don't STACK across repeated whole-loads and a shared attribute isn't
    // wiped by a later proficiency.
    if (ASC) {
        for (const FGameplayAttribute &Attr : ResetAttrs) {
            if (ASC->HasAttributeSetForAttribute(Attr)) {
                const UAttributeSet *CDO = Attr.GetAttributeSetClass()->GetDefaultObject<UAttributeSet>();
                ASC->SetNumericAttributeBase(Attr, Attr.GetNumericValue(CDO));
            }
        }
    }

    // PHASE 2: apply-only reapply over the union-reset baseline, in proficiency order.
    for (const TPair<FProficiency *, int32> &Entry : ToReapply) {
        ReapplyRewardsForLevel(*Entry.Key, Entry.Value);
    }

    // Clear the restoring flag only AFTER Phase 2 — keeps the reapply Give() calls suppressed exactly as the original
    // single-loop did (red-team requiredChange). Cold start never set it (all SavedXP=0), so this stays false there.
    bIsRestoring = false;
}

FProficiency* UProficiencyComponent::FindCombatProficiency() {
    const FGameplayAttribute CombatAttr = UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute();
    for (auto &Proficiency : Proficiencies) {
        if (Proficiency.ProgressAttribute == CombatAttr) {
            return &Proficiency;
        }
    }
    return nullptr;
}

void UProficiencyComponent::GrantCombatXP(float Amount) {
    if (Amount <= 0.0f) {
        return;
    }
    if (!ASC || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    FProficiency *CombatProf = FindCombatProficiency();
    if (!CombatProf || !CombatProf->ProgressAttribute.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("ProficiencyComponent: no combat proficiency configured, cannot grant XP"));
        return;
    }

    // apply ProficiencyXPBonus from utility attributes (gear/buffs)
    if (const UMythicAttributeSet_Utility *Util = ASC->GetSet<UMythicAttributeSet_Utility>()) {
        Amount *= FMath::Max(0.0f, 1.0f + Util->GetProficiencyXPBonus());
    }

    // apply Enlighten buff bonus
    if (ASC->HasMatchingGameplayTag(GAS_BUFF_ENLIGHTEN)) {
        if (const AMythicGameState *GS = GetWorld() ? GetWorld()->GetGameState<AMythicGameState>() : nullptr) {
            Amount *= FMath::Max(0.0f, 1.0f + GS->EnlightenProficiencyBonus);
        }
    }

    if (Amount <= 0.0f) {
        return;
    }

    // grant via attribute base value modification, which fires OnAttributeChanged for level-up processing
    const float Current = ASC->GetNumericAttributeBase(CombatProf->ProgressAttribute);
    ASC->SetNumericAttributeBase(CombatProf->ProgressAttribute, Current + Amount);

    UE_LOG(Myth, Log, TEXT("ProficiencyComponent: granted %.1f combat proficiency XP (%.1f -> %.1f)"),
           Amount, Current, Current + Amount);
}

void UProficiencyComponent::ApplyDeathPenalty(float PenaltyFraction) {
    if (PenaltyFraction <= 0.0f) {
        return;
    }
    if (!ASC || !GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    FProficiency *CombatProf = FindCombatProficiency();
    if (!CombatProf || !CombatProf->ProgressAttribute.IsValid()) {
        return;
    }

    const float OldXP = ASC->GetNumericAttributeBase(CombatProf->ProgressAttribute);
    const float NewXP = ComputeXpAfterDeathPenalty(OldXP, PenaltyFraction);
    ASC->SetNumericAttributeBase(CombatProf->ProgressAttribute, NewXP);

    UE_LOG(Myth, Log, TEXT("ProficiencyComponent: death penalty, combat XP %.1f -> %.1f (%.0f%% loss)"),
           OldXP, NewXP, PenaltyFraction * 100.0f);
}

float UProficiencyComponent::ComputeXpAfterDeathPenalty(float CurrentXP, float PenaltyFraction) {
    const float Frac = FMath::Clamp(PenaltyFraction, 0.0f, 1.0f);
    return FMath::Max(0.0f, CurrentXP * (1.0f - Frac));
}

FProficiencySummary UProficiencyComponent::GetSummary(int32 Index) const {
    FProficiencySummary Summary;
    if (!Proficiencies.IsValidIndex(Index)) {
        return Summary;
    }

    const FProficiency &Prof = Proficiencies[Index];
    if (!Prof.Definition) {
        return Summary;
    }

    Summary.Name = Prof.Definition->Name;
    Summary.Description = Prof.Definition->Description;

    // use asc attribute base value if asc is initialized otherwise fallback to saved xp
    if (ASC) {
        Summary.CurrentXP = ASC->GetNumericAttributeBase(Prof.ProgressAttribute);
    } else {
        Summary.CurrentXP = Prof.SavedXP;
    }

    Summary.Level = UProficiencyDefinition::CalcLevelAtXP(Summary.CurrentXP, Prof.Definition);
    
    // calculate start and end xp for the current level
    Summary.LevelXPStart = UProficiencyDefinition::CalcCumulativeXPForLevel(Summary.Level, Prof.Definition);
    Summary.LevelXPEnd = UProficiencyDefinition::CalcCumulativeXPForLevel(Summary.Level + 1, Prof.Definition);
    
    // clamp progress fraction to one if max level is reached or calculate based on level cost
    if (Summary.Level >= Prof.Definition->MaxLevel) {
        Summary.ProgressFraction = 1.0f;
    } else {
        float Cost = Summary.LevelXPEnd - Summary.LevelXPStart;
        if (Cost > 0.0f) {
            Summary.ProgressFraction = FMath::Clamp((Summary.CurrentXP - Summary.LevelXPStart) / Cost, 0.0f, 1.0f);
        } else {
            Summary.ProgressFraction = 0.0f;
        }
    }

    return Summary;
}

void UProficiencyComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // Replicate the Proficiencies array after it has been generated
    DOREPLIFETIME_CONDITION(UProficiencyComponent, Proficiencies, COND_OwnerOnly);
}
