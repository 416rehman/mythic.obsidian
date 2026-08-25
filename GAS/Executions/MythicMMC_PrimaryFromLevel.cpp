// Copyright Stellar Games. All Rights Reserved.

#include "GAS/Executions/MythicMMC_PrimaryFromLevel.h"

#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Settings/MythicCombatSettings.h"

UMythicMMC_PrimaryFromLevel::UMythicMMC_PrimaryFromLevel() {
    // Fighting is what grows the fighter: primaries follow the COMBAT proficiency level. The capture is
    // the re-evaluation trigger (a combat XP write recomputes this magnitude the same frame).
    XpDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    XpMaxDef = FGameplayEffectAttributeCaptureDefinition(
        UMythicAttributeSet_Proficiencies::GetOverallXpAttribute(), EGameplayEffectAttributeCaptureSource::Target, false);
    RelevantAttributesToCapture.Add(XpDef);
    RelevantAttributesToCapture.Add(XpMaxDef);
}

float UMythicMMC_PrimaryFromLevel::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec &Spec) const {
    const FCurveTableRowHandle *Curve = GetGrowthCurve();
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!Curve || Curve->IsNull() || !Settings) {
        return 0.0f;
    }

    // Read the combat XP off the ASC rather than the captured magnitude: the growth GE is self-applied,
    // so the instigator ASC is the target, and a direct read stays correct even when a stale CDO capture
    // list is still triggering re-evaluation off the overall pair.
    float CombatXp = 0.0f;
    if (const UAbilitySystemComponent *ASC = Spec.GetContext().GetInstigatorAbilitySystemComponent()) {
        bool bFound = false;
        const float Value = ASC->GetGameplayAttributeValue(UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute(), bFound);
        if (bFound) {
            CombatXp = Value;
        }
    }

    const UProficiencyDefinition *CombatDef =
        UProficiencyDefinition::FindByProgressAttribute(UMythicAttributeSet_Proficiencies::GetCombatProficiencyAttribute());
    if (!CombatDef) {
        return 0.0f;
    }
    const float Level = FMath::Max(1.0f, static_cast<float>(UProficiencyDefinition::CalcLevelAtXP(CombatXp, CombatDef)));

    // Growth above the level-1 value: the curve owns the whole shape, the seeded base owns the start, and
    // the two can never double-count. Rounded because primaries are whole numbers by design - a sheet
    // reading "Strength 11.27" is authored noise, not precision.
    const float Tail = Settings->PlayerPrimaryTailGrowth;
    return FMath::RoundToFloat(MythicCombat::SampleOpenEnded(*Curve, Level, Tail) - MythicCombat::SampleOpenEnded(*Curve, 1.0f, Tail));
}

const FCurveTableRowHandle *UMythicMMC_PowerFromLevel::GetGrowthCurve() const {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? &Settings->PlayerPowerCurve : nullptr;
}

const FCurveTableRowHandle *UMythicMMC_StrengthFromLevel::GetGrowthCurve() const {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    return Settings ? &Settings->PlayerStrengthCurve : nullptr;
}
