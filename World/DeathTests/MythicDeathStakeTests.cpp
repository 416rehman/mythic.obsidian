
#include "Misc/AutomationTest.h"
#include "World/Death/MythicDeathStakeTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDeathStakeTest,
    "Mythic.Death.PlayerStake",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDeathStakeTest::RunTest(const FString &Parameters) {
    using Rules = FMythicDeathStakeRules;

    FMythicDeathStakeConfig Cfg;
    Cfg.StakeFractionOfGold = 0.25f;
    Cfg.DangerScaleMax = 2.0f;
    Cfg.MinStake = 10;
    Cfg.MaxStake = 1000;

    {
        TestEqual(TEXT("0 gold → 0 stake (safe)"), Rules::ComputeStakeAmount(0, 0.0f, Cfg), 0);
        TestEqual(TEXT("0 gold → 0 stake (extreme)"), Rules::ComputeStakeAmount(0, 1.0f, Cfg), 0);
        TestEqual(TEXT("negative gold → 0 stake"), Rules::ComputeStakeAmount(-500, 1.0f, Cfg), 0);

        TestEqual(TEXT("1000 gold, danger 0 → 250"), Rules::ComputeStakeAmount(1000, 0.0f, Cfg), 250);
        TestEqual(TEXT("1000 gold, danger 1 → 500"), Rules::ComputeStakeAmount(1000, 1.0f, Cfg), 500);
        {
            const int32 Mid = Rules::ComputeStakeAmount(1000, 0.5f, Cfg);
            TestTrue(TEXT("danger 0.5 stake between danger 0 and danger 1"), Mid > 250 && Mid < 500);
        }

        {
            const int32 Low = Rules::ComputeStakeAmount(400, 0.5f, Cfg);
            const int32 High = Rules::ComputeStakeAmount(800, 0.5f, Cfg);
            TestTrue(TEXT("more gold → strictly larger stake"), High > Low);
        }

        {
            const int32 Safe = Rules::ComputeStakeAmount(800, 0.0f, Cfg);
            const int32 Deadly = Rules::ComputeStakeAmount(800, 1.0f, Cfg);
            TestTrue(TEXT("more danger → strictly larger stake"), Deadly > Safe);
        }

        {
            int32 Prev = Rules::ComputeStakeAmount(0, 0.7f, Cfg);
            for (int32 Gold = 0; Gold <= 5000; Gold += 25) {
                const int32 Cur = Rules::ComputeStakeAmount(Gold, 0.7f, Cfg);
                TestTrue(*FString::Printf(TEXT("stake non-decreasing in gold @%d"), Gold), Cur >= Prev);
                TestTrue(*FString::Printf(TEXT("stake never exceeds carried gold @%d"), Gold), Cur <= Gold);
                Prev = Cur;
            }
        }

        {
            int32 Prev = Rules::ComputeStakeAmount(2000, 0.0f, Cfg);
            for (float D = 0.0f; D <= 1.0f; D += 0.05f) {
                const int32 Cur = Rules::ComputeStakeAmount(2000, D, Cfg);
                TestTrue(*FString::Printf(TEXT("stake non-decreasing in danger @%.2f"), D), Cur >= Prev);
                Prev = Cur;
            }
        }

        TestEqual(TEXT("tiny stake raised to MinStake floor"), Rules::ComputeStakeAmount(20, 0.0f, Cfg), Cfg.MinStake);
        TestEqual(TEXT("floor clamped to carried gold (5 < MinStake)"), Rules::ComputeStakeAmount(5, 0.0f, Cfg), 5);

        TestEqual(TEXT("huge stake capped to MaxStake"), Rules::ComputeStakeAmount(100000, 1.0f, Cfg), Cfg.MaxStake);

        {
            FMythicDeathStakeConfig Greedy;
            Greedy.StakeFractionOfGold = 1.0f;
            Greedy.DangerScaleMax = 5.0f;
            Greedy.MinStake = 0;
            Greedy.MaxStake = 1000000;
            TestEqual(TEXT("100% fraction × 5x danger still ≤ carried gold"),
                      Rules::ComputeStakeAmount(300, 1.0f, Greedy), 300);
        }

        {
            FMythicDeathStakeConfig Off = Cfg;
            Off.StakeFractionOfGold = 0.0f;
            TestEqual(TEXT("0 fraction → 0 stake"), Rules::ComputeStakeAmount(1000, 1.0f, Off), 0);
        }
    }

    {
        TestTrue(TEXT("owner, in range → recover"),
                 Rules::CanRecover( true, false, true));
        TestTrue(TEXT("party member, in range → recover"),
                 Rules::CanRecover(false, true, true));
        TestTrue(TEXT("owner+party, in range → recover"),
                 Rules::CanRecover(true, true, true));
        TestFalse(TEXT("stranger, in range → denied"),
                  Rules::CanRecover(false, false, true));
        TestFalse(TEXT("owner, out of range → denied"),
                  Rules::CanRecover(true, false, false));
        TestFalse(TEXT("party member, out of range → denied"),
                  Rules::CanRecover(false, true, false));
        TestFalse(TEXT("stranger, out of range → denied"),
                  Rules::CanRecover(false, false, false));
    }

    return true;
}
