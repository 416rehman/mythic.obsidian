
#include "Misc/AutomationTest.h"
#include "World/Survival/SurvivalCore.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSurvivalTest,
    "Mythic.World.Survival",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSurvivalTest::RunTest(const FString &Parameters) {
    using Core = FMythicSurvivalCore;

    {
        TestEqual(TEXT("decay: normal decrement"), Core::ComputeDecayStep(100.0f, 0.05f, 2.0f, 100.0f), 99.9f);

        TestEqual(TEXT("decay: floors at 0"), Core::ComputeDecayStep(1.0f, 10.0f, 5.0f, 100.0f), 0.0f);
        TestTrue(TEXT("decay: result never < 0"), Core::ComputeDecayStep(0.0f, 5.0f, 100.0f, 100.0f) >= 0.0f);

        TestEqual(TEXT("decay: clamps to Max when over"), Core::ComputeDecayStep(150.0f, 0.0f, 1.0f, 100.0f), 100.0f);

        TestEqual(TEXT("decay: zero rate no-op"), Core::ComputeDecayStep(80.0f, 0.0f, 2.0f, 100.0f), 80.0f);
        TestEqual(TEXT("decay: zero delta no-op"), Core::ComputeDecayStep(80.0f, 5.0f, 0.0f, 100.0f), 80.0f);
        TestEqual(TEXT("decay: negative delta no-op"), Core::ComputeDecayStep(80.0f, 5.0f, -1.0f, 100.0f), 80.0f);

        float Prev = 100.0f;
        for (int32 i = 0; i < 50; ++i) {
            const float Cur = Core::ComputeDecayStep(Prev, 1.0f, 2.0f, 100.0f);
            TestTrue(TEXT("decay: monotonic non-increasing"), Cur <= Prev + KINDA_SMALL_NUMBER);
            Prev = Cur;
        }
        TestEqual(TEXT("decay: run bottoms out at 0"), Prev, 0.0f);
    }

    {
        FSurvivalThresholds T;
        const float SafeHyd = 0.5f, SafeWarm = 0.5f, Dry = 0.0f;

        uint8 M = Core::ResolveStatus(0.17f, SafeHyd, SafeWarm, Dry, T, ESSB_None);
        TestFalse(TEXT("starving: 0.17 from inactive → not starving"), (M & ESSB_Starving) != 0);

        M = Core::ResolveStatus(0.14f, SafeHyd, SafeWarm, Dry, T, ESSB_None);
        TestTrue(TEXT("starving: 0.14 → starving"), (M & ESSB_Starving) != 0);

        M = Core::ResolveStatus(0.17f, SafeHyd, SafeWarm, Dry, T, ESSB_Starving);
        TestTrue(TEXT("starving: 0.17 from active → stays starving (sticky band)"), (M & ESSB_Starving) != 0);

        M = Core::ResolveStatus(0.21f, SafeHyd, SafeWarm, Dry, T, ESSB_Starving);
        TestFalse(TEXT("starving: 0.21 → clears"), (M & ESSB_Starving) != 0);

        {
            uint8 Prev = ESSB_None;
            const float BandVals[] = {0.16f, 0.19f, 0.16f, 0.18f, 0.17f};
            for (float V : BandVals) {
                const uint8 Now = Core::ResolveStatus(V, SafeHyd, SafeWarm, Dry, T, Prev);
                TestEqual(TEXT("starving: no flicker inside dead-band"), (Now & ESSB_Starving), (Prev & ESSB_Starving));
                Prev = Now;
            }
        }

        M = Core::ResolveStatus(0.90f, SafeHyd, SafeWarm, Dry, T, ESSB_None);
        TestTrue(TEXT("wellfed: 0.90 → well fed"), (M & ESSB_WellFed) != 0);
        M = Core::ResolveStatus(0.82f, SafeHyd, SafeWarm, Dry, T, ESSB_WellFed);
        TestTrue(TEXT("wellfed: 0.82 from active → stays (sticky)"), (M & ESSB_WellFed) != 0);
        M = Core::ResolveStatus(0.79f, SafeHyd, SafeWarm, Dry, T, ESSB_WellFed);
        TestFalse(TEXT("wellfed: 0.79 → clears"), (M & ESSB_WellFed) != 0);

        M = Core::ResolveStatus(0.5f, 0.10f, SafeWarm, Dry, T, ESSB_None);
        TestTrue(TEXT("dehydrated: 0.10 → dehydrated"), (M & ESSB_Dehydrated) != 0);
        M = Core::ResolveStatus(0.5f, 0.25f, SafeWarm, Dry, T, ESSB_Dehydrated);
        TestFalse(TEXT("dehydrated: 0.25 → clears"), (M & ESSB_Dehydrated) != 0);
    }

    {
        FSurvivalThresholds T;

        uint8 M = Core::ResolveStatus(0.5f, 0.5f, 0.28f, 0.0f, T, ESSB_None);
        TestFalse(TEXT("cold: 0.28 warmth dry → not cold"), (M & ESSB_Cold) != 0);

        M = Core::ResolveStatus(0.5f, 0.5f, 0.28f, 1.0f, T, ESSB_None);
        TestTrue(TEXT("cold: 0.28 warmth soaked → cold (aggravated)"), (M & ESSB_Cold) != 0);

        M = Core::ResolveStatus(0.5f, 0.5f, 0.10f, 0.0f, T, ESSB_None);
        TestTrue(TEXT("cold: 0.10 warmth → cold"), (M & ESSB_Cold) != 0);
    }

    {
        FWarmthWetnessRates R;

        {
            const FSurvivalWarmthWetnessResult N =
                Core::ComputeWarmthWetnessNet( true, false, true, true, 50.0f, 40.0f, R, 2.0f);
            TestEqual(TEXT("net: warm source warms +16"), N.NetWarmthDelta, 16.0f);
            TestEqual(TEXT("net: warm source dries -8"), N.NetWetnessDelta, -8.0f);
        }

        {
            const FSurvivalWarmthWetnessResult Dry =
                Core::ComputeWarmthWetnessNet(false, false, true, false, 60.0f, 0.0f, R, 2.0f);
            TestEqual(TEXT("net: cold dry warmth -6"), Dry.NetWarmthDelta, -6.0f);

            const FSurvivalWarmthWetnessResult Wet =
                Core::ComputeWarmthWetnessNet(false, false, true, false, 60.0f, 100.0f, R, 2.0f);
            TestTrue(TEXT("net: cold WHEN WET loses more warmth than dry"), Wet.NetWarmthDelta < Dry.NetWarmthDelta);
            TestEqual(TEXT("net: cold soaked warmth -10"), Wet.NetWarmthDelta, -10.0f);
        }

        {
            const FSurvivalWarmthWetnessResult N =
                Core::ComputeWarmthWetnessNet(false, false, false, true, 50.0f, 0.0f, R, 2.0f);
            TestEqual(TEXT("net: rain wets +12"), N.NetWetnessDelta, 12.0f);
        }

        {
            const FSurvivalWarmthWetnessResult N =
                Core::ComputeWarmthWetnessNet(false, true, false, true, 50.0f, 30.0f, R, 2.0f);
            TestTrue(TEXT("net: sheltered in rain dries (delta <= 0)"), N.NetWetnessDelta <= 0.0f);
        }

        {
            const FSurvivalWarmthWetnessResult N =
                Core::ComputeWarmthWetnessNet(false, false, false, false, 30.0f, 0.0f, R, 2.0f);
            TestTrue(TEXT("net: passive drift up toward neutral"), N.NetWarmthDelta > 0.0f);
            TestTrue(TEXT("net: passive drift doesn't overshoot neutral"), 30.0f + N.NetWarmthDelta <= 50.0f + KINDA_SMALL_NUMBER);
        }

        {
            const FSurvivalWarmthWetnessResult N =
                Core::ComputeWarmthWetnessNet( true, false, false, false, 98.0f, 50.0f, R, 100.0f);
            TestTrue(TEXT("net: warmth never exceeds max"), 98.0f + N.NetWarmthDelta <= 100.0f + KINDA_SMALL_NUMBER);
            TestEqual(TEXT("net: warmth tops out exactly at max"), 98.0f + N.NetWarmthDelta, 100.0f);
        }

        {
            const FSurvivalWarmthWetnessResult N =
                Core::ComputeWarmthWetnessNet(false, false, true, false, 2.0f, 0.0f, R, 100.0f);
            TestTrue(TEXT("net: warmth never goes below 0"), 2.0f + N.NetWarmthDelta >= -KINDA_SMALL_NUMBER);
            TestEqual(TEXT("net: warmth bottoms out exactly at 0"), 2.0f + N.NetWarmthDelta, 0.0f);
        }
    }

    {
        TestTrue(TEXT("gate: enabled + set + alive → active"), Core::IsSurvivalActive(true, true, false));
        TestFalse(TEXT("gate: master OFF → inactive (today's behaviour)"), Core::IsSurvivalActive(false, true, false));
        TestFalse(TEXT("gate: no set → inactive"), Core::IsSurvivalActive(true, false, false));
        TestFalse(TEXT("gate: dead → inactive"), Core::IsSurvivalActive(true, true, true));
        TestFalse(TEXT("gate: all off → inactive"), Core::IsSurvivalActive(false, false, true));
    }

    return true;
}
