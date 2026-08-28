// Copyright Stellar Games. All Rights Reserved.

#include "UI/ViewModels/MythicAffixViewData.h"

#include "Itemization/Affixes/MythicAffixDefinition.h"
#include "Itemization/Affixes/MythicItemizationDataRegistrySubsystem.h"
#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "UI/ViewModels/MythicStatDisplay.h"

namespace {
bool IsMultiplyOperation(const EGameplayModOp::Type Op) {
    return Op == EGameplayModOp::MultiplyAdditive || Op == EGameplayModOp::MultiplyCompound
        || Op == EGameplayModOp::DivideAdditive;
}

float ToTargetSpaceValue(const float Magnitude, const EGameplayModOp::Type Op) {
    return Op == EGameplayModOp::DivideAdditive ? 1.0f / Magnitude : Magnitude;
}

float ContributionIdentity(const EGameplayModOp::Type Op, const float StatNeutral) {
    return IsMultiplyOperation(Op) ? 1.0f
        : Op == EGameplayModOp::Override ? StatNeutral : 0.0f;
}

FText FormatModifierValue(const float Magnitude, const EGameplayModOp::Type Op,
                          const FMythicStatNumberPresentation &Presentation) {
    const float EffectiveValue = ToTargetSpaceValue(Magnitude, Op);
    switch (Op) {
    case EGameplayModOp::AddBase:
    case EGameplayModOp::AddFinal:
        return MythicStatDisplay::FormatBonus(EffectiveValue, Presentation);
    case EGameplayModOp::MultiplyAdditive:
    case EGameplayModOp::MultiplyCompound:
    case EGameplayModOp::DivideAdditive:
        return MythicStatDisplay::FormatBonus(EffectiveValue - 1.0f, Presentation);
    case EGameplayModOp::Override:
    default:
        return MythicStatDisplay::FormatValue(EffectiveValue, Presentation);
    }
}

FText FormatRangeEndpoint(const float Magnitude, const EGameplayModOp::Type Op,
                          const FMythicStatNumberPresentation &Presentation) {
    const float EffectiveValue = ToTargetSpaceValue(Magnitude, Op);
    if (IsMultiplyOperation(Op)) {
        const FText Signed = MythicStatDisplay::FormatBonus(EffectiveValue - 1.0f, Presentation);
        return Signed.IsEmpty() ? MythicStatDisplay::FormatValue(EffectiveValue, Presentation) : Signed;
    }
    if (Op == EGameplayModOp::AddBase || Op == EGameplayModOp::AddFinal) {
        const FText Signed = MythicStatDisplay::FormatBonus(EffectiveValue, Presentation);
        return Signed.IsEmpty() ? MythicStatDisplay::FormatValue(EffectiveValue, Presentation) : Signed;
    }
    return MythicStatDisplay::FormatValue(EffectiveValue, Presentation);
}

FText ResolveTemplate(const FText &Template, const FMythicAffixValueViewData &Value) {
    if (Template.IsEmpty()) {
        return FText::GetEmpty();
    }

    // Named FText arguments preserve language-specific word order. Identity and gameplay never depend on templates.
    FFormatNamedArguments Arguments;
    Arguments.Add(TEXT("Stat"), Value.StatLabel);
    Arguments.Add(TEXT("Value"), Value.FormattedValue);
    return FText::Format(Template, Arguments);
}

FText ResolveTierDisplayName(const UMythicAffixDefinition &Definition, const int32 TierRank,
                             const FMythicAffixTierProgressionDefinition *ResolvedProgression) {
    if (TierRank < 1) {
        return FText::GetEmpty();
    }

    const int32 TierIndex = TierRank - 1;
    if (ResolvedProgression && ResolvedProgression->Tiers.IsValidIndex(TierIndex)
        && !ResolvedProgression->Tiers[TierIndex].DisplayName.IsEmpty()) {
        return ResolvedProgression->Tiers[TierIndex].DisplayName;
    }

    FText SharedLabel;
    bool bFoundLabel = false;
    bool bAmbiguousLabel = false;
    for (const FMythicAffixTierProgressionDefinition &Progression : Definition.TierProgressions) {
        if (!Progression.Tiers.IsValidIndex(TierIndex) || Progression.Tiers[TierIndex].DisplayName.IsEmpty()) {
            bAmbiguousLabel = true;
            continue;
        }

        const FText &Label = Progression.Tiers[TierIndex].DisplayName;
        if (!bFoundLabel) {
            SharedLabel = Label;
            bFoundLabel = true;
        }
        else if (!SharedLabel.EqualTo(Label)) {
            // The snapshot deliberately carries no progression selector. An ambiguous authored label must not be
            // chosen by array order, so detailed UI falls back to the stable rank shared by every progression.
            bAmbiguousLabel = true;
            break;
        }
    }

    return bFoundLabel && !bAmbiguousLabel
        ? SharedLabel
        : FText::Format(NSLOCTEXT("MythicAffixes", "AffixTierRank", "Tier {0}"),
                        FText::AsNumber(TierRank));
}

FText BuildRichText(const FMythicAffixViewData &View, const bool bNameTemplateContainsValue) {
    if (View.Values.Num() != 1) {
        return FText::GetEmpty();
    }
    if (bNameTemplateContainsValue) {
        return View.DisplayName;
    }
    return FText::Format(NSLOCTEXT("MythicAffixes", "AffixRichText", "<Roll>{0}</> {1}"),
                         View.Values[0].FormattedValue, View.DisplayName);
}

bool BuildViewDataInternal(
    const FRolledAffix &Snapshot,
    const UMythicItemizationDataRegistrySubsystem *Registry,
    const FGameplayTagContainer *ItemContextTags,
    const int32 ItemLevel,
    FMythicAffixViewData &OutViewData) {
    OutViewData = FMythicAffixViewData();
    if (!Registry || !Registry->IsCoreSemanticReady() || !Snapshot.IsGameplayValid()) {
        return false;
    }

    const FPrimaryAssetId DefinitionId = Snapshot.AffixDefinition.GetPrimaryAssetId();
    const UMythicAffixDefinition *Definition = Registry->FindAffix(DefinitionId);
    if (!Definition || !Definition->AffixTag.IsValid() || !Definition->TargetStat.IsValid()
        || !MythicAffix::IsSupportedModifierOp(Definition->ModifierOp.GetValue())
        || !Definition->Quantization.IsValid()
        || (MythicAffix::ModifierRequiresNonZeroMagnitude(Definition->ModifierOp)
            && FMath::IsNearlyZero(Snapshot.Magnitude))) {
        return false;
    }

    const FPrimaryAssetId StatId = Definition->TargetStat.GetPrimaryAssetId();
    const UMythicStatDefinition *Stat = Registry->FindStat(StatId);
    if (!Stat || Stat->GetPrimaryAssetId() != StatId || !Stat->StatTag.IsValid()
        || Stat->DisplayName.IsEmpty() || !Stat->bCanBeAffixTarget) {
        return false;
    }
    const UMythicStatCategoryDefinition *Category =
        Registry->GetStatRegistry().FindCategory(Stat->Category.GetPrimaryAssetId());
    if (!Category || !Category->CategoryTag.IsValid()) {
        return false;
    }

    const FMythicAffixTierProgressionDefinition *Progression = ItemContextTags
        ? Definition->ResolveTierProgression(*ItemContextTags)
        : nullptr;

    FMythicAffixViewData Candidate;
    Candidate.RollGuid = Snapshot.RollGuid;
    Candidate.AffixTag = Definition->AffixTag;
    Candidate.PrimaryStatTag = Stat->StatTag;
    Candidate.RollGroup = Snapshot.Provenance.RollGroup;
    Candidate.SourceKind = Snapshot.Provenance.SourceKind;
    Candidate.TierRank = Snapshot.TierRank;
    Candidate.TierDisplayName = ResolveTierDisplayName(*Definition, Snapshot.TierRank, Progression);
    Candidate.bLocked = Snapshot.bIsLocked;

    Candidate.SemanticCategoryTag = Category->CategoryTag;

    FMythicAffixValueViewData &Value = Candidate.Values.AddDefaulted_GetRef();
    Value.StatTag = Stat->StatTag;
    Value.StatLabel = Stat->DisplayName;
    Value.ModifierOp = Definition->ModifierOp;
    Value.RawValue = Snapshot.Magnitude;
    Value.ComparisonValue = ToTargetSpaceValue(Snapshot.Magnitude, Definition->ModifierOp);
    Value.FinalStatNeutralValue = Stat->NeutralValue;
    Value.ContributionIdentity = ContributionIdentity(Definition->ModifierOp, Stat->NeutralValue);
    Value.ComparisonDirection = Stat->ComparisonDirection;

    const FMythicStatNumberPresentation Presentation =
        MythicStatDisplay::ResolveModifierPresentation(*Stat, Definition->ModifierOp);
    Value.NumberPresentation = Presentation;
    Value.FormattedValue = FormatModifierValue(Snapshot.Magnitude,
                                               Definition->ModifierOp.GetValue(), Presentation);
    if (Value.FormattedValue.IsEmpty()) {
        Value.FormattedValue = MythicStatDisplay::FormatValue(Snapshot.Magnitude, Presentation);
    }
    if (Progression && ItemLevel >= 1) {
        const int32 TierIndex = Snapshot.TierRank - 1;
        float RangeMin = 0.0f;
        float RangeMax = 0.0f;
        if (Progression->Tiers.IsValidIndex(TierIndex)
            && UMythicAffixDefinition::ResolveMagnitudeBand(
                Progression->Tiers[TierIndex].Magnitude, ItemLevel, RangeMin, RangeMax)) {
            RangeMin = Definition->Quantization.Apply(RangeMin);
            RangeMax = Definition->Quantization.Apply(RangeMax);
            if (Definition->ModifierOp == EGameplayModOp::DivideAdditive
                && !FMath::IsNearlyZero(RangeMin) && !FMath::IsNearlyZero(RangeMax)) {
                Swap(RangeMin, RangeMax);
            }
            if (FMath::IsFinite(RangeMin) && FMath::IsFinite(RangeMax)
                && !FMath::IsNearlyEqual(RangeMin, RangeMax, 0.0001f)) {
                Value.FormattedRange = FText::Format(
                    NSLOCTEXT("MythicAffixes", "AffixRange", "[{0}-{1}]"),
                    FormatRangeEndpoint(RangeMin, Definition->ModifierOp.GetValue(), Presentation),
                    FormatRangeEndpoint(RangeMax, Definition->ModifierOp.GetValue(), Presentation));
            }
        }
    }

    const FText DisplayTemplate = Definition->DisplayNameTemplate;
    Candidate.DisplayName = DisplayTemplate.IsEmpty()
        ? Value.StatLabel
        : ResolveTemplate(DisplayTemplate, Value);
    Candidate.Description = ResolveTemplate(Definition->DescriptionTemplate, Value);
    if (Candidate.DisplayName.IsEmpty()) {
        return false;
    }

    Candidate.RichText = BuildRichText(Candidate,
        !DisplayTemplate.IsEmpty()
        && DisplayTemplate.ToString().Contains(TEXT("{Value}"), ESearchCase::CaseSensitive));
    OutViewData = MoveTemp(Candidate);
    return true;
}
}

bool UMythicAffixViewDataLibrary::BuildViewData(
    const FRolledAffix &Snapshot,
    const UMythicItemizationDataRegistrySubsystem *Registry,
    FMythicAffixViewData &OutViewData) {
    return BuildViewDataInternal(Snapshot, Registry, nullptr, 0, OutViewData);
}

bool UMythicAffixViewDataLibrary::BuildViewDataWithItemContext(
    const FRolledAffix &Snapshot,
    const FGameplayTagContainer &ItemContextTags,
    const int32 ItemLevel,
    const UMythicItemizationDataRegistrySubsystem *Registry,
    FMythicAffixViewData &OutViewData) {
    return ItemLevel >= 1
        && BuildViewDataInternal(Snapshot, Registry, &ItemContextTags, ItemLevel, OutViewData);
}
