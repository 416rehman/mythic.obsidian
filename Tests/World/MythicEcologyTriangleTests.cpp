
#include "Misc/AutomationTest.h"
#include "Math/UnrealMathUtility.h"

namespace {
    int32 EcoTriCarrionBonus(float Carrion, int32 TargetCount, int32 MaxEntitiesPerCell) {
        if (Carrion <= 0.0f) {
            return 0;
        }
        return FMath::Clamp(FMath::RoundToInt(Carrion), 0, FMath::Max(0, MaxEntitiesPerCell - TargetCount));
    }

    float EcoTriEffectiveWeight(float BaseWeight, bool bIsScavenger, float Carrion) {
        const float Base = FMath::Max(0.0f, BaseWeight);
        return (bIsScavenger && Carrion > 0.0f) ? Base * (1.0f + Carrion) : Base;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEcologyTriangleTest,
    "Mythic.World.EcologyTriangle",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEcologyTriangleTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("no carrion => no deficit bonus"), EcoTriCarrionBonus(0.0f, 6, 20), 0);
    TestEqual(TEXT("negative carrion => no deficit bonus"), EcoTriCarrionBonus(-3.0f, 6, 20), 0);
    TestEqual(TEXT("no carrion => scavenger weight unchanged"), EcoTriEffectiveWeight(2.0f, true, 0.0f), 2.0f);
    TestEqual(TEXT("no carrion => non-scavenger weight unchanged"), EcoTriEffectiveWeight(2.0f, false, 0.0f), 2.0f);

    TestEqual(TEXT("carrion adds to the deficit"), EcoTriCarrionBonus(3.0f, 6, 20), 3);

    TestEqual(TEXT("bonus is capped at headroom"), EcoTriCarrionBonus(50.0f, 6, 20), 14);
    TestEqual(TEXT("no headroom => no bonus"), EcoTriCarrionBonus(50.0f, 20, 20), 0);
    TestEqual(TEXT("target already over cap => no bonus (never negative)"), EcoTriCarrionBonus(50.0f, 25, 20), 0);
    for (int32 Target = 0; Target <= 25; ++Target) {
        const int32 Bonus = EcoTriCarrionBonus(99.0f, Target, 20);
        TestTrue(TEXT("bonus is never negative"), Bonus >= 0);
        TestTrue(TEXT("target + bonus never exceeds the cap"), Target >= 20 || (Target + Bonus) <= 20);
    }

    TestEqual(TEXT("scavenger amplified by (1 + carrion)"), EcoTriEffectiveWeight(2.0f, true, 1.5f), 5.0f);
    TestEqual(TEXT("non-scavenger untouched at high carrion"), EcoTriEffectiveWeight(2.0f, false, 1.5f), 2.0f);

    float Prev = EcoTriEffectiveWeight(1.0f, true, 0.0f);
    for (float C = 0.5f; C <= 5.0f; C += 0.5f) {
        const float Now = EcoTriEffectiveWeight(1.0f, true, C);
        TestTrue(TEXT("scavenger weight is monotonic in carrion"), Now >= Prev);
        Prev = Now;
    }
    TestEqual(TEXT("zero-weight species stays unspawnable"), EcoTriEffectiveWeight(0.0f, true, 5.0f), 0.0f);
    TestEqual(TEXT("negative authored weight is floored to 0"), EcoTriEffectiveWeight(-4.0f, true, 5.0f), 0.0f);

    return true;
}
