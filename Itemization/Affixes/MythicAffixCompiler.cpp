#include "Itemization/Affixes/MythicAffixCompiler.h"

#include "Itemization/Affixes/MythicAffixRng.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Itemization/Affixes/MythicItemizationHash.h"
#include "Internationalization/Text.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "Stats/MythicStatRegistry.h"

#define LOCTEXT_NAMESPACE "MythicAffixCompiler"

namespace {
bool HasStableStringTableIdentity(const FText &Text) {
    FName TableId;
    FString Key;
    return !Text.IsEmpty() && FTextInspector::GetTableIdAndKey(Text, TableId, Key)
        && !TableId.IsNone() && !Key.IsEmpty();
}

void AddError(TArray<FText> &Errors, const FString &Message) {
    Errors.Add(FText::FromString(Message));
}

double RoundHalfAwayFromZero(const double Value) {
    return Value >= 0.0 ? FMath::FloorToDouble(Value + 0.5) : FMath::CeilToDouble(Value - 0.5);
}

bool AddCheckedPositiveWeight(const int64 Weight, int64 &InOutTotal) {
    if (Weight <= 0 || InOutTotal < 0 || Weight > MAX_int64 - InOutTotal) return false;
    InOutTotal += Weight;
    return true;
}

FMythicContentHash HashWriter(const FMythicAffixCanonicalWriter &Writer) {
    FMythicContentHash Result;
    if (!Writer.IsValid()) return Result;
    FSHA256Signature Digest{};
    if (!MythicItemizationHash::Sha256(Writer.GetBytes(), Digest)) return Result;
    auto Read64 = [](const uint8 *Bytes) {
        uint64 Value = 0;
        for (int32 Index = 7; Index >= 0; --Index) Value = (Value << 8) | Bytes[Index];
        return Value;
    };
    Result.Word0 = Read64(Digest.Signature);
    Result.Word1 = Read64(Digest.Signature + 8);
    Result.Word2 = Read64(Digest.Signature + 16);
    Result.Word3 = Read64(Digest.Signature + 24);
    return Result;
}

bool CanonicalBytesLess(const TArray<uint8> &A, const TArray<uint8> &B) {
    const int32 Shared = FMath::Min(A.Num(), B.Num());
    for (int32 Index = 0; Index < Shared; ++Index) {
        if (A[Index] != B[Index]) return A[Index] < B[Index];
    }
    return A.Num() < B.Num();
}

void AddQueryExpression(FMythicAffixCanonicalWriter &Writer,
                        const FGameplayTagQueryExpression &Expression) {
    Writer.AddUInt8(static_cast<uint8>(Expression.ExprType));
    if (Expression.UsesTagSet()) {
        TArray<FGameplayTag> Tags = Expression.TagSet;
        Tags.Sort([](const FGameplayTag &A, const FGameplayTag &B) {
            return A.ToString() < B.ToString();
        });
        Writer.AddUInt32(static_cast<uint32>(Tags.Num()));
        for (const FGameplayTag &Tag : Tags) Writer.AddName(Tag.GetTagName());
        return;
    }
    TArray<TArray<uint8>> Children;
    for (const FGameplayTagQueryExpression &Child : Expression.ExprSet) {
        FMythicAffixCanonicalWriter ChildWriter("MYTHIC_AFFIX_QUERY_NODE_V1");
        AddQueryExpression(ChildWriter, Child);
        Children.Add(ChildWriter.GetBytes());
    }
    Children.Sort([](const TArray<uint8> &A, const TArray<uint8> &B) {
        return CanonicalBytesLess(A, B);
    });
    Writer.AddUInt32(static_cast<uint32>(Children.Num()));
    for (const TArray<uint8> &Child : Children) Writer.AddBytes(Child);
}

void AddGameplayTagQuery(FMythicAffixCanonicalWriter &Writer, const FGameplayTagQuery &Query) {
    Writer.AddUInt8(Query.IsEmpty() ? 0 : 1);
    if (!Query.IsEmpty()) {
        FGameplayTagQueryExpression Expression;
        Query.GetQueryExpr(Expression);
        AddQueryExpression(Writer, Expression);
    }
}

void AddSortedTags(FMythicAffixCanonicalWriter &Writer,
                   const FGameplayTagContainer &Container) {
    TArray<FGameplayTag> Tags;
    for (const FGameplayTag &Tag : Container) Tags.Add(Tag);
    Tags.Sort([](const FGameplayTag &A, const FGameplayTag &B) {
        return A.ToString() < B.ToString();
    });
    Writer.AddUInt32(static_cast<uint32>(Tags.Num()));
    for (const FGameplayTag &Tag : Tags) Writer.AddName(Tag.GetTagName());
}

void AddCurveFingerprint(FMythicAffixCanonicalWriter &Writer, const FRichCurve &Curve) {
    Writer.AddUInt32(FMath::AsUInt(Curve.GetDefaultValue()));
    Writer.AddUInt8(static_cast<uint8>(Curve.PreInfinityExtrap.GetValue()));
    Writer.AddUInt8(static_cast<uint8>(Curve.PostInfinityExtrap.GetValue()));
    const TArray<FRichCurveKey> &Keys = Curve.GetConstRefOfKeys();
    Writer.AddUInt32(static_cast<uint32>(Keys.Num()));
    for (const FRichCurveKey &Key : Keys) {
        Writer.AddUInt8(static_cast<uint8>(Key.InterpMode.GetValue()));
        Writer.AddUInt8(static_cast<uint8>(Key.TangentMode.GetValue()));
        Writer.AddUInt8(static_cast<uint8>(Key.TangentWeightMode.GetValue()));
        Writer.AddUInt32(FMath::AsUInt(Key.Time));
        Writer.AddUInt32(FMath::AsUInt(Key.Value));
        Writer.AddUInt32(FMath::AsUInt(Key.ArriveTangent));
        Writer.AddUInt32(FMath::AsUInt(Key.ArriveTangentWeight));
        Writer.AddUInt32(FMath::AsUInt(Key.LeaveTangent));
        Writer.AddUInt32(FMath::AsUInt(Key.LeaveTangentWeight));
    }
}

bool EvaluateOpenEndedCurve(const FRichCurve &Curve, const float Level,
                            const float TailGrowth, float &OutValue) {
    if (Curve.GetNumKeys() <= 0 || !FMath::IsFinite(Level)
        || !FMath::IsFinite(TailGrowth) || TailGrowth < 1.0f) return false;
    float MinTime = 0.0f;
    float MaxTime = 0.0f;
    Curve.GetTimeRange(MinTime, MaxTime);
    if (!FMath::IsFinite(MinTime) || !FMath::IsFinite(MaxTime) || MinTime > MaxTime) return false;
    const float Base = Curve.Eval(FMath::Clamp(Level, MinTime, MaxTime));
    OutValue = Level <= MaxTime ? Base : Base * FMath::Pow(TailGrowth, Level - MaxTime);
    return FMath::IsFinite(OutValue) && OutValue >= 0.0f;
}

bool CurveRangeIsStrictlyPositive(const FRichCurve &Curve) {
    if (Curve.GetNumKeys() <= 0) return false;
    float Minimum = 0.0f;
    float Maximum = 0.0f;
    Curve.GetValueRange(Minimum, Maximum);
    return FMath::IsFinite(Minimum) && FMath::IsFinite(Maximum)
        && Minimum > 0.0f && Maximum >= Minimum;
}

float ContributionNeutral(const EGameplayModOp::Type Op, const float StatNeutral) {
    switch (Op) {
    case EGameplayModOp::MultiplyAdditive:
    case EGameplayModOp::DivideAdditive:
    case EGameplayModOp::MultiplyCompound:
        return 1.0f;
    case EGameplayModOp::Override:
        return StatNeutral;
    default:
        return 0.0f;
    }
}

bool IsNoOpBand(const FCompiledAffixMagnitudeBand &Band, const float Neutral) {
    if (Band.ScaleMode != EMythicAffixScaleMode::None) return false;
    return FMath::IsNearlyEqual(Band.Min, Neutral, SMALL_NUMBER)
        && FMath::IsNearlyEqual(Band.Max, Neutral, SMALL_NUMBER);
}

bool CompileDefinitionInternal(const FMythicAffixDefinitionHandle &Handle,
                               const UMythicItemizationDataRegistrySubsystem &Registry,
                               FCompiledAffix &Out, TArray<FText> &Errors) {
    Out = FCompiledAffix();
    const FPrimaryAssetId DefinitionId = Handle.GetPrimaryAssetId();
    const UMythicAffixDefinition *Definition = Registry.FindAffix(DefinitionId);
    if (!Definition) {
        AddError(Errors, FString::Printf(TEXT("Affix Definition reference %s is not resident in the registry."),
                                         *Handle.Asset.ToSoftObjectPath().ToString()));
        return false;
    }
    if (!FMythicAffixCompiler::ValidateDefinitionAuthoring(
            *Definition, Registry.GetStatRegistry(), Errors)) return false;

    const UMythicStatDefinition *Stat = Registry.FindStat(Definition->TargetStat.GetPrimaryAssetId());
    const UMythicStatCategoryDefinition *Category = Stat
        ? Registry.GetStatRegistry().FindCategory(Stat->Category.GetPrimaryAssetId()) : nullptr;
    if (!Stat || !Category) {
        AddError(Errors, FString::Printf(TEXT("Affix Definition %s has an unresolved Stat Definition/category."),
                                         *Definition->GetName()));
        return false;
    }

    FCompiledAffixDefinition &CompiledDefinition = Out.Definition;
    CompiledDefinition.Definition = Handle;
    CompiledDefinition.DefinitionId = DefinitionId;
    CompiledDefinition.AffixTag = Definition->AffixTag;
    CompiledDefinition.Revision = Definition->Revision;
    CompiledDefinition.PresentationRevision = Definition->PresentationRevision;
    CompiledDefinition.TargetStatId = Stat->GetPrimaryAssetId();
    CompiledDefinition.TargetCategoryId = Category->GetPrimaryAssetId();
    CompiledDefinition.TargetStatTag = Stat->StatTag;
    CompiledDefinition.TargetAttribute = Stat->Attribute;
    CompiledDefinition.TargetStatRevision = Stat->Revision;
    CompiledDefinition.TargetStatPresentationRevision = Stat->PresentationRevision;
    CompiledDefinition.TargetCategoryPresentationRevision = Category->PresentationRevision;
    CompiledDefinition.ModifierOp = Definition->ModifierOp;
    CompiledDefinition.Quantization = Definition->Quantization;
    CompiledDefinition.ComparisonDirection = Stat->ComparisonDirection;
    CompiledDefinition.NeutralValue = Stat->NeutralValue;
    CompiledDefinition.StackingGroup = Definition->GetEffectiveStackingGroup();
    CompiledDefinition.StackingRule = Definition->StackingRule;
    CompiledDefinition.ConflictGroups = Definition->ConflictGroups;

    for (const FMythicAffixTierProgressionDefinition &SourceProgression : Definition->TierProgressions) {
        FCompiledAffixTierProgression Progression;
        Progression.DeveloperName = SourceProgression.DeveloperName;
        Progression.TuningContext = SourceProgression.TuningContext;
        Progression.ApplicabilityQuery = SourceProgression.ApplicabilityQuery;
        Progression.SelectionPriority = SourceProgression.SelectionPriority;
        int64 TotalTierWeight = 0;
        for (int32 Index = 0; Index < SourceProgression.Tiers.Num(); ++Index) {
            const FMythicAffixTierDefinition &SourceTier = SourceProgression.Tiers[Index];
            FCompiledAffixTier Tier;
            Tier.TierRank = Index + 1;
            Tier.DeveloperName = SourceTier.DeveloperName;
            Tier.PresentationRevision = SourceTier.PresentationRevision;
            Tier.MinItemLevel = SourceTier.MinItemLevel;
            if (!FMythicAffixCompiler::TryCompileFixedPoint(SourceTier.TierWeight, true,
                                                            Tier.TierWeight)
                || !FMythicAffixCompiler::TryCompileFixedPoint(SourceTier.BudgetCost, false,
                                                               Tier.BudgetCost)
                || !AddCheckedPositiveWeight(Tier.TierWeight, TotalTierWeight)
                || !FMythicAffixCompiler::CompileMagnitudeBand(SourceTier.Magnitude,
                                                               Tier.Magnitude, Errors)) {
                AddError(Errors, FString::Printf(TEXT("Affix %s progression %s tier %d failed compilation."),
                    *Definition->GetName(), *SourceProgression.DeveloperName.ToString(), Index + 1));
                return false;
            }
            float TestMin = 0.0f;
            float TestMax = 0.0f;
            if (!Tier.Magnitude.Resolve(Tier.MinItemLevel, TestMin, TestMax)) return false;
            float QuantizedMin = Definition->Quantization.Apply(TestMin);
            float QuantizedMax = Definition->Quantization.Apply(TestMax);
            if (QuantizedMin > QuantizedMax) Swap(QuantizedMin, QuantizedMax);
            if (!FMath::IsFinite(QuantizedMin) || !FMath::IsFinite(QuantizedMax)) {
                AddError(Errors, FString::Printf(
                    TEXT("Affix %s tier %d cannot produce a finite value after rounding."),
                    *Definition->GetName(), Index + 1));
                return false;
            }
            if (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)) {
                const bool bPositiveCurve = Tier.Magnitude.ScaleMode != EMythicAffixScaleMode::Curve
                    || CurveRangeIsStrictlyPositive(Tier.Magnitude.LevelScalingCurve);
                if (TestMin <= 0.0f || TestMax <= 0.0f || QuantizedMin <= 0.0f
                    || QuantizedMax <= 0.0f || !bPositiveCurve) {
                    AddError(Errors, FString::Printf(
                        TEXT("Affix %s tier %d can reach zero before or after rounding for a multiplicative/divisive operation."),
                        *Definition->GetName(), Index + 1));
                    return false;
                }
            }
            const float Neutral = ContributionNeutral(Definition->ModifierOp, Stat->NeutralValue);
            if (IsNoOpBand(Tier.Magnitude, Neutral)
                || (QuantizedMin <= Neutral && QuantizedMax >= Neutral)) {
                AddError(Errors, FString::Printf(TEXT("Affix %s tier %d can materialize a semantic no-op."),
                                                 *Definition->GetName(), Index + 1));
                return false;
            }
            Progression.Tiers.Add(MoveTemp(Tier));
        }
        Out.Progressions.Add(MoveTemp(Progression));
    }
    Out.Progressions.Sort([](const FCompiledAffixTierProgression &A,
                             const FCompiledAffixTierProgression &B) {
        if (A.SelectionPriority != B.SelectionPriority) return A.SelectionPriority > B.SelectionPriority;
        return A.DeveloperName.LexicalLess(B.DeveloperName);
    });
    return !Out.Progressions.IsEmpty();
}

void AddDefinitionHash(FMythicAffixCanonicalWriter &Gameplay,
                       FMythicAffixCanonicalWriter &Presentation,
                       const FCompiledAffix &Affix) {
    const FCompiledAffixDefinition &Definition = Affix.Definition;
    Gameplay.AddPrimaryAssetId(Definition.DefinitionId);
    Gameplay.AddName(Definition.AffixTag.GetTagName());
    Gameplay.AddInt32(Definition.Revision);
    Gameplay.AddPrimaryAssetId(Definition.TargetStatId);
    Gameplay.AddName(Definition.TargetStatTag.GetTagName());
    Gameplay.AddString(Definition.TargetAttribute.GetAttributeSetClass()
        ? Definition.TargetAttribute.GetAttributeSetClass()->GetPathName() : FString());
    Gameplay.AddString(Definition.TargetAttribute.GetName());
    Gameplay.AddInt32(Definition.TargetStatRevision);
    Gameplay.AddUInt8(static_cast<uint8>(Definition.ModifierOp));
    Gameplay.AddUInt8(static_cast<uint8>(Definition.Quantization.Mode));
    Gameplay.AddUInt32(FMath::AsUInt(Definition.Quantization.Step));
    Gameplay.AddUInt8(static_cast<uint8>(Definition.ComparisonDirection));
    Gameplay.AddUInt32(FMath::AsUInt(Definition.NeutralValue));
    Gameplay.AddName(Definition.StackingGroup.GetTagName());
    Gameplay.AddUInt8(static_cast<uint8>(Definition.StackingRule));
    AddSortedTags(Gameplay, Definition.ConflictGroups);
    Presentation.AddPrimaryAssetId(Definition.DefinitionId);
    Presentation.AddInt32(Definition.PresentationRevision);
    Presentation.AddPrimaryAssetId(Definition.TargetStatId);
    Presentation.AddPrimaryAssetId(Definition.TargetCategoryId);
    Presentation.AddInt32(Definition.TargetStatPresentationRevision);
    Presentation.AddInt32(Definition.TargetCategoryPresentationRevision);
    Gameplay.AddUInt32(static_cast<uint32>(Affix.Progressions.Num()));
    for (const FCompiledAffixTierProgression &Progression : Affix.Progressions) {
        Gameplay.AddName(Progression.DeveloperName);
        Gameplay.AddName(Progression.TuningContext);
        Gameplay.AddInt32(Progression.SelectionPriority);
        AddGameplayTagQuery(Gameplay, Progression.ApplicabilityQuery);
        Gameplay.AddUInt32(static_cast<uint32>(Progression.Tiers.Num()));
        for (const FCompiledAffixTier &Tier : Progression.Tiers) {
            Gameplay.AddInt32(Tier.TierRank);
            Gameplay.AddInt32(Tier.MinItemLevel);
            Gameplay.AddUInt64(static_cast<uint64>(Tier.TierWeight));
            Gameplay.AddUInt64(static_cast<uint64>(Tier.BudgetCost));
            Gameplay.AddUInt32(FMath::AsUInt(Tier.Magnitude.Min));
            Gameplay.AddUInt32(FMath::AsUInt(Tier.Magnitude.Max));
            Gameplay.AddUInt8(static_cast<uint8>(Tier.Magnitude.ScaleMode));
            Gameplay.AddUInt32(FMath::AsUInt(Tier.Magnitude.LinearPerItemLevel));
            Gameplay.AddUInt32(FMath::AsUInt(Tier.Magnitude.CurveTailGrowth));
            AddCurveFingerprint(Gameplay, Tier.Magnitude.LevelScalingCurve);
            Presentation.AddName(Tier.DeveloperName);
            Presentation.AddInt32(Tier.PresentationRevision);
        }
    }
}

bool FindContextIndependentTierCost(const FCompiledAffix &Affix,
                                    const int32 ExactTierRank,
                                    int64 &OutWorstCaseCost) {
    // Profiles currently accept every item level >= 1 and do not declare a finite set of context-tag
    // combinations. Weighted selection is proved against level one; explicit exact ranks bypass that
    // availability gate. In both modes take the most expensive minimum across every progression that an
    // arbitrary context could select. Taking the cheapest tier across progressions is optimistic: a
    // conditional progression may be cheap while the fallback is not.
    OutWorstCaseCost = 0;
    if (Affix.Progressions.IsEmpty()) return false;
    for (const FCompiledAffixTierProgression &Progression : Affix.Progressions) {
        int64 ProgressionMinimum = MAX_int64;
        for (const FCompiledAffixTier &Tier : Progression.Tiers) {
            if (ExactTierRank > 0 && Tier.TierRank != ExactTierRank) continue;
            // Exact grants are explicit content overrides and are intentionally not
            // constrained by random-roll item-level availability.
            if (ExactTierRank <= 0 && Tier.MinItemLevel > 1) continue;
            ProgressionMinimum = FMath::Min(ProgressionMinimum, Tier.BudgetCost);
        }
        if (ProgressionMinimum == MAX_int64) return false;
        OutWorstCaseCost = FMath::Max(OutWorstCaseCost, ProgressionMinimum);
    }
    return true;
}

bool HasExactRankEverywhere(const FCompiledAffix &Affix, const int32 Rank) {
    if (Rank <= 0 || Affix.Progressions.IsEmpty()) return false;
    for (const FCompiledAffixTierProgression &Progression : Affix.Progressions) {
        if (!Progression.Tiers.ContainsByPredicate([Rank](const FCompiledAffixTier &Tier) {
                return Tier.TierRank == Rank;
            })) return false;
    }
    return true;
}

struct FFeasibilityState {
    TSet<FPrimaryAssetId> Definitions;
    TSet<FPrimaryAssetId> Stats;
    FGameplayTagContainer Conflicts;
    TSet<FGameplayTag> ExclusiveStackingGroups;
    TMap<FGameplayTag, int32> RollGroupCounts;
    TSet<FGuid> SelectedPoolRows;
    int64 RemainingBudget = MAX_int64;
};

struct FFeasibilityRow {
    const FCompiledAffixPoolRow *Row = nullptr;
    int32 SlicePriority = 0;
    int64 ContextIndependentCost = MAX_int64;
    bool bContextIndependent = false;
};

bool CanAccept(const FCompiledAffix &Affix, const FCompiledAffixPolicy &Policy,
               const FFeasibilityState &State) {
    const FCompiledAffixDefinition &Definition = Affix.Definition;
    if (Policy.bDisallowDuplicateAffixDefinition
        && State.Definitions.Contains(Definition.DefinitionId)) return false;
    if (Policy.bDisallowDuplicateTargetStat && State.Stats.Contains(Definition.TargetStatId)) return false;
    if (Definition.ConflictGroups.HasAny(State.Conflicts)) return false;
    if (Definition.StackingRule != EMythicAffixStackingRule::StackAll
        && Definition.StackingGroup.IsValid()
        && State.ExclusiveStackingGroups.Contains(Definition.StackingGroup)) return false;
    return true;
}

void Reserve(const FCompiledAffix &Affix, FFeasibilityState &State) {
    State.Definitions.Add(Affix.Definition.DefinitionId);
    State.Stats.Add(Affix.Definition.TargetStatId);
    State.Conflicts.AppendTags(Affix.Definition.ConflictGroups);
    if (Affix.Definition.StackingRule != EMythicAffixStackingRule::StackAll
        && Affix.Definition.StackingGroup.IsValid()) {
        State.ExclusiveStackingGroups.Add(Affix.Definition.StackingGroup);
    }
}

bool SearchRows(const TArray<FFeasibilityRow> &Rows,
                const FCompiledAffixPolicy &Policy,
                const FCompiledAffixRarityBudget &Budget,
                const int32 Needed, const int32 Start,
                FFeasibilityState State) {
    if (Needed <= 0) return true;

    int32 HighestConfiguredPriority = MIN_int32;
    int32 HighestEligiblePriority = MIN_int32;
    for (const FFeasibilityRow &Candidate : Rows) {
        if (!Candidate.Row || State.SelectedPoolRows.Contains(Candidate.Row->PoolRowGuid)) continue;
        HighestConfiguredPriority = FMath::Max(HighestConfiguredPriority, Candidate.SlicePriority);
        if (!Candidate.bContextIndependent) continue;
        const FCompiledAffixPoolRow &Row = *Candidate.Row;
        const int32 Cap = Budget.RollGroupCaps.FindRef(Row.RollGroup);
        if (Cap <= 0 || State.RollGroupCounts.FindRef(Row.RollGroup) >= Cap
            || !CanAccept(Row.Affix, Policy, State)) continue;
        if (Candidate.ContextIndependentCost == MAX_int64
            || (!Budget.bUnlimitedMagnitudeBudget
                && Candidate.ContextIndependentCost > State.RemainingBudget)) continue;
        HighestEligiblePriority = FMath::Max(HighestEligiblePriority, Candidate.SlicePriority);
    }
    if (HighestEligiblePriority == MIN_int32) return false;

    // Generation refuses to descend while any unremoved row remains at a higher configured priority unless the
    // policy explicitly enables lower-priority fallback. Include conditional rows in HighestConfiguredPriority:
    // in a context where their query does not match they still block that descent at runtime.
    if (!Policy.bAllowLowerPriorityFallback
        && HighestEligiblePriority < HighestConfiguredPriority) return false;

    for (int32 Index = Start; Index < Rows.Num(); ++Index) {
        const FFeasibilityRow &Candidate = Rows[Index];
        if (!Candidate.Row || !Candidate.bContextIndependent
            || Candidate.SlicePriority != HighestEligiblePriority
            || State.SelectedPoolRows.Contains(Candidate.Row->PoolRowGuid)) continue;
        const FCompiledAffixPoolRow &Row = *Candidate.Row;
        const int32 Cap = Budget.RollGroupCaps.FindRef(Row.RollGroup);
        if (Cap <= 0 || State.RollGroupCounts.FindRef(Row.RollGroup) >= Cap
            || !CanAccept(Row.Affix, Policy, State)) continue;
        const int64 Cost = Candidate.ContextIndependentCost;
        if (Cost == MAX_int64
            || (!Budget.bUnlimitedMagnitudeBudget && Cost > State.RemainingBudget)) continue;
        FFeasibilityState Next = State;
        Reserve(Row.Affix, Next);
        Next.SelectedPoolRows.Add(Row.PoolRowGuid);
        Next.RollGroupCounts.FindOrAdd(Row.RollGroup)++;
        if (!Budget.bUnlimitedMagnitudeBudget) Next.RemainingBudget -= Cost;
        if (SearchRows(Rows, Policy, Budget, Needed - 1, Index + 1, MoveTemp(Next))) return true;
    }
    return false;
}
}

bool FCompiledAffixMagnitudeBand::Resolve(const int32 ItemLevel, float &OutMin,
                                          float &OutMax) const {
    const float Level = static_cast<float>(FMath::Max(1, ItemLevel));
    switch (ScaleMode) {
    case EMythicAffixScaleMode::None:
        OutMin = Min;
        OutMax = Max;
        break;
    case EMythicAffixScaleMode::Linear:
        OutMin = Min + LinearPerItemLevel * Level;
        OutMax = Max + LinearPerItemLevel * Level;
        break;
    case EMythicAffixScaleMode::Curve: {
        float Scale = 0.0f;
        if (!EvaluateOpenEndedCurve(LevelScalingCurve, Level, CurveTailGrowth, Scale)) return false;
        OutMin = Min * Scale;
        OutMax = Max * Scale;
        break;
    }
    default:
        return false;
    }
    return FMath::IsFinite(OutMin) && FMath::IsFinite(OutMax)
        && OutMin >= 0.0f && OutMax >= OutMin;
}

const FCompiledAffixTierProgression *FCompiledAffix::ResolveProgression(
    const FGameplayTagContainer &ContextTags) const {
    const FCompiledAffixTierProgression *Winner = nullptr;
    const FCompiledAffixTierProgression *Fallback = nullptr;
    int32 Priority = MIN_int32;
    bool bTied = false;
    for (const FCompiledAffixTierProgression &Progression : Progressions) {
        if (Progression.ApplicabilityQuery.IsEmpty()) {
            if (Fallback) return nullptr;
            Fallback = &Progression;
            continue;
        }
        if (!Progression.ApplicabilityQuery.Matches(ContextTags)) continue;
        if (!Winner || Progression.SelectionPriority > Priority) {
            Winner = &Progression;
            Priority = Progression.SelectionPriority;
            bTied = false;
        }
        else if (Progression.SelectionPriority == Priority) {
            bTied = true;
        }
    }
    return bTied ? nullptr : (Winner ? Winner : Fallback);
}

bool FMythicAffixCompiler::TryCompileFixedPoint(const double Value,
                                               const bool bRequirePositive,
                                               int64 &OutValue) {
    if (!FMath::IsFinite(Value) || Value < 0.0 || (bRequirePositive && Value <= 0.0)) return false;
    const double Scaled = Value * static_cast<double>(FixedPointScale);
    if (!FMath::IsFinite(Scaled) || Scaled > static_cast<double>(MAX_int64)) return false;
    OutValue = static_cast<int64>(RoundHalfAwayFromZero(Scaled));
    return bRequirePositive ? OutValue > 0 : OutValue >= 0;
}

bool FMythicAffixCompiler::CompileMagnitudeBand(const FMythicAffixMagnitudeBand &Source,
                                                FCompiledAffixMagnitudeBand &OutBand,
                                                TArray<FText> &OutErrors) {
    OutBand = FCompiledAffixMagnitudeBand();
    if (!FMath::IsFinite(Source.Min) || !FMath::IsFinite(Source.Max)
        || Source.Min < 0.0f || Source.Max < Source.Min) {
        AddError(OutErrors, TEXT("Affix magnitude range must be finite, non-negative, and ordered."));
        return false;
    }
    OutBand.Min = Source.Min;
    OutBand.Max = Source.Max;
    OutBand.ScaleMode = Source.ScaleMode;
    OutBand.LinearPerItemLevel = Source.LinearPerItemLevel;
    OutBand.CurveTailGrowth = Source.CurveTailGrowth;
    if (Source.ScaleMode == EMythicAffixScaleMode::Linear
        && (!FMath::IsFinite(Source.LinearPerItemLevel)
            || Source.LinearPerItemLevel <= 0.0f)) {
        AddError(OutErrors, TEXT("Linear affix scaling requires a finite positive per-level amount."));
        return false;
    }
    if (Source.ScaleMode == EMythicAffixScaleMode::Curve) {
        if (Source.LevelScalingCurve.ExternalCurve != nullptr) {
            AddError(OutErrors,
                     TEXT("Curve affix scaling must be owned inline by its Affix Definition; external curve assets are rejected."));
            return false;
        }
        if (!FMath::IsFinite(Source.CurveTailGrowth) || Source.CurveTailGrowth < 1.0f) {
            AddError(OutErrors, TEXT("Curve affix scaling requires tail growth of at least one."));
            return false;
        }
        const FRichCurve *Curve = Source.LevelScalingCurve.GetRichCurveConst();
        if (!Curve || Curve->GetNumKeys() <= 0) {
            AddError(OutErrors, TEXT("Curve affix scaling requires an embedded curve with keys."));
            return false;
        }
        OutBand.LevelScalingCurve = *Curve;
    }
    return Source.ScaleMode == EMythicAffixScaleMode::None
        || Source.ScaleMode == EMythicAffixScaleMode::Linear
        || Source.ScaleMode == EMythicAffixScaleMode::Curve;
}

bool FMythicAffixCompiler::ValidateDefinitionAuthoring(
    const UMythicAffixDefinition &Definition, const FMythicStatRegistry &StatRegistry,
    TArray<FText> &OutErrors) {
    bool bValid = Definition.GetPrimaryAssetId().IsValid() && Definition.AffixTag.IsValid()
        && !Definition.DeveloperName.IsNone() && !Definition.DesignerPurpose.TrimStartAndEnd().IsEmpty()
        && Definition.Revision > 0 && Definition.PresentationRevision > 0
        && HasStableStringTableIdentity(Definition.DisplayNameTemplate)
        && (Definition.DescriptionTemplate.IsEmpty()
            || HasStableStringTableIdentity(Definition.DescriptionTemplate))
        && Definition.TargetStat.IsValid() && Definition.Quantization.IsValid()
        && MythicAffix::IsSupportedModifierOp(Definition.ModifierOp)
        && !Definition.TierProgressions.IsEmpty();
    const UEnum *StackingRuleEnum = StaticEnum<EMythicAffixStackingRule>();
    bValid &= StackingRuleEnum
        && StackingRuleEnum->IsValidEnumValue(static_cast<int64>(Definition.StackingRule));
    const UMythicStatDefinition *Stat = StatRegistry.FindStat(
        Definition.TargetStat.GetPrimaryAssetId());
    bValid &= Stat && Stat->bCanBeAffixTarget && Stat->Attribute.IsValid();
    bValid &= Definition.StackingRule == EMythicAffixStackingRule::StackAll
        || Definition.GetEffectiveStackingGroup().IsValid();
    if (Stat && (Definition.StackingRule == EMythicAffixStackingRule::HighestPerItem
                 || Definition.StackingRule == EMythicAffixStackingRule::HighestOverall)) {
        bValid &= Stat->ComparisonDirection != EMythicStatComparisonDirection::Neutral;
    }
    TSet<int32> Priorities;
    int32 Fallbacks = 0;
    for (const FMythicAffixTierProgressionDefinition &Progression : Definition.TierProgressions) {
        if (Progression.ApplicabilityQuery.IsEmpty()) ++Fallbacks;
        if (Progression.DeveloperName.IsNone() || Progression.TuningContext.IsNone()
            || Progression.Tiers.IsEmpty() || Priorities.Contains(Progression.SelectionPriority)) {
            bValid = false;
        }
        Priorities.Add(Progression.SelectionPriority);
        int32 PreviousLevel = 0;
        for (int32 TierIndex = 0; TierIndex < Progression.Tiers.Num(); ++TierIndex) {
            const FMythicAffixTierDefinition &Tier = Progression.Tiers[TierIndex];
            FCompiledAffixMagnitudeBand Ignored;
            if (Tier.DeveloperName.IsNone() || Tier.PresentationRevision < 1
                || (TierIndex == 0 && Tier.MinItemLevel != 1)
                || Tier.MinItemLevel <= PreviousLevel || !FMath::IsFinite(Tier.TierWeight)
                || Tier.TierWeight <= 0.0f || !FMath::IsFinite(Tier.BudgetCost)
                || Tier.BudgetCost < 0.0f
                || (!Tier.DisplayName.IsEmpty() && !HasStableStringTableIdentity(Tier.DisplayName))
                || !CompileMagnitudeBand(Tier.Magnitude, Ignored, OutErrors)) {
                bValid = false;
            }
            PreviousLevel = Tier.MinItemLevel;
        }
    }
    if (Fallbacks != 1) bValid = false;
    if (!bValid) {
        AddError(OutErrors, FString::Printf(
            TEXT("Affix Definition %s is invalid. It needs stable localized text, one typed affixable stat, supported operation/stacking semantics, unique progression priorities, exactly one fallback progression, and ordered valid tiers."),
            *Definition.GetName()));
    }
    return bValid;
}

bool FMythicAffixCompiler::ValidatePoolAuthoring(const UMythicAffixPool &Pool,
                                                 TArray<FText> &OutErrors) {
    bool bValid = Pool.GetPrimaryAssetId().IsValid() && !Pool.DeveloperName.IsNone()
        && !Pool.DesignerPurpose.TrimStartAndEnd().IsEmpty() && Pool.Revision > 0
        && !Pool.Entries.IsEmpty();
    TSet<FGuid> Guids;
    TSet<FMythicAffixDefinitionHandle> Definitions;
    for (const FMythicAffixPoolEntry &Row : Pool.Entries) {
        if (!Row.PoolRowGuid.IsValid() || Guids.Contains(Row.PoolRowGuid)
            || !Row.AffixDefinition.IsValid() || Definitions.Contains(Row.AffixDefinition)
            || Row.DeveloperName.IsNone() || Row.RowRevision < 1
            || !Row.RollGroup.IsValid() || !FMath::IsFinite(Row.SelectionWeight)
            || Row.SelectionWeight <= 0.0f) bValid = false;
        Guids.Add(Row.PoolRowGuid);
        Definitions.Add(Row.AffixDefinition);
    }
    if (!bValid) AddError(OutErrors, FString::Printf(TEXT("Affix Pool %s has invalid or duplicate rows."),
                                                     *Pool.GetName()));
    return bValid;
}

bool FMythicAffixCompiler::CompileDefinition(
    const FMythicAffixDefinitionHandle &Handle,
    const UMythicItemizationDataRegistrySubsystem &Registry,
    FCompiledAffix &OutAffix, TArray<FText> &OutErrors) {
    return CompileDefinitionInternal(Handle, Registry, OutAffix, OutErrors);
}

bool FMythicAffixCompiler::CompileGrant(
    const FMythicAffixGrantSpec &Spec,
    const UMythicItemizationDataRegistrySubsystem &Registry,
    TSharedPtr<const FCompiledAffixGrantClosure> &OutGrant,
    TArray<FText> &OutErrors) {
    OutGrant.Reset();
    TSharedRef<FCompiledAffixGrantClosure> Built = MakeShared<FCompiledAffixGrantClosure>();
    Built->Spec = Spec;
    if (!Spec.GrantGuid.IsValid() || Spec.DeveloperName.IsNone()
        || !Spec.RollGroup.IsValid() || !Spec.SourceKind.IsValid()
        || !MythicAffixGrant::IsTierSelectionValid(Spec.TierMode, Spec.ExactTierRank)
        || !CompileDefinitionInternal(Spec.AffixDefinition, Registry, Built->Affix, OutErrors)
        || (Spec.TierMode == EMythicAffixGrantTierMode::ExactTier
            && !HasExactRankEverywhere(Built->Affix, Spec.ExactTierRank))) {
        AddError(OutErrors, TEXT("Exact affix grant has invalid identity, source, definition, or tier rank."));
        return false;
    }
    FMythicAffixCanonicalWriter Gameplay("MYTHIC_AFFIX_GRANT_GAMEPLAY_HASH_V4");
    FMythicAffixCanonicalWriter Presentation("MYTHIC_AFFIX_GRANT_PRESENTATION_HASH_V4");
    Gameplay.AddGuid(Spec.GrantGuid);
    Gameplay.AddUInt8(static_cast<uint8>(Spec.TierMode));
    Gameplay.AddInt32(Spec.ExactTierRank);
    Gameplay.AddName(Spec.RollGroup.GetTagName());
    Gameplay.AddName(Spec.SourceKind.GetTagName());
    Gameplay.AddUInt8(Spec.bLocked ? 1 : 0);
    AddDefinitionHash(Gameplay, Presentation, Built->Affix);
    Built->GameplayContentHash = HashWriter(Gameplay);
    Built->PresentationContentHash = HashWriter(Presentation);
    if (Built->GameplayContentHash.IsZero() || Built->PresentationContentHash.IsZero()) return false;
    OutGrant = Built;
    return true;
}

bool FMythicAffixCompiler::ValidateStructuralFeasibility(
    const FCompiledAffixProfile &Profile, TArray<FText> &OutErrors) {
    bool bValid = true;
    for (uint8 RarityByte = static_cast<uint8>(EItemRarity::Common);
         RarityByte <= static_cast<uint8>(EItemRarity::Mythic); ++RarityByte) {
        const EItemRarity Rarity = static_cast<EItemRarity>(RarityByte);
        const FCompiledAffixRarityBudget *Budget = Profile.Policy.Budgets.Find(Rarity);
        if (!Budget) {
            AddError(OutErrors, FString::Printf(TEXT("Profile %s has no budget for rarity %d."),
                *Profile.ProfileId.ToString(), RarityByte));
            bValid = false;
            continue;
        }
        FFeasibilityState State;
        State.RemainingBudget = Budget->bUnlimitedMagnitudeBudget ? MAX_int64 : Budget->MagnitudeBudget;
        for (const FCompiledAffixGrant &Grant : Profile.GuaranteedGrants) {
            if (!CanAccept(Grant.Affix, Profile.Policy, State)) {
                AddError(OutErrors, TEXT("Guaranteed affixes violate duplicate-stat, stacking-group, or conflict rules."));
                bValid = false;
                break;
            }
            const int32 ExactRank = Grant.Spec.TierMode == EMythicAffixGrantTierMode::ExactTier
                ? Grant.Spec.ExactTierRank : 0;
            int64 Cost = MAX_int64;
            if (!FindContextIndependentTierCost(Grant.Affix, ExactRank, Cost)
                || (Profile.Policy.bGuaranteedConsumesMagnitudeBudget
                && !Budget->bUnlimitedMagnitudeBudget && Cost > State.RemainingBudget)) {
                AddError(OutErrors, TEXT("Guaranteed affix has no selectable tier in every contextual progression, or cannot fit the worst-case rarity magnitude budget."));
                bValid = false;
                break;
            }
            if (Profile.Policy.bGuaranteedConsumesMagnitudeBudget && !Budget->bUnlimitedMagnitudeBudget)
                State.RemainingBudget -= Cost;
            Reserve(Grant.Affix, State);
        }
        if (!bValid) continue;

        TArray<FFeasibilityRow> Rows;
        for (const FCompiledAffixSlice &Slice : Profile.RandomSlices) {
            for (const FCompiledAffixPoolRow &Row : Slice.Rows) {
                FFeasibilityRow &FeasibilityRow = Rows.AddDefaulted_GetRef();
                FeasibilityRow.Row = &Row;
                FeasibilityRow.SlicePriority = Slice.Priority;
                FeasibilityRow.bContextIndependent = Row.EligibilityQuery.IsEmpty()
                    && FindContextIndependentTierCost(
                        Row.Affix, 0, FeasibilityRow.ContextIndependentCost);
            }
        }
        Rows.Sort([](const FFeasibilityRow &Left, const FFeasibilityRow &Right) {
            if (Left.SlicePriority != Right.SlicePriority) {
                return Left.SlicePriority > Right.SlicePriority;
            }
            return Left.Row && Right.Row
                ? Left.Row->PoolRowGuid < Right.Row->PoolRowGuid
                : Left.Row != nullptr;
        });

        // AllowPartial explicitly accepts fewer random affixes. FailGeneration publishes no partial result, so its
        // profile needs at least one full-count structural path for every supported item context. Profiles do not yet
        // declare a finite context-tag matrix, so conditional rows cannot establish that path; they may enrich
        // generation, but an unconditional backbone must independently satisfy caps, conflicts, priorities and the
        // worst contextual tier cost.
        if (Profile.Policy.ShortfallMode == EMythicAffixShortfallMode::FailGeneration
            && !SearchRows(Rows, Profile.Policy, *Budget,
                           Budget->RandomRollCount, 0, State)) {
            AddError(OutErrors, FString::Printf(
                TEXT("FailGeneration profile %s cannot prove a context-independent backbone for %d random rolls at rarity %d. Rows in the backbone need empty eligibility queries, a tier available from item level 1 in every contextual progression, sufficient worst-case magnitude budget, and slice priorities compatible with the lower-priority fallback policy."),
                *Profile.ProfileId.ToString(), Budget->RandomRollCount, RarityByte));
            bValid = false;
        }
    }
    return bValid;
}

bool FMythicAffixCompiler::Compile(
    const UMythicAffixProfile &Profile,
    const UMythicItemizationDataRegistrySubsystem &Registry,
    TSharedPtr<const FCompiledAffixProfile> &OutProfile,
    TArray<FText> &OutErrors) {
    OutProfile.Reset();
    TSharedRef<FCompiledAffixProfile> Built = MakeShared<FCompiledAffixProfile>();
    Built->ProfileId = Profile.GetPrimaryAssetId();
    Built->ProfileRevision = Profile.Revision;
    const UMythicAffixRollPolicy *Policy = Registry.FindPolicy(Profile.RollPolicy.GetPrimaryAssetId());
    if (!Built->ProfileId.IsValid() || Profile.DeveloperName.IsNone()
        || Profile.DesignerPurpose.TrimStartAndEnd().IsEmpty() || Profile.Revision < 1
        || !Policy || Policy->DeveloperName.IsNone()
        || Policy->DesignerPurpose.TrimStartAndEnd().IsEmpty()
        || Policy->Revision < 1 || Policy->AlgorithmVersion != 1) {
        AddError(OutErrors, TEXT("Profile or its directly referenced roll policy is invalid."));
        return false;
    }
    const UEnum *ShortfallModeEnum = StaticEnum<EMythicAffixShortfallMode>();
    if (!ShortfallModeEnum
        || !ShortfallModeEnum->IsValidEnumValue(static_cast<int64>(Policy->ShortfallMode))) {
        AddError(OutErrors, TEXT("Roll policy has an invalid shortfall mode."));
        return false;
    }
    Built->Policy.PolicyId = Policy->GetPrimaryAssetId();
    Built->Policy.Revision = Policy->Revision;
    Built->Policy.AlgorithmVersion = Policy->AlgorithmVersion;
    Built->Policy.bGuaranteedConsumesMagnitudeBudget = Policy->bGuaranteedConsumesMagnitudeBudget;
    Built->Policy.bDisallowDuplicateAffixDefinition = Policy->bDisallowDuplicateAffixDefinition;
    Built->Policy.bDisallowDuplicateTargetStat = Policy->bDisallowDuplicateTargetStat;
    Built->Policy.ShortfallMode = Policy->ShortfallMode;
    Built->Policy.bAllowLowerPriorityFallback = Policy->bAllowLowerPriorityFallback;
    for (const FMythicAffixRarityBudget &Source : Policy->BudgetsByRarity) {
        const uint8 RarityValue = static_cast<uint8>(Source.Rarity.GetValue());
        if (RarityValue < static_cast<uint8>(EItemRarity::Common)
            || RarityValue > static_cast<uint8>(EItemRarity::Mythic)) return false;
        if (Built->Policy.Budgets.Contains(Source.Rarity.GetValue())) return false;
        FCompiledAffixRarityBudget Budget;
        Budget.Rarity = Source.Rarity.GetValue();
        Budget.RandomRollCount = Source.RandomRollCount;
        Budget.bUnlimitedMagnitudeBudget = Source.bUnlimitedMagnitudeBudget;
        if (!TryCompileFixedPoint(Source.MagnitudeBudget, false, Budget.MagnitudeBudget)) return false;
        int64 Capacity = 0;
        for (const FMythicAffixRollGroupBudget &RollGroupBudget : Source.RollGroupBudgets) {
            if (!RollGroupBudget.RollGroup.IsValid() || RollGroupBudget.MaxRolls < 0
                || Budget.RollGroupCaps.Contains(RollGroupBudget.RollGroup)) return false;
            if (Capacity > MAX_int64 - RollGroupBudget.MaxRolls) return false;
            Capacity += RollGroupBudget.MaxRolls;
            Budget.RollGroupCaps.Add(RollGroupBudget.RollGroup, RollGroupBudget.MaxRolls);
        }
        if (Source.RandomRollCount < 0 || Capacity < Source.RandomRollCount) return false;
        Built->Policy.Budgets.Add(Budget.Rarity, MoveTemp(Budget));
    }
    for (uint8 Rarity = static_cast<uint8>(EItemRarity::Common);
         Rarity <= static_cast<uint8>(EItemRarity::Mythic); ++Rarity) {
        if (!Built->Policy.Budgets.Contains(static_cast<EItemRarity>(Rarity))) {
            AddError(OutErrors, TEXT("The roll policy must explicitly define every supported item rarity."));
            return false;
        }
    }

    TSet<FGuid> GrantGuids;
    for (const FMythicAffixGrantSpec &Grant : Profile.GuaranteedGrants) {
        FCompiledAffixGrant Compiled;
        Compiled.Spec = Grant;
        if (Grant.DeveloperName.IsNone() || !Grant.SourceKind.IsValid()) {
            AddError(OutErrors, TEXT("Every guaranteed affix grant requires a Developer Name and typed Source Kind."));
            return false;
        }
        if (!Grant.GrantGuid.IsValid() || GrantGuids.Contains(Grant.GrantGuid)
            || !Grant.RollGroup.IsValid()
            || !MythicAffixGrant::IsTierSelectionValid(Grant.TierMode, Grant.ExactTierRank)
            || !CompileDefinitionInternal(Grant.AffixDefinition, Registry, Compiled.Affix, OutErrors)
            || (Grant.TierMode == EMythicAffixGrantTierMode::ExactTier
                && !HasExactRankEverywhere(Compiled.Affix, Grant.ExactTierRank))) return false;
        GrantGuids.Add(Grant.GrantGuid);
        Built->GuaranteedGrants.Add(MoveTemp(Compiled));
    }

    TSet<FGuid> SliceGuids;
    TSet<FPrimaryAssetId> PoolIds;
    for (const FMythicAffixPoolSlice &Slice : Profile.RandomPoolSlices) {
        const FPrimaryAssetId PoolId = Slice.Pool.GetPrimaryAssetId();
        const UMythicAffixPool *Pool = Registry.FindPool(PoolId);
        if (Slice.DeveloperName.IsNone() || !Slice.SourceKind.IsValid()) {
            AddError(OutErrors, TEXT("Every random pool slice requires a Developer Name and typed Source Kind."));
            return false;
        }
        if (!Pool || !Slice.SliceGuid.IsValid() || SliceGuids.Contains(Slice.SliceGuid)
            || PoolIds.Contains(PoolId) || Slice.SliceWeight <= 0
            || !ValidatePoolAuthoring(*Pool, OutErrors)) return false;
        FCompiledAffixSlice CompiledSlice;
        CompiledSlice.SliceGuid = Slice.SliceGuid;
        CompiledSlice.DeveloperName = Slice.DeveloperName;
        CompiledSlice.PoolId = PoolId;
        CompiledSlice.PoolRevision = Pool->Revision;
        CompiledSlice.SourceKind = Slice.SourceKind;
        CompiledSlice.Priority = Slice.Priority;
        CompiledSlice.SliceWeight = Slice.SliceWeight;
        int64 TotalRowWeight = 0;
        for (const FMythicAffixPoolEntry &Row : Pool->Entries) {
            FCompiledAffixPoolRow CompiledRow;
            CompiledRow.PoolRowGuid = Row.PoolRowGuid;
            CompiledRow.DeveloperName = Row.DeveloperName;
            CompiledRow.RowRevision = Row.RowRevision;
            CompiledRow.RollGroup = Row.RollGroup;
            CompiledRow.EligibilityQuery = Row.EligibilityQuery;
            if (!TryCompileFixedPoint(Row.SelectionWeight, true, CompiledRow.SelectionWeight)
                || !AddCheckedPositiveWeight(CompiledRow.SelectionWeight, TotalRowWeight)
                || !CompileDefinitionInternal(Row.AffixDefinition, Registry,
                                              CompiledRow.Affix, OutErrors)) return false;
            CompiledSlice.Rows.Add(MoveTemp(CompiledRow));
        }
        CompiledSlice.Rows.Sort([](const FCompiledAffixPoolRow &A,
                                   const FCompiledAffixPoolRow &B) {
            return A.PoolRowGuid < B.PoolRowGuid;
        });
        SliceGuids.Add(Slice.SliceGuid);
        PoolIds.Add(PoolId);
        Built->RandomSlices.Add(MoveTemp(CompiledSlice));
    }
    Built->RandomSlices.Sort([](const FCompiledAffixSlice &A, const FCompiledAffixSlice &B) {
        if (A.Priority != B.Priority) return A.Priority > B.Priority;
        return A.SliceGuid < B.SliceGuid;
    });
    if (!ValidateStructuralFeasibility(*Built, OutErrors)) return false;

    FMythicAffixCanonicalWriter Gameplay("MYTHIC_AFFIX_PROFILE_GAMEPLAY_HASH_V4");
    FMythicAffixCanonicalWriter Presentation("MYTHIC_AFFIX_PROFILE_PRESENTATION_HASH_V4");
    Gameplay.AddPrimaryAssetId(Built->ProfileId);
    Gameplay.AddInt32(Built->ProfileRevision);
    Gameplay.AddPrimaryAssetId(Built->Policy.PolicyId);
    Gameplay.AddInt32(Built->Policy.Revision);
    Gameplay.AddInt32(Built->Policy.AlgorithmVersion);
    Gameplay.AddUInt8(Built->Policy.bGuaranteedConsumesMagnitudeBudget ? 1 : 0);
    Gameplay.AddUInt8(Built->Policy.bDisallowDuplicateAffixDefinition ? 1 : 0);
    Gameplay.AddUInt8(Built->Policy.bDisallowDuplicateTargetStat ? 1 : 0);
    Gameplay.AddUInt8(static_cast<uint8>(Built->Policy.ShortfallMode));
    Gameplay.AddUInt8(Built->Policy.bAllowLowerPriorityFallback ? 1 : 0);
    TArray<EItemRarity> Rarities;
    Built->Policy.Budgets.GetKeys(Rarities);
    Rarities.Sort([](const EItemRarity A, const EItemRarity B) {
        return static_cast<uint8>(A) < static_cast<uint8>(B);
    });
    for (const EItemRarity Rarity : Rarities) {
        const FCompiledAffixRarityBudget &Budget = Built->Policy.Budgets.FindChecked(Rarity);
        Gameplay.AddUInt8(static_cast<uint8>(Rarity));
        Gameplay.AddInt32(Budget.RandomRollCount);
        Gameplay.AddUInt8(Budget.bUnlimitedMagnitudeBudget ? 1 : 0);
        Gameplay.AddUInt64(static_cast<uint64>(Budget.MagnitudeBudget));
        TArray<FGameplayTag> RollGroups;
        Budget.RollGroupCaps.GetKeys(RollGroups);
        RollGroups.Sort([](const FGameplayTag &A, const FGameplayTag &B) {
            return A.ToString() < B.ToString();
        });
        for (const FGameplayTag &RollGroup : RollGroups) {
            Gameplay.AddName(RollGroup.GetTagName());
            Gameplay.AddInt32(Budget.RollGroupCaps.FindChecked(RollGroup));
        }
    }
    Presentation.AddPrimaryAssetId(Built->ProfileId);
    for (const FCompiledAffixGrant &Grant : Built->GuaranteedGrants) {
        Gameplay.AddGuid(Grant.Spec.GrantGuid);
        Gameplay.AddUInt8(static_cast<uint8>(Grant.Spec.TierMode));
        Gameplay.AddInt32(Grant.Spec.ExactTierRank);
        Gameplay.AddName(Grant.Spec.RollGroup.GetTagName());
        Gameplay.AddName(Grant.Spec.SourceKind.GetTagName());
        Gameplay.AddUInt8(Grant.Spec.bLocked ? 1 : 0);
        AddDefinitionHash(Gameplay, Presentation, Grant.Affix);
    }
    for (const FCompiledAffixSlice &Slice : Built->RandomSlices) {
        Gameplay.AddGuid(Slice.SliceGuid);
        Gameplay.AddPrimaryAssetId(Slice.PoolId);
        Gameplay.AddInt32(Slice.PoolRevision);
        Gameplay.AddName(Slice.SourceKind.GetTagName());
        Gameplay.AddInt32(Slice.Priority);
        Gameplay.AddInt32(Slice.SliceWeight);
        for (const FCompiledAffixPoolRow &Row : Slice.Rows) {
            Gameplay.AddGuid(Row.PoolRowGuid);
            Gameplay.AddInt32(Row.RowRevision);
            Gameplay.AddName(Row.RollGroup.GetTagName());
            Gameplay.AddUInt64(static_cast<uint64>(Row.SelectionWeight));
            AddGameplayTagQuery(Gameplay, Row.EligibilityQuery);
            AddDefinitionHash(Gameplay, Presentation, Row.Affix);
        }
    }
    Built->GameplayContentHash = HashWriter(Gameplay);
    Built->PresentationContentHash = HashWriter(Presentation);
    if (Built->GameplayContentHash.IsZero() || Built->PresentationContentHash.IsZero()) return false;
    FMythicAffixDependencyManifest &Manifest = Built->DependencyManifest;
    Manifest.ProfileId = Built->ProfileId;
    Manifest.GameplayContentHash = Built->GameplayContentHash;
    Manifest.PresentationContentHash = Built->PresentationContentHash;
    Manifest.RuntimePrimaryAssets.AddUnique(Built->ProfileId);
    Manifest.RuntimePrimaryAssets.AddUnique(Built->Policy.PolicyId);
    auto AddDefinitionAssets = [&Manifest](const FCompiledAffix &Affix) {
        Manifest.RuntimePrimaryAssets.AddUnique(Affix.Definition.DefinitionId);
        Manifest.RuntimePrimaryAssets.AddUnique(Affix.Definition.TargetStatId);
        Manifest.RuntimePrimaryAssets.AddUnique(Affix.Definition.TargetCategoryId);
        Manifest.PresentationPrimaryAssets.AddUnique(Affix.Definition.DefinitionId);
        Manifest.PresentationPrimaryAssets.AddUnique(Affix.Definition.TargetStatId);
        Manifest.PresentationPrimaryAssets.AddUnique(Affix.Definition.TargetCategoryId);
    };
    for (const FCompiledAffixGrant &Grant : Built->GuaranteedGrants) AddDefinitionAssets(Grant.Affix);
    for (const FCompiledAffixSlice &Slice : Built->RandomSlices) {
        Manifest.RuntimePrimaryAssets.AddUnique(Slice.PoolId);
        for (const FCompiledAffixPoolRow &Row : Slice.Rows) AddDefinitionAssets(Row.Affix);
    }
    Manifest.RuntimePrimaryAssets.Sort([](const FPrimaryAssetId &A, const FPrimaryAssetId &B) {
        return A.ToString() < B.ToString();
    });
    Manifest.PresentationPrimaryAssets.Sort([](const FPrimaryAssetId &A, const FPrimaryAssetId &B) {
        return A.ToString() < B.ToString();
    });
    OutProfile = Built;
    return true;
}

#undef LOCTEXT_NAMESPACE
