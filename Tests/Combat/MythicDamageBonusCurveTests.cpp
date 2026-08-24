#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicStatDiminishing.h"
#include "Settings/MythicCombatSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDamageBonusCurveTest,
    "Mythic.Combat.DamageBonusCurve",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDamageBonusCurveTest::RunTest(const FString &Parameters) {
    using Off = UMythicAttributeSet_Offense;

    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!TestNotNull(TEXT("combat settings exist"), Settings)) {
        return false;
    }

    // Every fraction MythicDamageApplication multiplies as (1 + x) must ride a diminishing curve, or the narrowest,
    // most stackable damage stats ride uncapped forever. This list mirrors the (1+x) attribute consumptions in
    // MythicDamageApplication::Execute_Implementation: add one there and it must earn a curve here, or this fails.
    const FGameplayAttribute BonusFractions[] = {
        Off::GetCriticalHitDamageAttribute(),
        Off::GetBonusSwordDamageAttribute(),
        Off::GetBonusAxeDamageAttribute(),
        Off::GetBonusDaggerDamageAttribute(),
        Off::GetBonusSickleDamageAttribute(),
        Off::GetBonusSpearDamageAttribute(),
        Off::GetBonusHammerDamageAttribute(),
        Off::GetBonusSkillDamageAttribute(),
        Off::GetIncreasedDamageToEnemiesUnderStatusEffectsAttribute(),
        Off::GetBonusDamageToSuperiorEnemiesAttribute(),
    };

    for (const FGameplayAttribute &Attr : BonusFractions) {
        float Soft = 0.0f;
        float Ceiling = 0.0f;
        FMythicStatDiminishingRules::FindCurve(Settings->StatDiminishing, Attr, Soft, Ceiling);
        TestTrue(*FString::Printf(TEXT("%s has an authored diminishing curve"), *Attr.GetName()), Ceiling > 0.0f);

        // The curve must actually bite: a huge stack comes back under its uncurved passthrough of (1 + x).
        const float Bent = FMythicStatDiminishingRules::ApplyToBonus(Settings->StatDiminishing, Attr, 1000.0f);
        TestTrue(*FString::Printf(TEXT("%s is bounded past its soft cap"), *Attr.GetName()), Bent < 1.0f + 1000.0f);

        // ...and never inverts damage: even bent, the multiplier stays at or above face value.
        TestTrue(*FString::Printf(TEXT("%s never reduces below face value"), *Attr.GetName()), Bent >= 1.0f);
    }

    return true;
}
