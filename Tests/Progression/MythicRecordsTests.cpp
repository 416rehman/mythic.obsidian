
#include "Misc/AutomationTest.h"
#include "Progression/MythicStatLedger.h"
#include "Progression/MythicStatCounterTypes.h"
#include "Progression/MythicTags_MetaProgression.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRecordsTest,
    "Mythic.Progression.Records",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRecordsTest::RunTest(const FString &Parameters) {
    using Ledger = FMythicStatLedger;

    const FGameplayTag RecordTag = STAT_KILL_GENERIC;
    const FGameplayTag OtherTag = STAT_KILL_BOSS;

    {
        TArray<FMythicStatCounter> Counters;
        bool bNewRecord = false;
        TestEqual(TEXT("first positive value is stored"), Ledger::ApplyMax(Counters, RecordTag, 120, &bNewRecord), (int64)120);
        TestTrue(TEXT("first positive value IS a new record"), bNewRecord);
        TestEqual(TEXT("one counter created"), Counters.Num(), 1);
    }

    {
        TArray<FMythicStatCounter> Counters;
        bool bNewRecord = true;
        TestEqual(TEXT("zero 'record' stores nothing"), Ledger::ApplyMax(Counters, RecordTag, 0, &bNewRecord), (int64)0);
        TestFalse(TEXT("zero 'record' fires no edge"), bNewRecord);
        TestEqual(TEXT("no counter created for zero"), Counters.Num(), 0);

        bNewRecord = true;
        TestEqual(TEXT("negative 'record' clamps to 0 and stores nothing"), Ledger::ApplyMax(Counters, RecordTag, -50, &bNewRecord), (int64)0);
        TestFalse(TEXT("negative 'record' fires no edge"), bNewRecord);
        TestEqual(TEXT("no counter created for negative"), Counters.Num(), 0);
    }

    {
        TArray<FMythicStatCounter> Counters;
        Ledger::ApplyMax(Counters, RecordTag, 100);

        bool bNewRecord = true;
        TestEqual(TEXT("smaller catch never erodes the record"), Ledger::ApplyMax(Counters, RecordTag, 60, &bNewRecord), (int64)100);
        TestFalse(TEXT("smaller catch fires no edge"), bNewRecord);

        bNewRecord = true;
        TestEqual(TEXT("EQUAL value holds the record"), Ledger::ApplyMax(Counters, RecordTag, 100, &bNewRecord), (int64)100);
        TestFalse(TEXT("equal value does NOT re-fire (idempotence — no trophy re-mint)"), bNewRecord);

        bNewRecord = false;
        TestEqual(TEXT("bigger catch raises the record"), Ledger::ApplyMax(Counters, RecordTag, 250, &bNewRecord), (int64)250);
        TestTrue(TEXT("the raise fires the new-record edge"), bNewRecord);

        bNewRecord = true;
        TestEqual(TEXT("negative probe cannot lower an existing record"), Ledger::ApplyMax(Counters, RecordTag, -10, &bNewRecord), (int64)250);
        TestFalse(TEXT("negative probe fires no edge"), bNewRecord);

        TestEqual(TEXT("still exactly one counter for the species"), Counters.Num(), 1);
    }

    {
        TArray<FMythicStatCounter> Counters;
        TestEqual(TEXT("null bOutNewRecord is accepted"), Ledger::ApplyMax(Counters, RecordTag, 42, nullptr), (int64)42);
        Ledger::ApplyMax(Counters, OtherTag, 7);
        TestEqual(TEXT("two species → two independent records"), Counters.Num(), 2);
        TestEqual(TEXT("record reads back via FindValue"), Ledger::FindValue(Counters, RecordTag), (int64)42);
        TestEqual(TEXT("other record unaffected"), Ledger::FindValue(Counters, OtherTag), (int64)7);
        Ledger::ApplyMax(Counters, RecordTag, 41);
        TestEqual(TEXT("cross-tag ApplyMax never bleeds"), Ledger::FindValue(Counters, OtherTag), (int64)7);
    }

    return true;
}
