
#include "Misc/AutomationTest.h"
#include "World/Farming/MythicFarmingRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFarmingTest,
    "Mythic.World.Farming",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFarmingTest::RunTest(const FString &Parameters) {
    using Rules = FMythicFarmingRules;

    {
        const TArray<float> Durations = {10.0f, 20.0f, 30.0f};
        auto Adv = [&](float Elapsed) { return Rules::ResolveStageAdvance(Elapsed, Durations); };

        TestEqual(TEXT("t=0 → stage 0"), Adv(0.0f).NewStageIndex, 0);
        TestEqual(TEXT("t=0 → 10s to next"), Adv(0.0f).RemainingToNextStage, 10.0f);

        TestEqual(TEXT("t=5 → stage 0"), Adv(5.0f).NewStageIndex, 0);
        TestEqual(TEXT("t=5 → 5s to next"), Adv(5.0f).RemainingToNextStage, 5.0f);

        TestEqual(TEXT("t=10 (boundary) → stage 1"), Adv(10.0f).NewStageIndex, 1);
        TestEqual(TEXT("t=10 → 20s to next"), Adv(10.0f).RemainingToNextStage, 20.0f);

        TestEqual(TEXT("t=15 → stage 1"), Adv(15.0f).NewStageIndex, 1);
        TestEqual(TEXT("t=15 → 15s to next"), Adv(15.0f).RemainingToNextStage, 15.0f);
        TestEqual(TEXT("t=30 (boundary) → stage 2"), Adv(30.0f).NewStageIndex, 2);
        TestEqual(TEXT("t=30 → 30s to next"), Adv(30.0f).RemainingToNextStage, 30.0f);

        TestEqual(TEXT("t=45 → stage 2"), Adv(45.0f).NewStageIndex, 2);
        TestEqual(TEXT("t=45 → 15s to next"), Adv(45.0f).RemainingToNextStage, 15.0f);

        TestEqual(TEXT("t=60 (final boundary) → mature stage 3"), Adv(60.0f).NewStageIndex, 3);
        TestEqual(TEXT("t=60 → 0s remaining"), Adv(60.0f).RemainingToNextStage, 0.0f);
        TestEqual(TEXT("t=1000 (overshoot) → still mature 3"), Adv(1000.0f).NewStageIndex, 3);
        TestEqual(TEXT("t=1000 → 0s remaining"), Adv(1000.0f).RemainingToNextStage, 0.0f);

        TestEqual(TEXT("t=-5 clamps → stage 0"), Adv(-5.0f).NewStageIndex, 0);
        TestEqual(TEXT("t=-5 → 10s to next"), Adv(-5.0f).RemainingToNextStage, 10.0f);

        const TArray<float> None;
        TestEqual(TEXT("no stages → stage 0"), Rules::ResolveStageAdvance(50.0f, None).NewStageIndex, 0);
        TestEqual(TEXT("no stages → 0s remaining"), Rules::ResolveStageAdvance(50.0f, None).RemainingToNextStage, 0.0f);
    }

    {
        TestTrue(TEXT("empty + seed + level ok → can plant"), Rules::CanPlant(true, true, 5, 3));
        TestTrue(TEXT("empty + seed + no level requirement → can plant"), Rules::CanPlant(true, true, 0, 0));
        TestTrue(TEXT("exactly at min level → can plant"), Rules::CanPlant(true, true, 3, 3));
        TestFalse(TEXT("occupied plot → cannot plant"), Rules::CanPlant(false, true, 5, 3));
        TestFalse(TEXT("no seed → cannot plant"), Rules::CanPlant(true, false, 5, 3));
        TestFalse(TEXT("level below min → cannot plant"), Rules::CanPlant(true, true, 2, 3));
    }

    {
        TestTrue(TEXT("stage == mature → harvestable"), Rules::CanHarvest(3, 3));
        TestTrue(TEXT("stage past mature → harvestable"), Rules::CanHarvest(4, 3));
        TestFalse(TEXT("below mature → not harvestable"), Rules::CanHarvest(2, 3));
        TestFalse(TEXT("negative mature → never harvestable"), Rules::CanHarvest(0, -1));
    }

    {
        TestEqual(TEXT("base 0 → 0"), Rules::ComputeHarvestYield(0, 10, 0.1f, 0.0f), 0);

        TestEqual(TEXT("base 2 lvl 0 → 2"), Rules::ComputeHarvestYield(2, 0, 0.1f, 0.99f), 2);

        TestEqual(TEXT("base 2 lvl 5 bonus .1 → 3 (roll low, snapped)"), Rules::ComputeHarvestYield(2, 5, 0.1f, 0.0f), 3);
        TestEqual(TEXT("base 2 lvl 5 bonus .1 → 3 (roll high)"), Rules::ComputeHarvestYield(2, 5, 0.1f, 1.0f), 3);

        TestEqual(TEXT("2.5 expected, roll 0.4 → 3"), Rules::ComputeHarvestYield(2, 1, 0.25f, 0.4f), 3);
        TestEqual(TEXT("2.5 expected, roll 0.9 → 2"), Rules::ComputeHarvestYield(2, 1, 0.25f, 0.9f), 2);
        TestEqual(TEXT("2.5 expected, roll exactly 0.5 → 2 (strict <)"), Rules::ComputeHarvestYield(2, 1, 0.25f, 0.5f), 2);

        TestEqual(TEXT("negative level clamps → base"), Rules::ComputeHarvestYield(3, -4, 0.1f, 0.0f), 3);

        int32 Prev = Rules::ComputeHarvestYield(4, 0, 0.05f, 0.0f);
        for (int32 Level = 1; Level <= 20; ++Level) {
            const int32 Cur = Rules::ComputeHarvestYield(4, Level, 0.05f, 0.0f);
            TestTrue(TEXT("yield is monotonic non-decreasing in level"), Cur >= Prev);
            Prev = Cur;
        }
        TestTrue(TEXT("high level out-yields base"), Rules::ComputeHarvestYield(4, 20, 0.05f, 0.0f) > Rules::ComputeHarvestYield(4, 0, 0.05f, 0.0f));
    }

    {
        const double Now = 1000.0;
        const double R = 123.5;
        const double Deadline = Rules::RebuildDeadline(Now, R);
        TestEqual(TEXT("rebuild deadline = now + remaining"), Deadline, 1123.5);
        TestEqual(TEXT("round-trip recovers remaining"), Rules::RemainingSecondsToNextStage(Deadline, Now), R);

        TestEqual(TEXT("past-due deadline floors at 0"), Rules::RemainingSecondsToNextStage(500.0, 1000.0), 0.0);
        TestEqual(TEXT("negative remaining floors deadline at now"), Rules::RebuildDeadline(Now, -5.0), Now);
    }

    return true;
}
