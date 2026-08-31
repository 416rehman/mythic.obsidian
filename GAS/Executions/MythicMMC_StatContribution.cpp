
#include "GAS/Executions/MythicMMC_StatContribution.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicStatContribution.h"
#include "Settings/MythicCombatSettings.h"

UMythicMMC_StatContribution::UMythicMMC_StatContribution() {
    // Captured from the Target, because this runs inside an infinite effect the character applies to itself.
    // Not snapshotted: raising Strength must move MaxHealth now, not at whatever value it held when the
    // effect was first applied.
    PowerDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Offense::GetPowerAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    StrengthDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Defense::GetStrengthAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    ResolveDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Utility::GetResolveAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);

    RelevantAttributesToCapture.Add(PowerDef);
    RelevantAttributesToCapture.Add(StrengthDef);
    RelevantAttributesToCapture.Add(ResolveDef);
}

float UMythicMMC_StatContribution::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec &Spec) const {
    if (!TargetAttribute.IsValid()) {
        return 0.0f;
    }
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Settings) {
        return 0.0f;
    }

    FAggregatorEvaluateParameters Params;
    Params.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    Params.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float Power = 0.0f;
    float Strength = 0.0f;
    float Resolve = 0.0f;
    GetCapturedAttributeMagnitude(PowerDef, Spec, Params, Power);
    GetCapturedAttributeMagnitude(StrengthDef, Spec, Params, Strength);
    GetCapturedAttributeMagnitude(ResolveDef, Spec, Params, Resolve);

    return FMythicStatContributionRules::ResolveTarget(
        Settings->StatContributions.Contributions, TargetAttribute,
        [Power, Strength, Resolve](const FGameplayAttribute &Attr) -> float {
            if (Attr == UMythicAttributeSet_Offense::GetPowerAttribute()) {
                return Power;
            }
            if (Attr == UMythicAttributeSet_Defense::GetStrengthAttribute()) {
                return Strength;
            }
            if (Attr == UMythicAttributeSet_Utility::GetResolveAttribute()) {
                return Resolve;
            }
            // A row sourced from a stat this calculation does not capture contributes nothing rather than
            // reading as a confident zero, so adding a source later is a capture change and not a silent bug.
            return 0.0f;
        });
}
