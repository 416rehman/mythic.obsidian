// Copyright Stellar Games. All Rights Reserved.

#include "GAS/Effects/MythicGE_PrimaryGrowth.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Executions/MythicMMC_PrimaryFromLevel.h"

UMythicGE_PrimaryGrowth::UMythicGE_PrimaryGrowth() {
    DurationPolicy = EGameplayEffectDurationType::Infinite;

    {
        FGameplayModifierInfo PowerMod;
        PowerMod.Attribute = UMythicAttributeSet_Offense::GetPowerAttribute();
        PowerMod.ModifierOp = EGameplayModOp::Additive;
        FCustomCalculationBasedFloat PowerCalc;
        PowerCalc.CalculationClassMagnitude = UMythicMMC_PowerFromLevel::StaticClass();
        PowerMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(PowerCalc);
        Modifiers.Add(PowerMod);
    }

    {
        FGameplayModifierInfo StrengthMod;
        StrengthMod.Attribute = UMythicAttributeSet_Defense::GetStrengthAttribute();
        StrengthMod.ModifierOp = EGameplayModOp::Additive;
        FCustomCalculationBasedFloat StrengthCalc;
        StrengthCalc.CalculationClassMagnitude = UMythicMMC_StrengthFromLevel::StaticClass();
        StrengthMod.ModifierMagnitude = FGameplayEffectModifierMagnitude(StrengthCalc);
        Modifiers.Add(StrengthMod);
    }
}
