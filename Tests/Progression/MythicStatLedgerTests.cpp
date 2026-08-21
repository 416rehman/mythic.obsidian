
#include "Misc/AutomationTest.h"
#include "Progression/MythicStatLedger.h"
#include "Progression/MythicStatCounterTypes.h"
#include "Progression/MythicTags_MetaProgression.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatLedgerTest,
    "Mythic.Progression.StatLedger",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatLedgerTest::RunTest(const FString &Parameters) {
    using Ledger = FMythicStatLedger;

    const FGameplayTag KillGeneric = STAT_KILL_GENERIC;
    const FGameplayTag KillBoss = STAT_KILL_BOSS;
    const FGameplayTag Death = STAT_DEATH;
    const FGameplayTag KillPrefix = KillGeneric.RequestDirectParent();
    const FGameplayTag StatRoot = KillPrefix.RequestDirectParent();

    {
        TArray<FMythicStatCounter> Counters;

        TestEqual(TEXT("first ApplyDelta creates and returns the delta"), Ledger::ApplyDelta(Counters, KillGeneric, 5), (int64)5);
        TestEqual(TEXT("one counter after create"), Counters.Num(), 1);

        TestEqual(TEXT("second ApplyDelta accumulates"), Ledger::ApplyDelta(Counters, KillGeneric, 3), (int64)8);
        TestEqual(TEXT("still one counter after accumulate"), Counters.Num(), 1);

        TestEqual(TEXT("underflowing delta clamps to 0"), Ledger::ApplyDelta(Counters, KillGeneric, -100), (int64)0);

        TestEqual(TEXT("negative-delta create clamps to 0"), Ledger::ApplyDelta(Counters, Death, -7), (int64)0);
    }

    {
        TArray<FMythicStatCounter> Counters;
        Ledger::ApplyDelta(Counters, KillGeneric, 42);
        TestEqual(TEXT("FindValue hit"), Ledger::FindValue(Counters, KillGeneric), (int64)42);
        TestEqual(TEXT("FindValue miss is 0"), Ledger::FindValue(Counters, Death), (int64)0);
        TestEqual(TEXT("FindValue miss did not create a counter"), Counters.Num(), 1);
    }

    {
        TArray<FMythicStatCounter> Counters;
        Ledger::ApplyDelta(Counters, KillGeneric, 3);
        Ledger::ApplyDelta(Counters, KillBoss, 2);
        Ledger::ApplyDelta(Counters, Death, 1);

        TestEqual(TEXT("Stat.Kill rolls up Generic + Boss"), Ledger::SumByPrefix(Counters, KillPrefix), (int64)5);
        TestEqual(TEXT("Stat.Death rolls up only Death"), Ledger::SumByPrefix(Counters, Death), (int64)1);
        TestEqual(TEXT("Stat root rolls up everything"), Ledger::SumByPrefix(Counters, StatRoot), (int64)6);

        TestEqual(TEXT("unrecorded prefix sums to 0"), Ledger::SumByPrefix(Counters, STAT_GOLD_EARNED), (int64)0);
    }

    {
        TestTrue(TEXT("9->10 crosses 10"), Ledger::DidCrossThreshold(9, 10, 10));
        TestFalse(TEXT("8->9 does not cross 10"), Ledger::DidCrossThreshold(8, 9, 10));
        TestFalse(TEXT("already-crossed 10->11 does not re-fire"), Ledger::DidCrossThreshold(10, 11, 10));
        TestFalse(TEXT("at-threshold 10->10 does not fire (already at/over)"), Ledger::DidCrossThreshold(10, 10, 10));
        TestTrue(TEXT("0->10 jump crosses 10"), Ledger::DidCrossThreshold(0, 10, 10));
        TestTrue(TEXT("0->100 crosses 10"), Ledger::DidCrossThreshold(0, 100, 10));
    }

    return true;
}
