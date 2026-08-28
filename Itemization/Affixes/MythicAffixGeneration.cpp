#include "Itemization/Affixes/MythicAffixGeneration.h"

#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"

namespace {
struct FAffixGenerationLedger {
    TSet<FPrimaryAssetId> Definitions;
    TSet<FGameplayTag> Stats;
    TSet<FGameplayTag> Conflicts;
    TSet<FGameplayTag> UniqueStackingGroups;
    TSet<FGuid> RollGuids;
};

bool FindEligibleTiers(const FCompiledAffix &Affix,
                       const FGameplayTagContainer &ContextTags,
                       const int32 ItemLevel, const int64 RemainingBudget,
                       const bool bUnlimitedBudget,
                       const FCompiledAffixTierProgression *&OutProgression,
                       TArray<const FCompiledAffixTier *> &OutTiers) {
    OutTiers.Reset();
    OutProgression = Affix.ResolveProgression(ContextTags);
    if (!OutProgression) return false;
    for (const FCompiledAffixTier &Tier : OutProgression->Tiers) {
        if (Tier.MinItemLevel <= ItemLevel
            && (bUnlimitedBudget || Tier.BudgetCost <= RemainingBudget)) {
            OutTiers.Add(&Tier);
        }
    }
    return !OutTiers.IsEmpty();
}

bool PickTier(const FCompiledAffix &Affix, const FGuid &OriginGuid,
              const EMythicAffixGrantTierMode TierMode, const int32 ExactTierRank,
              const FMythicAffixGrantContext &Context, const int64 RemainingBudget,
              const bool bUnlimitedBudget,
              const FCompiledAffixTierProgression *&OutProgression,
              const FCompiledAffixTier *&OutTier, uint64 *OutDraw = nullptr) {
    OutProgression = Affix.ResolveProgression(Context.ContextTags);
    OutTier = nullptr;
    if (!OutProgression
        || !MythicAffixGrant::IsTierSelectionValid(TierMode, ExactTierRank)) return false;
    if (TierMode == EMythicAffixGrantTierMode::ExactTier) {
        OutTier = OutProgression->Tiers.FindByPredicate(
            [ExactTierRank](const FCompiledAffixTier &Tier) {
                return Tier.TierRank == ExactTierRank;
            });
        // An exact grant is an explicit content decision (for example, a gem grade or
        // weapon implicit), so its tier rank overrides random-roll item-level
        // availability. Budget enforcement remains authoritative.
        return OutTier && (bUnlimitedBudget || OutTier->BudgetCost <= RemainingBudget);
    }
    TArray<const FCompiledAffixTier *> Tiers;
    if (!FindEligibleTiers(Affix, Context.ContextTags, Context.ItemLevel, RemainingBudget,
                           bUnlimitedBudget, OutProgression, Tiers)) return false;
    TArray<int64> Weights;
    for (const FCompiledAffixTier *Tier : Tiers) Weights.Add(Tier->TierWeight);
    FMythicAffixRngV1 Rng(0, 0);
    const FPrimaryAssetId StreamOwner = Context.ProfileId.IsValid()
        ? Context.ProfileId : Affix.Definition.DefinitionId;
    if (!FMythicAffixRngFactory::BuildSubstream(
            Context.Seed, 1, StreamOwner, OriginGuid, FGuid(),
            Affix.Definition.DefinitionId, 0, Context.RollOrdinal,
            EMythicAffixRngPurpose::TierSelection, Rng)) return false;
    int32 Index = INDEX_NONE;
    if (!Rng.PickWeightedIndex(Weights, Index, OutDraw) || !Tiers.IsValidIndex(Index)) return false;
    OutTier = Tiers[Index];
    return true;
}

bool CanAcceptAffix(const FCompiledAffix &Affix, const FCompiledAffixPolicy &Policy,
                    const FAffixGenerationLedger &Ledger, FName &OutReason) {
    if (Policy.bDisallowDuplicateAffixDefinition
        && Ledger.Definitions.Contains(Affix.Definition.DefinitionId)) {
        OutReason = TEXT("DuplicateAffix");
        return false;
    }
    if (Policy.bDisallowDuplicateTargetStat
        && Ledger.Stats.Contains(Affix.Definition.TargetStatTag)) {
        OutReason = TEXT("DuplicateTargetStat");
        return false;
    }
    for (const FGameplayTag &Conflict : Affix.Definition.ConflictGroups) {
        if (Ledger.Conflicts.Contains(Conflict)) {
            OutReason = TEXT("ConflictGroup");
            return false;
        }
    }
    if (Affix.Definition.StackingRule != EMythicAffixStackingRule::StackAll
        && Affix.Definition.StackingGroup.IsValid()
        && Ledger.UniqueStackingGroups.Contains(Affix.Definition.StackingGroup)) {
        OutReason = TEXT("StackingGroup");
        return false;
    }
    OutReason = NAME_None;
    return true;
}

void ReserveAffix(const FCompiledAffix &Affix, const FRolledAffix &Snapshot,
                  FAffixGenerationLedger &Ledger) {
    Ledger.Definitions.Add(Affix.Definition.DefinitionId);
    Ledger.Stats.Add(Affix.Definition.TargetStatTag);
    for (const FGameplayTag &Conflict : Affix.Definition.ConflictGroups) Ledger.Conflicts.Add(Conflict);
    if (Affix.Definition.StackingRule != EMythicAffixStackingRule::StackAll
        && Affix.Definition.StackingGroup.IsValid()) {
        Ledger.UniqueStackingGroups.Add(Affix.Definition.StackingGroup);
    }
    Ledger.RollGuids.Add(Snapshot.RollGuid);
}

FGuid MakeRollGuid(const FMythicAffixGrantContext &Context, const FCompiledAffix &Affix,
                   const FCompiledAffixTier &Tier, const FGuid &GrantGuid,
                   const FGuid &SliceGuid, const FGuid &PoolRowGuid) {
    FMythicAffixCanonicalWriter Payload("MYTHIC_AFFIX_ROLL_FIELDS_V2");
    Payload.AddGuid(Context.ItemInstanceGuid);
    Payload.AddInt32(Context.RollOrdinal);
    Payload.AddPrimaryAssetId(Affix.Definition.DefinitionId);
    Payload.AddInt32(Tier.TierRank);
    Payload.AddGuid(GrantGuid);
    Payload.AddGuid(SliceGuid);
    Payload.AddGuid(PoolRowGuid);
    return Payload.IsValid()
        ? FMythicAffixRngFactory::GuidFromCanonicalBytes("Mythic.Affix.Roll.V2", Payload.GetBytes())
        : FGuid();
}

bool MaterializeCompiled(const FCompiledAffix &Affix,
                         const FCompiledAffixTierProgression &Progression,
                         const FCompiledAffixTier &Tier,
                         const FMythicAffixGrantContext &Context,
                         const FGuid &GrantGuid, const FGuid &SliceGuid,
                         const FGuid &PoolRowGuid, const FGameplayTag RollGroup,
                         const FGameplayTag SourceKind, const bool bLocked,
                         const int32 PoolRevision, const int32 PoolRowRevision,
                         const FCompiledAffixProfile *Profile,
                         const FCompiledAffixGrantClosure *GrantClosure,
                         FRolledAffix &Out, FMythicAffixRollDecisionTrace *Trace) {
    if ((Profile != nullptr) == (GrantClosure != nullptr)) return false;
    Out = FRolledAffix();
    Out.RollGuid = MakeRollGuid(Context, Affix, Tier, GrantGuid, SliceGuid, PoolRowGuid);
    if (!Out.RollGuid.IsValid()) return false;
    Out.AffixDefinition = Affix.Definition.Definition;
    Out.TierRank = Tier.TierRank;
    Out.bIsLocked = bLocked;
    Out.Provenance.ProfileId = Context.ProfileId;
    Out.Provenance.PolicyId = Context.PolicyId;
    Out.Provenance.PoolId = Context.PoolId;
    Out.Provenance.RollGroup = RollGroup;
    Out.Provenance.SourceKind = SourceKind;
    Out.Provenance.ProfileRevision = Profile ? Profile->ProfileRevision : 0;
    Out.Provenance.PolicyRevision = Profile ? Profile->Policy.Revision : 0;
    Out.Provenance.PoolRevision = PoolRevision;
    Out.Provenance.PoolRowRevision = PoolRowRevision;
    Out.Provenance.DefinitionRevision = Affix.Definition.Revision;
    Out.Provenance.OriginGrantGuid = GrantGuid;
    Out.Provenance.OriginSliceGuid = SliceGuid;
    Out.Provenance.OriginPoolRowGuid = PoolRowGuid;
    Out.Provenance.SourceItemGuid = Context.ItemInstanceGuid;
    Out.Provenance.GameplayContentHash = Profile ? Profile->GameplayContentHash
                                                 : GrantClosure->GameplayContentHash;
    Out.Provenance.GeneratedItemLevel = Context.ItemLevel;
    Out.Provenance.GeneratedRarity = Context.Rarity;
    Out.Provenance.AlgorithmVersion = Profile ? Profile->Policy.AlgorithmVersion
                                              : GrantClosure->AlgorithmVersion;

    float Min = 0.0f;
    float Max = 0.0f;
    if (!Tier.Magnitude.Resolve(Context.ItemLevel, Min, Max)) return false;
    FMythicAffixRngV1 Rng(0, 0);
    const FPrimaryAssetId StreamOwner = Context.ProfileId.IsValid()
        ? Context.ProfileId : Affix.Definition.DefinitionId;
    if (!FMythicAffixRngFactory::BuildSubstream(
            Context.Seed, Out.Provenance.AlgorithmVersion, StreamOwner,
            GrantGuid.IsValid() ? GrantGuid : SliceGuid, PoolRowGuid,
            Affix.Definition.DefinitionId, Tier.TierRank, Context.RollOrdinal,
            EMythicAffixRngPurpose::Magnitude, Rng)) return false;
    const uint32 Draw = Rng.NextUInt32();
    const double Unit = (static_cast<double>(Draw) + 0.5) / 4294967296.0;
    const float Before = Min == Max ? Min
        : static_cast<float>(static_cast<double>(Min)
            + (static_cast<double>(Max) - Min) * Unit);
    const float Final = Affix.Definition.Quantization.Apply(Before);
    if (!FMath::IsFinite(Final)
        || (MythicAffix::ModifierRequiresNonZeroMagnitude(Affix.Definition.ModifierOp)
            && FMath::IsNearlyZero(Final))) return false;
    Out.Magnitude = Final;
    if (Trace) {
        Trace->SelectedProgression = Progression.DeveloperName;
        Trace->SelectedTierRank = Tier.TierRank;
        Trace->Value.Draw = Draw;
        Trace->Value.ResolvedMin = Min;
        Trace->Value.ResolvedMax = Max;
        Trace->Value.ValueBeforeQuantization = Before;
        Trace->Value.FinalValue = Final;
        Trace->ResultRollGuid = Out.RollGuid;
    }
    return Out.IsGameplayValid();
}
}

bool FMythicAffixGrantService::Materialize(
    const FMythicAffixGrantSpec &Spec, const FMythicAffixGrantContext &Context,
    const UMythicItemizationDataRegistrySubsystem &Registry,
    FRolledAffix &OutSnapshot, FMythicAffixRollTrace *OptionalTrace) {
    const TSharedPtr<const FCompiledAffixGrantClosure> Closure = Registry.FindCompiledGrant(Spec);
    if (!Closure.IsValid() || Closure->GameplayContentHash.IsZero()) return false;
    const FCompiledAffixTierProgression *Progression = nullptr;
    const FCompiledAffixTier *Tier = nullptr;
    uint64 TierDraw = 0;
    if (!PickTier(Closure->Affix, Spec.GrantGuid, Spec.TierMode, Spec.ExactTierRank,
                  Context, MAX_int64, true, Progression, Tier, &TierDraw)) return false;
    FMythicAffixRollDecisionTrace *Decision = nullptr;
    if (OptionalTrace) {
        Decision = &OptionalTrace->Decisions.AddDefaulted_GetRef();
        Decision->RollOrdinal = Context.RollOrdinal;
        Decision->TierSelectionDraw = TierDraw;
        Decision->SelectedAffixDefinition = Spec.AffixDefinition;
    }
    return MaterializeCompiled(Closure->Affix, *Progression, *Tier, Context,
        Spec.GrantGuid, FGuid(), FGuid(), Spec.RollGroup, Spec.SourceKind,
        Spec.bLocked, 0, 0, nullptr, Closure.Get(), OutSnapshot, Decision);
}

bool FMythicAffixGenerator::Generate(const FMythicAffixRollRequest &Request,
                                     const FCompiledAffixProfile &Profile,
                                     TArray<FRolledAffix> &OutSnapshots,
                                     FMythicAffixRollTrace *OptionalTrace) {
    OutSnapshots.Reset();
    if (!Request.ItemInstanceGuid.IsValid() || Request.ItemLevel < 1
        || Request.ProfileId != Profile.ProfileId
        || Request.AlgorithmVersion != Profile.Policy.AlgorithmVersion) return false;
    const FCompiledAffixRarityBudget *Budget = Profile.Policy.Budgets.Find(Request.Rarity);
    if (!Budget) return false;
    if (OptionalTrace) {
        *OptionalTrace = FMythicAffixRollTrace();
        OptionalTrace->TraceId = FGuid::NewGuid();
        OptionalTrace->ProfileId = Profile.ProfileId;
        OptionalTrace->PolicyId = Profile.Policy.PolicyId;
        OptionalTrace->GameplayContentHash = Profile.GameplayContentHash;
        OptionalTrace->PresentationContentHash = Profile.PresentationContentHash;
        OptionalTrace->ServerSeed = Request.Seed;
        OptionalTrace->AlgorithmVersion = Request.AlgorithmVersion;
        OptionalTrace->RequestContext = Request.ContextTags;
    }

    TArray<FRolledAffix> Temporary;
    FAffixGenerationLedger Ledger;
    int64 RemainingBudget = Budget->bUnlimitedMagnitudeBudget ? MAX_int64 : Budget->MagnitudeBudget;
    int32 RollOrdinal = 0;
    for (const FCompiledAffixGrant &Grant : Profile.GuaranteedGrants) {
        FName Reason;
        if (!CanAcceptAffix(Grant.Affix, Profile.Policy, Ledger, Reason)) return false;
        FMythicAffixGrantContext Context{Request.ItemInstanceGuid, Request.ItemLevel,
            Request.Rarity, Request.ContextTags, Request.Seed, Profile.ProfileId,
            Profile.Policy.PolicyId, FPrimaryAssetId(), RollOrdinal};
        const FCompiledAffixTierProgression *Progression = nullptr;
        const FCompiledAffixTier *Tier = nullptr;
        uint64 TierDraw = 0;
        if (!PickTier(Grant.Affix, Grant.Spec.GrantGuid, Grant.Spec.TierMode,
                      Grant.Spec.ExactTierRank, Context, RemainingBudget,
                      Budget->bUnlimitedMagnitudeBudget
                          || !Profile.Policy.bGuaranteedConsumesMagnitudeBudget,
                      Progression, Tier, &TierDraw)) return false;
        FMythicAffixRollDecisionTrace *Decision = OptionalTrace
            ? &OptionalTrace->Decisions.AddDefaulted_GetRef() : nullptr;
        if (Decision) {
            Decision->RollOrdinal = RollOrdinal;
            Decision->TierSelectionDraw = TierDraw;
            Decision->SelectedAffixDefinition = Grant.Spec.AffixDefinition;
            Decision->BudgetBefore = RemainingBudget;
        }
        FRolledAffix Snapshot;
        if (!MaterializeCompiled(Grant.Affix, *Progression, *Tier, Context,
                Grant.Spec.GrantGuid, FGuid(), FGuid(), Grant.Spec.RollGroup,
                Grant.Spec.SourceKind, Grant.Spec.bLocked, 0, 0, &Profile, nullptr,
                Snapshot, Decision) || Ledger.RollGuids.Contains(Snapshot.RollGuid)) return false;
        if (Profile.Policy.bGuaranteedConsumesMagnitudeBudget
            && !Budget->bUnlimitedMagnitudeBudget) RemainingBudget -= Tier->BudgetCost;
        if (Decision) Decision->BudgetAfter = RemainingBudget;
        ReserveAffix(Grant.Affix, Snapshot, Ledger);
        Temporary.Add(MoveTemp(Snapshot));
        ++RollOrdinal;
    }

    TMap<FGameplayTag, int32> RollGroupUsed;
    TSet<FGuid> RemovedRows;
    int32 RandomLanded = 0;
    while (RandomLanded < Budget->RandomRollCount) {
        FMythicAffixRollDecisionTrace *Decision = OptionalTrace
            ? &OptionalTrace->Decisions.AddDefaulted_GetRef() : nullptr;
        if (Decision) {
            Decision->RollOrdinal = RollOrdinal;
            Decision->BudgetBefore = RemainingBudget;
        }
        struct FEligibleSlice {
            const FCompiledAffixSlice *Slice = nullptr;
            TArray<const FCompiledAffixPoolRow *> Rows;
        };
        TArray<FEligibleSlice> EligibleSlices;
        int32 HighestConfiguredPriority = MIN_int32;
        for (const FCompiledAffixSlice &Slice : Profile.RandomSlices) {
            const bool bHasUnremoved = Slice.Rows.ContainsByPredicate(
                [&RemovedRows](const FCompiledAffixPoolRow &Row) {
                    return !RemovedRows.Contains(Row.PoolRowGuid);
                });
            if (bHasUnremoved) HighestConfiguredPriority = FMath::Max(HighestConfiguredPriority,
                                                                      Slice.Priority);
            FEligibleSlice Eligible;
            Eligible.Slice = &Slice;
            for (const FCompiledAffixPoolRow &Row : Slice.Rows) {
                FMythicAffixCandidateTrace Candidate;
                Candidate.SliceGuid = Slice.SliceGuid;
                Candidate.SlicePriority = Slice.Priority;
                Candidate.SliceWeight = Slice.SliceWeight;
                Candidate.PoolId = Slice.PoolId;
                Candidate.PoolRowGuid = Row.PoolRowGuid;
                Candidate.AffixDefinition = Row.Affix.Definition.Definition;
                Candidate.RollGroup = Row.RollGroup;
                Candidate.CompiledRowWeight = Row.SelectionWeight;
                FName Rejection;
                if (RemovedRows.Contains(Row.PoolRowGuid)) Rejection = TEXT("WithoutReplacement");
                else if (!Row.EligibilityQuery.IsEmpty()
                    && !Row.EligibilityQuery.Matches(Request.ContextTags)) Rejection = TEXT("ContextQuery");
                else if (!Budget->RollGroupCaps.Contains(Row.RollGroup)
                    || RollGroupUsed.FindRef(Row.RollGroup)
                        >= Budget->RollGroupCaps.FindRef(Row.RollGroup)) Rejection = TEXT("RollGroupCap");
                else if (!CanAcceptAffix(Row.Affix, Profile.Policy, Ledger, Rejection)) {}
                else {
                    const FCompiledAffixTierProgression *Progression = nullptr;
                    TArray<const FCompiledAffixTier *> Tiers;
                    if (!FindEligibleTiers(Row.Affix, Request.ContextTags, Request.ItemLevel,
                                           RemainingBudget, Budget->bUnlimitedMagnitudeBudget,
                                           Progression, Tiers)) Rejection = TEXT("NoEligibleTier");
                    else {
                        Candidate.MinimumEligibleTierCost = MAX_int64;
                        for (const FCompiledAffixTier *Tier : Tiers) {
                            Candidate.MinimumEligibleTierCost = FMath::Min(
                                Candidate.MinimumEligibleTierCost, Tier->BudgetCost);
                        }
                        Candidate.bEligible = true;
                        Eligible.Rows.Add(&Row);
                    }
                }
                Candidate.RejectionCode = Rejection;
                if (Decision) Decision->Candidates.Add(MoveTemp(Candidate));
            }
            if (!Eligible.Rows.IsEmpty()) EligibleSlices.Add(MoveTemp(Eligible));
        }
        if (EligibleSlices.IsEmpty()) {
            if (Decision) Decision->BudgetAfter = RemainingBudget;
            break;
        }
        int32 HighestEligiblePriority = MIN_int32;
        for (const FEligibleSlice &Slice : EligibleSlices) {
            HighestEligiblePriority = FMath::Max(HighestEligiblePriority, Slice.Slice->Priority);
        }
        if (HighestEligiblePriority < HighestConfiguredPriority
            && !Profile.Policy.bAllowLowerPriorityFallback) break;
        EligibleSlices.RemoveAll([HighestEligiblePriority](const FEligibleSlice &Slice) {
            return Slice.Slice->Priority != HighestEligiblePriority;
        });
        EligibleSlices.Sort([](const FEligibleSlice &A, const FEligibleSlice &B) {
            return A.Slice->SliceGuid < B.Slice->SliceGuid;
        });
        TArray<int64> SliceWeights;
        for (const FEligibleSlice &Slice : EligibleSlices) SliceWeights.Add(Slice.Slice->SliceWeight);
        FMythicAffixRngV1 SliceRng(0, 0);
        if (!FMythicAffixRngFactory::BuildSubstream(
                Request.Seed, Request.AlgorithmVersion, Profile.ProfileId,
                FGuid(), FGuid(), Profile.ProfileId, 0, RollOrdinal,
                EMythicAffixRngPurpose::SliceSelection, SliceRng)) return false;
        int32 SliceIndex = INDEX_NONE;
        uint64 SliceDraw = 0;
        if (!SliceRng.PickWeightedIndex(SliceWeights, SliceIndex, &SliceDraw)
            || !EligibleSlices.IsValidIndex(SliceIndex)) return false;
        FEligibleSlice &ChosenSlice = EligibleSlices[SliceIndex];
        TArray<int64> RowWeights;
        for (const FCompiledAffixPoolRow *Row : ChosenSlice.Rows) {
            RowWeights.Add(Row->SelectionWeight);
        }
        FMythicAffixRngV1 RowRng(0, 0);
        if (!FMythicAffixRngFactory::BuildSubstream(
                Request.Seed, Request.AlgorithmVersion, Profile.ProfileId,
                ChosenSlice.Slice->SliceGuid, FGuid(), Profile.ProfileId, 0,
                RollOrdinal, EMythicAffixRngPurpose::RowSelection, RowRng)) return false;
        int32 RowIndex = INDEX_NONE;
        uint64 RowDraw = 0;
        if (!RowRng.PickWeightedIndex(RowWeights, RowIndex, &RowDraw)
            || !ChosenSlice.Rows.IsValidIndex(RowIndex)) return false;
        const FCompiledAffixPoolRow &Row = *ChosenSlice.Rows[RowIndex];
        RemovedRows.Add(Row.PoolRowGuid);
        FMythicAffixGrantContext Context{Request.ItemInstanceGuid, Request.ItemLevel,
            Request.Rarity, Request.ContextTags, Request.Seed, Profile.ProfileId,
            Profile.Policy.PolicyId, ChosenSlice.Slice->PoolId, RollOrdinal};
        const FCompiledAffixTierProgression *Progression = nullptr;
        const FCompiledAffixTier *Tier = nullptr;
        uint64 TierDraw = 0;
        if (!PickTier(Row.Affix, ChosenSlice.Slice->SliceGuid,
                      EMythicAffixGrantTierMode::WeightedEligible, 0, Context,
                      RemainingBudget, Budget->bUnlimitedMagnitudeBudget,
                      Progression, Tier, &TierDraw)) return false;
        if (Decision) {
            Decision->SliceSelectionDraw = SliceDraw;
            Decision->RowSelectionDraw = RowDraw;
            Decision->TierSelectionDraw = TierDraw;
            Decision->SelectedSliceGuid = ChosenSlice.Slice->SliceGuid;
            Decision->SelectedPoolRowGuid = Row.PoolRowGuid;
            Decision->SelectedAffixDefinition = Row.Affix.Definition.Definition;
        }
        FRolledAffix Snapshot;
        if (!MaterializeCompiled(Row.Affix, *Progression, *Tier, Context, FGuid(),
                ChosenSlice.Slice->SliceGuid, Row.PoolRowGuid, Row.RollGroup,
                ChosenSlice.Slice->SourceKind, false, ChosenSlice.Slice->PoolRevision,
                Row.RowRevision, &Profile, nullptr, Snapshot, Decision)
            || Ledger.RollGuids.Contains(Snapshot.RollGuid)) return false;
        if (!Budget->bUnlimitedMagnitudeBudget) RemainingBudget -= Tier->BudgetCost;
        RollGroupUsed.FindOrAdd(Row.RollGroup)++;
        if (Decision) Decision->BudgetAfter = RemainingBudget;
        ReserveAffix(Row.Affix, Snapshot, Ledger);
        Temporary.Add(MoveTemp(Snapshot));
        ++RandomLanded;
        ++RollOrdinal;
    }
    if (RandomLanded < Budget->RandomRollCount
        && Profile.Policy.ShortfallMode == EMythicAffixShortfallMode::FailGeneration) return false;
    if (OptionalTrace && RandomLanded < Budget->RandomRollCount) {
        OptionalTrace->Diagnostics.Add(TEXT("AllowPartialShortfall"));
    }
    OutSnapshots = MoveTemp(Temporary);
    return true;
}
