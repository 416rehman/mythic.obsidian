#include "Itemization/Affixes/MythicAffixDefinition.h"

#include "Internationalization/Text.h"
#include "Misc/DataValidation.h"
#include "System/MythicAssetManager.h"

#define LOCTEXT_NAMESPACE "MythicAffixDefinition"

namespace {
bool HasStableAffixDefinitionStringTableIdentity(const FText &Text) {
    FName TableId;
    FString Key;
    return !Text.IsEmpty() && FTextInspector::GetTableIdAndKey(Text, TableId, Key)
        && !TableId.IsNone() && !Key.IsEmpty();
}

float SampleOpenEnded(const FRuntimeFloatCurve &CurveSource, const float Level,
                      const float TailGrowth) {
    const FRichCurve *Curve = CurveSource.GetRichCurveConst();
    if (!Curve || Curve->GetNumKeys() == 0) {
        return -1.0f;
    }

    float MinTime = 0.0f;
    float MaxTime = 0.0f;
    Curve->GetTimeRange(MinTime, MaxTime);
    const float Base = Curve->Eval(FMath::Clamp(Level, MinTime, MaxTime));
    return Level <= MaxTime
               ? Base
               : Base * FMath::Pow(FMath::Max(TailGrowth, 1.0f), Level - MaxTime);
}
}

const FMythicAffixTierProgressionDefinition *UMythicAffixDefinition::ResolveTierProgression(
    const FGameplayTagContainer &ContextTags) const {
    const FMythicAffixTierProgressionDefinition *Winner = nullptr;
    const FMythicAffixTierProgressionDefinition *Fallback = nullptr;
    int32 WinningPriority = MIN_int32;
    bool bAmbiguous = false;
    for (const FMythicAffixTierProgressionDefinition &Progression : TierProgressions) {
        if (Progression.ApplicabilityQuery.IsEmpty()) {
            if (Fallback) {
                return nullptr;
            }
            Fallback = &Progression;
            continue;
        }
        if (!Progression.ApplicabilityQuery.Matches(ContextTags)) {
            continue;
        }
        if (!Winner || Progression.SelectionPriority > WinningPriority) {
            Winner = &Progression;
            WinningPriority = Progression.SelectionPriority;
            bAmbiguous = false;
        }
        else if (Progression.SelectionPriority == WinningPriority) {
            bAmbiguous = true;
        }
    }
    return bAmbiguous ? nullptr : (Winner ? Winner : Fallback);
}

FGameplayTag UMythicAffixDefinition::GetEffectiveStackingGroup() const {
    if (StackingRule == EMythicAffixStackingRule::StackAll) {
        return FGameplayTag();
    }
    return StackingGroup.IsValid() ? StackingGroup : AffixTag;
}

bool UMythicAffixDefinition::ResolveMagnitudeBand(const FMythicAffixMagnitudeBand &Band,
                                                   int32 ItemLevel, float &OutMin,
                                                   float &OutMax) {
    if (!FMath::IsFinite(Band.Min) || !FMath::IsFinite(Band.Max) || Band.Min < 0.0f
        || Band.Max < Band.Min) {
        return false;
    }
    const float Level = static_cast<float>(FMath::Max(1, ItemLevel));
    switch (Band.ScaleMode) {
    case EMythicAffixScaleMode::None:
        OutMin = Band.Min;
        OutMax = Band.Max;
        break;
    case EMythicAffixScaleMode::Linear:
        if (!FMath::IsFinite(Band.LinearPerItemLevel)
            || Band.LinearPerItemLevel <= 0.0f) {
            return false;
        }
        OutMin = Band.Min + Level * Band.LinearPerItemLevel;
        OutMax = Band.Max + Level * Band.LinearPerItemLevel;
        break;
    case EMythicAffixScaleMode::Curve: {
        if (Band.LevelScalingCurve.ExternalCurve != nullptr
            || !FMath::IsFinite(Band.CurveTailGrowth) || Band.CurveTailGrowth < 1.0f) {
            return false;
        }
        const float Scale = SampleOpenEnded(Band.LevelScalingCurve, Level, Band.CurveTailGrowth);
        if (!FMath::IsFinite(Scale) || Scale < 0.0f) {
            return false;
        }
        OutMin = Band.Min * Scale;
        OutMax = Band.Max * Scale;
        break;
    }
    default:
        return false;
    }
    return FMath::IsFinite(OutMin) && FMath::IsFinite(OutMax) && OutMin >= 0.0f
        && OutMax >= OutMin;
}

FPrimaryAssetId UMythicAffixDefinition::GetPrimaryAssetId() const {
    return AffixTag.IsValid()
               ? FPrimaryAssetId(UMythicAssetManager::AffixDefinitionType, AffixTag.GetTagName())
               : FPrimaryAssetId();
}

#if WITH_EDITOR
EDataValidationResult UMythicAffixDefinition::IsDataValid(FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (!AffixTag.IsValid() || !AffixTag.ToString().StartsWith(TEXT("Itemization.Affix."))) {
        Error(LOCTEXT("InvalidAffixTag", "AffixTag must be a valid Itemization.Affix.* tag."));
    }
    if (DeveloperName.IsNone() || DesignerPurpose.TrimStartAndEnd().IsEmpty() || Revision < 1
        || PresentationRevision < 1) {
        Error(LOCTEXT("InvalidMetadata", "DeveloperName, DesignerPurpose, and positive revisions are required."));
    }
    if (TargetStat.Asset.IsNull() || !TargetStat.IsValid()) {
        Error(LOCTEXT("InvalidTargetStat",
                      "Target Stat must directly reference one canonical Stat Definition asset."));
    }
    if (!HasStableAffixDefinitionStringTableIdentity(DisplayNameTemplate)) {
        Error(LOCTEXT("InvalidDisplayNameTemplate",
                      "Display Name Template must use a nonempty stable String Table identity."));
    }
    if (!DescriptionTemplate.IsEmpty() && !HasStableAffixDefinitionStringTableIdentity(DescriptionTemplate)) {
        Error(LOCTEXT("InvalidDescriptionTemplate",
                      "A nonempty Description Template must use a stable String Table identity."));
    }
    if (!Quantization.IsValid()) {
        Error(LOCTEXT("InvalidQuantization", "Step quantization requires a finite positive step."));
    }
    if (!MythicAffix::IsSupportedModifierOp(ModifierOp)) {
        Error(LOCTEXT("InvalidModifierOp", "Operation must be one of the permanent stat ledger's supported GAS operations."));
    }

    if (TierProgressions.IsEmpty()) {
        Error(LOCTEXT("MissingTierProgressions",
                      "Every Affix Definition requires at least one embedded Tier Progression."));
    }
    TSet<int32> ProgressionPriorities;
    int32 FallbackProgressionCount = 0;
    for (const FMythicAffixTierProgressionDefinition &Progression : TierProgressions) {
        if (Progression.ApplicabilityQuery.IsEmpty()) {
            ++FallbackProgressionCount;
        }
        if (ProgressionPriorities.Contains(Progression.SelectionPriority)) {
            Error(LOCTEXT("DuplicateProgressionPriority",
                          "Tier Progression priorities must be globally unique within an Affix Definition."));
        }
        ProgressionPriorities.Add(Progression.SelectionPriority);
        if (Progression.DeveloperName.IsNone() || Progression.TuningContext.IsNone()
            || Progression.Tiers.IsEmpty()) {
            Error(LOCTEXT("InvalidTierProgression", "Tier progressions require a developer identity, tuning context, and at least one tier."));
        }
        int32 PreviousMinItemLevel = 0;
        for (int32 TierIndex = 0; TierIndex < Progression.Tiers.Num(); ++TierIndex) {
            const FMythicAffixTierDefinition &Tier = Progression.Tiers[TierIndex];
            if (Tier.DeveloperName.IsNone() || Tier.PresentationRevision < 1 || Tier.MinItemLevel < 1
                || (TierIndex == 0 && Tier.MinItemLevel != 1)
                || Tier.MinItemLevel <= PreviousMinItemLevel
                || !FMath::IsFinite(Tier.TierWeight) || Tier.TierWeight <= 0.0f
                || !FMath::IsFinite(Tier.BudgetCost) || Tier.BudgetCost < 0.0f) {
                Error(LOCTEXT("InvalidTier", "Tiers require identity, a first tier at item level 1, strictly increasing later minimum levels, positive presentation revision and weight, and a non-negative budget cost."));
            }
            if (!Tier.DisplayName.IsEmpty() && !HasStableAffixDefinitionStringTableIdentity(Tier.DisplayName)) {
                Error(LOCTEXT("InvalidTierDisplayName",
                              "A nonempty Tier Display Name must use a stable String Table identity."));
            }
            PreviousMinItemLevel = Tier.MinItemLevel;
            float Min = 0.0f;
            float Max = 0.0f;
            if (!ResolveMagnitudeBand(Tier.Magnitude, Tier.MinItemLevel, Min, Max)) {
                Error(LOCTEXT("InvalidBand",
                              "Every tier requires one finite, ordered Roll Range owned inline by the Affix Definition for its Target Stat."));
            }
        }
    }
    if (FallbackProgressionCount != 1) {
        Error(LOCTEXT("InvalidFallbackProgressionCount",
                      "Every Affix Definition requires exactly one fallback Tier Progression with an empty Applies When query."));
    }
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
