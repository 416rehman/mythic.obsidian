
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

    // A ceiling of exactly zero is the authored "no ceiling", so stacked attack speed keeps paying.
    TestEqual(TEXT("no ceiling lets stacked attack speed through"), Ability::ComputeAttackSpeedPlayRate(3.0f, 0.8f, 0.0f), 4.0f);
    TestEqual(TEXT("no ceiling still respects the floor"), Ability::ComputeAttackSpeedPlayRate(-0.9f, 0.8f, 0.0f), 0.8f);

    // Only an exact zero uncaps. A negative ceiling is corrupt data, not a request for no ceiling, and this very
    // field has been rewritten out of band before.
    TestEqual(TEXT("a negative ceiling is repaired, not read as uncapped"), Ability::ComputeAttackSpeedPlayRate(3.0f, 0.8f, -1.0f), 0.8f);

    // The ceiling used to be this path's only NaN sink, because FMath::Clamp returns the ceiling for a NaN input.
    // Uncapped there is no ceiling to fall back on, and a NaN rate stalls the montage forever.
    const float NaNBonus = FMath::Sqrt(-1.0f);
    TestEqual(TEXT("a NaN bonus falls back to the floor when uncapped"), Ability::ComputeAttackSpeedPlayRate(NaNBonus, 0.8f, 0.0f), 0.8f);
    TestEqual(TEXT("a NaN bonus falls back to the floor when capped"), Ability::ComputeAttackSpeedPlayRate(NaNBonus, 0.8f, 1.4f), 0.8f);
    // Loge(0) is -inf at runtime, so the non-finite guard is exercised rather than constant-folded.
    const float InfiniteBonus = -FMath::Loge(0.0f);
    TestEqual(TEXT("an infinite bonus falls back to the floor"), Ability::ComputeAttackSpeedPlayRate(InfiniteBonus, 0.8f, 0.0f), 0.8f);
    TestTrue(TEXT("an enormous finite bonus still yields a usable rate"),
             Ability::ComputeAttackSpeedPlayRate(TNumericLimits<float>::Max(), 0.8f, 0.0f) > 0.0f);

    return true;
}
