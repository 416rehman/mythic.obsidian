// Copyright Stellar Games. All Rights Reserved.

#include "GAS/Executions/MythicMMC_PrimaryFromLevel.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "Settings/MythicCombatSettings.h"

UMythicMMC_PrimaryFromLevel::UMythicMMC_PrimaryFromLevel() {
    // Level is derived, not stored: capture the XP pair non-snapshot and run the one shared formula, so a
    // level-up (an XP write) re-evaluates this magnitude the same frame.
    XpDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Proficiencies::GetOverallXpAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    XpMaxDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Proficiencies::GetOverallXpMaxAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    RelevantAttributesToCapture.Add(XpDef);
    RelevantAttributesToCapture.Add(XpMaxDef);
}

float UMythicMMC_PrimaryFromLevel::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec &Spec) const {
    const FCurveTableRowHandle *Curve = GetGrowthCurve();
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Curve || Curve->IsNull() || !Settings) {
        return 0.0f;
    }

    FAggregatorEvaluateParameters Params;
    Params.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    Params.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float Xp = 0.0f;
    float XpMax = 0.0f;
    GetCapturedAttributeMagnitude(XpDef, Spec, Params, Xp);
    GetCapturedAttributeMagnitude(XpMaxDef, Spec, Params, XpMax);
    const float Level = FMath::Max(1.0f, static_cast<float>(UMythicAttributeSet_Proficiencies::LevelFromXp(Xp, XpMax)));

    // Growth above the level-1 value: the curve owns the whole shape, the seeded base owns the start, and
    // the two can never double-count.
    const float Tail = Settings->PlayerPrimaryTailGrowth;
    return MythicCombat::SampleOpenEnded(*Curve, Level, Tail) - MythicCombat::SampleOpenEnded(*Curve, 1.0f, Tail);
}

const FCurveTableRowHandle *UMythicMMC_PowerFromLevel::GetGrowthCurve() const {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? &Settings->PlayerPowerCurve : nullptr;
}

const FCurveTableRowHandle *UMythicMMC_StrengthFromLevel::GetGrowthCurve() const {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? &Settings->PlayerStrengthCurve : nullptr;
}
