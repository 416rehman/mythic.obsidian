#include "GAS/MythicStatSummary.h"

#include "AbilitySystemComponent.h"
#include "GAS/Combat/MythicWeaponOffenseProjection.h"

float UMythicBasicAttackDamageSummaryCalculation::Calculate_Implementation(
    const UAbilitySystemComponent *ASC) const {
    FMythicWeaponDamageProjection Projection;
    return MythicCombat::BuildWeaponDamageProjection(ASC, Projection)
        ? Projection.EffectiveAverageDamage : 0.0f;
}

bool UMythicBasicAttackDamageSummaryCalculation::CalculateRange(
    const UAbilitySystemComponent *ASC,
    float &OutMinimum,
    float &OutMaximum) const {
    OutMinimum = 0.0f;
    OutMaximum = 0.0f;

    FMythicWeaponDamageProjection Projection;
    if (!MythicCombat::BuildWeaponDamageProjection(ASC, Projection)) {
        return false;
    }
    OutMinimum = Projection.EffectiveMinimumDamage;
    OutMaximum = Projection.EffectiveMaximumDamage;
    return true;
}

float UMythicStatSummaryDefinition::Compute(const UAbilitySystemComponent *ASC) const {
    if (!CalculationClass) {
        return 0.0f;
    }
    // The calculation is pure, so its class default object is a valid evaluator — no per-read allocation, nothing to
    // store, nothing to free.
    const UMythicStatSummaryCalculation *Calc = GetDefault<UMythicStatSummaryCalculation>(CalculationClass);
    return Calc ? Calc->Calculate(ASC) : 0.0f;
}

bool UMythicStatSummaryDefinition::ComputeRange(
    const UAbilitySystemComponent *ASC,
    float &OutMinimum,
    float &OutMaximum) const {
    OutMinimum = 0.0f;
    OutMaximum = 0.0f;
    if (!CalculationClass) {
        return false;
    }
    const UMythicStatSummaryCalculation *Calc =
        GetDefault<UMythicStatSummaryCalculation>(CalculationClass);
    return Calc && Calc->CalculateRange(ASC, OutMinimum, OutMaximum);
}
