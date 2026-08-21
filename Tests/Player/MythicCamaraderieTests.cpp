
#include "Misc/AutomationTest.h"
#include "World/Feedback/MythicCamaraderieCore.h"

namespace {
static FVector CamaraderieTest_LocAtX(double X) {
    return FVector(X, 0.0, 0.0);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCamaraderieTest,
    "Mythic.Coop.Camaraderie",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCamaraderieTest::RunTest(const FString &Parameters) {
    using namespace MythicCamaraderie;

    const FVector Self = FVector::ZeroVector;
    const double Radius = 1000.0;
    const double RadiusSq = Radius * Radius;

    {
        TestEqual(TEXT("count: empty view → 0"),
                  CountAlliesInRadius(Self, TArray<FVector>{}, RadiusSq, 4), 0);

        {
            TArray<FVector> Locs = {CamaraderieTest_LocAtX(1500.0), CamaraderieTest_LocAtX(-2000.0)};
            TestEqual(TEXT("count: all out of range → 0"), CountAlliesInRadius(Self, Locs, RadiusSq, 4), 0);
        }

        {
            TArray<FVector> Locs = {CamaraderieTest_LocAtX(100.0), CamaraderieTest_LocAtX(-500.0)};
            TestEqual(TEXT("count: 2 in range → 2"), CountAlliesInRadius(Self, Locs, RadiusSq, 8), 2);
        }

        {
            TArray<FVector> Locs = {CamaraderieTest_LocAtX(100.0), CamaraderieTest_LocAtX(1500.0),
                                    CamaraderieTest_LocAtX(-900.0), CamaraderieTest_LocAtX(5000.0)};
            TestEqual(TEXT("count: mixed → only in-range (2)"), CountAlliesInRadius(Self, Locs, RadiusSq, 8), 2);
        }

        {
            TArray<FVector> Locs = {CamaraderieTest_LocAtX(Radius)};
            TestEqual(TEXT("count: exactly at radius → counts (inclusive boundary)"),
                      CountAlliesInRadius(Self, Locs, RadiusSq, 4), 1);
            TArray<FVector> Past = {CamaraderieTest_LocAtX(Radius + 1.0)};
            TestEqual(TEXT("count: just past radius → 0"), CountAlliesInRadius(Self, Past, RadiusSq, 4), 0);
        }

        {
            TArray<FVector> Locs = {CamaraderieTest_LocAtX(10.0), CamaraderieTest_LocAtX(20.0),
                                    CamaraderieTest_LocAtX(30.0), CamaraderieTest_LocAtX(40.0),
                                    CamaraderieTest_LocAtX(50.0)};
            TestEqual(TEXT("count: 5 in range clamped to cap 3"), CountAlliesInRadius(Self, Locs, RadiusSq, 3), 3);
            TestEqual(TEXT("count: MaxStacks 0 → 0"), CountAlliesInRadius(Self, Locs, RadiusSq, 0), 0);
        }
    }

    {
        for (int32 S = 0; S <= 5; ++S) {
            TestEqual(FString::Printf(TEXT("bonus: PerAllyBonus 0 → 0 at stacks=%d (inert)"), S),
                      EffectiveBonus(S, 0.0f), 0.0f);
        }

        TestEqual(TEXT("bonus: 0 stacks → 0"), EffectiveBonus(0, 2.5f), 0.0f);
        TestEqual(TEXT("bonus: negative stacks → 0"), EffectiveBonus(-3, 2.5f), 0.0f);

        TestEqual(TEXT("bonus: 1 stack × 5 → 5"), EffectiveBonus(1, 5.0f), 5.0f);
        TestEqual(TEXT("bonus: 3 stacks × 5 → 15"), EffectiveBonus(3, 5.0f), 15.0f);

        float Prev = -1.0f;
        for (int32 S = 0; S <= 4; ++S) {
            const float B = EffectiveBonus(S, 2.0f);
            TestTrue(TEXT("bonus: monotonic non-decreasing in stacks"), B >= Prev);
            Prev = B;
        }

        {
            TArray<FVector> Locs;
            for (int32 i = 0; i < 6; ++i) {
                Locs.Add(CamaraderieTest_LocAtX(10.0 * (i + 1)));
            }
            const int32 Stacks = CountAlliesInRadius(Self, Locs, RadiusSq, 3);
            TestEqual(TEXT("pipeline: 6 allies clamped to cap 3"), Stacks, 3);
            TestEqual(TEXT("pipeline: capped bonus = 3 × 4 = 12"), EffectiveBonus(Stacks, 4.0f), 12.0f);
        }
    }

    return true;
}
