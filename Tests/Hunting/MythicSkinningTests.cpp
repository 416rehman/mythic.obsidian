
#include "Misc/AutomationTest.h"
#include "World/Hunting/MythicSkinningRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSkinningTest,
    "Mythic.Hunting.Skinning",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSkinningTest::RunTest(const FString &Parameters) {
    using Rules = FMythicSkinningRules;

    {
        TestEqual(TEXT("no cap → pays base at low level"), Rules::ComputeSkinXpReward(15.0f, 0, 0), 15.0f);
        TestEqual(TEXT("no cap → pays base at high level"), Rules::ComputeSkinXpReward(15.0f, 999, 0), 15.0f);

        TestEqual(TEXT("below cap → pays base"), Rules::ComputeSkinXpReward(15.0f, 9, 10), 15.0f);
        TestEqual(TEXT("exactly at cap → pays 0"), Rules::ComputeSkinXpReward(15.0f, 10, 10), 0.0f);
        TestEqual(TEXT("above cap → pays 0"), Rules::ComputeSkinXpReward(15.0f, 25, 10), 0.0f);

        TestEqual(TEXT("zero base → 0"), Rules::ComputeSkinXpReward(0.0f, 0, 0), 0.0f);
        TestEqual(TEXT("negative base → 0"), Rules::ComputeSkinXpReward(-5.0f, 0, 0), 0.0f);
    }

    {
        const float PerLevel = 0.5f;
        const float PerTier = 1.0f;

        TestEqual(TEXT("base yield at lvl0 tier1"), Rules::ComputeYieldCount(1, 0, 1, PerLevel, PerTier), 1);
        TestEqual(TEXT("+level bonus (floored)"), Rules::ComputeYieldCount(1, 2, 1, PerLevel, PerTier), 2);
        TestEqual(TEXT("fractional level bonus floors"), Rules::ComputeYieldCount(1, 1, 1, PerLevel, PerTier), 1);
        TestEqual(TEXT("+tier bonus"), Rules::ComputeYieldCount(1, 0, 3, PerLevel, PerTier), 3);
        TestEqual(TEXT("boss + level stack"), Rules::ComputeYieldCount(1, 4, 5, PerLevel, PerTier), 7);

        TestEqual(TEXT("tier 0 (unknown) == tier 1"),
                  Rules::ComputeYieldCount(1, 3, 0, PerLevel, PerTier), Rules::ComputeYieldCount(1, 3, 1, PerLevel, PerTier));

        TestEqual(TEXT("negative base clamps"), Rules::ComputeYieldCount(-5, 0, 1, PerLevel, PerTier), 0);
        TestEqual(TEXT("negative level clamps"), Rules::ComputeYieldCount(1, -9, 1, PerLevel, PerTier), 1);

        int32 PrevL = Rules::ComputeYieldCount(1, 0, 2, PerLevel, PerTier);
        for (int32 L = 0; L <= 40; ++L) {
            const int32 Cur = Rules::ComputeYieldCount(1, L, 2, PerLevel, PerTier);
            TestTrue(*FString::Printf(TEXT("yield non-decreasing @lvl %d"), L), Cur >= PrevL);
            PrevL = Cur;
        }
        int32 PrevT = Rules::ComputeYieldCount(1, 5, 1, PerLevel, PerTier);
        for (int32 T = 1; T <= 5; ++T) {
            const int32 Cur = Rules::ComputeYieldCount(1, 5, T, PerLevel, PerTier);
            TestTrue(*FString::Printf(TEXT("yield non-decreasing @tier %d"), T), Cur >= PrevT);
            PrevT = Cur;
        }
    }

    {
        {
            FRandomStream R(1);
            TestEqual(TEXT("0 available → empty"), Rules::RollYield(0, 3, R).Num(), 0);
            TestEqual(TEXT("0 count → empty"), Rules::RollYield(5, 0, R).Num(), 0);
            TestEqual(TEXT("negative count → empty"), Rules::RollYield(5, -2, R).Num(), 0);
        }

        {
            FRandomStream R(7);
            const TArray<int32> Picks = Rules::RollYield(4, 10, R);
            TestEqual(TEXT("count clamps to available"), Picks.Num(), 4);
        }

        {
            FRandomStream R(42);
            const int32 N = 8;
            const TArray<int32> Picks = Rules::RollYield(N, 5, R);
            TestEqual(TEXT("partial draw size"), Picks.Num(), 5);
            TSet<int32> Seen;
            for (const int32 Idx : Picks) {
                TestTrue(TEXT("index in range"), Idx >= 0 && Idx < N);
                TestFalse(TEXT("index is unique"), Seen.Contains(Idx));
                Seen.Add(Idx);
            }
        }

        {
            FRandomStream A(12345);
            FRandomStream B(12345);
            const TArray<int32> PA = Rules::RollYield(10, 6, A);
            const TArray<int32> PB = Rules::RollYield(10, 6, B);
            TestTrue(TEXT("same seed → identical roll"), PA == PB);
        }

        {
            FRandomStream R(99);
            const int32 N = 6;
            const TArray<int32> Full = Rules::RollYield(N, N, R);
            TestEqual(TEXT("full draw size == N"), Full.Num(), N);
            TSet<int32> Seen(Full);
            TestEqual(TEXT("full draw is a permutation (all unique)"), Seen.Num(), N);
            for (int32 i = 0; i < N; ++i) {
                TestTrue(*FString::Printf(TEXT("permutation contains %d"), i), Seen.Contains(i));
            }
        }
    }

    return true;
}
