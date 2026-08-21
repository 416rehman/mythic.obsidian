
#include "Misc/AutomationTest.h"
#include "AI/MonsterAffixes/MonsterAffixTypes.h"
#include "AI/MonsterAffixes/MonsterAffixPool.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMonsterAffixWiringTest,
    "Mythic.Combat.MonsterAffixWiring",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMonsterAffixWiringTest::RunTest(const FString &Parameters) {
    using S = FMonsterAffixSelector;

    TestEqual(TEXT("tier 1 (Normal) budget is 0"), S::ComputeAffixBudget(1, 0), 0);
    TestEqual(TEXT("tier 2 (Superior) budget is 0"), S::ComputeAffixBudget(2, 0), 0);
    TestEqual(TEXT("tier 1 budget stays 0 even at Extreme danger"), S::ComputeAffixBudget(1, 4), 0);

    const int32 EliteSafe = S::ComputeAffixBudget(3, 0);
    const int32 EliteExtreme = S::ComputeAffixBudget(3, 4);
    TestTrue(TEXT("Elite has a positive budget"), EliteSafe > 0);
    TestTrue(TEXT("danger widens the Elite budget"), EliteExtreme > EliteSafe);
    TestTrue(TEXT("Boss out-budgets Champion out-budgets Elite"),
             S::ComputeAffixBudget(5, 0) > S::ComputeAffixBudget(4, 0)
             && S::ComputeAffixBudget(4, 0) > EliteSafe);

    const TArray<FMonsterAffixDef> &DefaultPool = UMonsterAffixPool::GetDefaultPool();
    TestTrue(TEXT("code-default affix pool is non-empty"), DefaultPool.Num() > 0);

    {
        FRandomStream A(12345);
        FRandomStream B(12345);
        const TArray<FGameplayTag> PickA = S::Select(4, 2, S::ComputeAffixBudget(4, 2), DefaultPool, A);
        const TArray<FGameplayTag> PickB = S::Select(4, 2, S::ComputeAffixBudget(4, 2), DefaultPool, B);
        TestEqual(TEXT("same seed => same affix count"), PickA.Num(), PickB.Num());
        for (int32 i = 0; i < PickA.Num(); ++i) {
            TestTrue(TEXT("same seed => same affix at each index"), PickA[i] == PickB[i]);
        }
    }

    for (int32 Seed = 0; Seed < 32; ++Seed) {
        FRandomStream Rng(Seed);
        const int32 Budget = S::ComputeAffixBudget(5, 4);
        const TArray<FGameplayTag> Picked = S::Select(5, 4, Budget, DefaultPool, Rng);

        int32 SpentCost = 0;
        TSet<FGameplayTag> Seen;
        for (const FGameplayTag &Tag : Picked) {
            TestFalse(TEXT("no affix is selected twice"), Seen.Contains(Tag));
            Seen.Add(Tag);
            for (const FMonsterAffixDef &Def : DefaultPool) {
                if (Def.AffixTag == Tag) {
                    SpentCost += Def.BudgetCost;
                    break;
                }
            }
        }
        TestTrue(TEXT("selection never overspends the budget"), SpentCost <= Budget);
    }

    {
        FRandomStream Rng(7);
        TestEqual(TEXT("tier 1 selects nothing even with a forced budget"),
                  S::Select(1, 4, 99, DefaultPool, Rng).Num(), 0);
    }

    return true;
}
