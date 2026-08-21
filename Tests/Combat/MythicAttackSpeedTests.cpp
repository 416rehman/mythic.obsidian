
#include "Misc/AutomationTest.h"

#include "GAS/Abilities/MythicGameplayAbility.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicAttackSpeedPlayRateTest,
    "Mythic.Combat.AttackSpeedPlayRate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicAttackSpeedPlayRateTest::RunTest(const FString &Parameters) {
    using Ability = UMythicGameplayAbility;

    TestEqual(TEXT("no bonus plays at the authored rate"), Ability::ComputeAttackSpeedPlayRate(0.0f, 0.8f, 1.4f), 1.0f);
    TestEqual(TEXT("a rolled bonus speeds the montage up by that fraction"), Ability::ComputeAttackSpeedPlayRate(0.2f, 0.8f, 1.4f), 1.2f);
    TestEqual(TEXT("stacked attack speed caps at the ceiling"), Ability::ComputeAttackSpeedPlayRate(3.0f, 0.8f, 1.4f), 1.4f);
    TestEqual(TEXT("a slow bottoms out at the floor"), Ability::ComputeAttackSpeedPlayRate(-0.9f, 0.8f, 1.4f), 0.8f);

    // A zero or negative rate stalls the montage, and PlayMontageAndWait then never completes.
    TestTrue(TEXT("a debuff that cancels attack speed still advances the montage"), Ability::ComputeAttackSpeedPlayRate(-1.0f, 0.0f, 1.4f) > 0.0f);
    TestTrue(TEXT("an over-stacked slow still advances the montage"), Ability::ComputeAttackSpeedPlayRate(-5.0f, 0.0f, 1.4f) > 0.0f);
    TestTrue(TEXT("an inverted band still advances the montage"), Ability::ComputeAttackSpeedPlayRate(0.0f, 1.4f, 0.8f) > 0.0f);

    return true;
}
