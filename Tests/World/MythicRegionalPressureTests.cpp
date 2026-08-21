
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureRules.h"
#include "World/LivingWorld/Pressure/MythicTags_Pressure.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRegionalPressureTest,
    "Mythic.World.RegionalPressure",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRegionalPressureTest::RunTest(const FString &Parameters) {
    using Rules = FMythicRegionalPressureRules;

    {
        TestEqual(TEXT("no gap → value holds"), Rules::ValueAtTime(10.0f, 100.0, 100.0, 0.5f), 10.0f);
        TestEqual(TEXT("10s at 0.5/s → -5"), Rules::ValueAtTime(10.0f, 0.0, 10.0, 0.5f), 5.0f);
        TestEqual(TEXT("long gap → floors at 0"), Rules::ValueAtTime(10.0f, 0.0, 1.0e6, 0.5f), 0.0f);
        TestEqual(TEXT("negative gap clamps → holds"), Rules::ValueAtTime(10.0f, 500.0, 100.0, 0.5f), 10.0f);
        TestEqual(TEXT("zero decay → eternal"), Rules::ValueAtTime(10.0f, 0.0, 1.0e9, 0.0f), 10.0f);

        FMythicPressureCellState State;
        State.Value = 20.0f;
        State.LastUpdateTime = 0.0;
        const float Resolved = Rules::Resolve(State, 10.0, 1.0f);
        TestEqual(TEXT("Resolve applies the decay"), Resolved, 10.0f);
        TestEqual(TEXT("Resolve stamps the clock"), State.LastUpdateTime, 10.0);
        TestEqual(TEXT("Resolve writes the state"), State.Value, 10.0f);
    }

    {
        FMythicPressureCellState State;
        Rules::Accumulate(State, 6.0f, 0.0, 0.1f);
        TestEqual(TEXT("first accumulate seeds the value"), State.Value, 6.0f);
        Rules::Accumulate(State, 4.0f, 10.0, 0.1f);
        TestEqual(TEXT("accumulate decays the gap first, then adds"), State.Value, 9.0f);
        Rules::Accumulate(State, -100.0f, 10.0, 0.1f);
        TestEqual(TEXT("negative amounts ignored"), State.Value, 9.0f);
    }

    {
        TestTrue(TEXT("below → at threshold crosses (inclusive)"), Rules::CrossesThreshold(9.9f, 10.0f, 10.0f));
        TestTrue(TEXT("below → past crosses"), Rules::CrossesThreshold(5.0f, 15.0f, 10.0f));
        TestFalse(TEXT("already above → no re-fire"), Rules::CrossesThreshold(11.0f, 15.0f, 10.0f));
        TestFalse(TEXT("still below → no cross"), Rules::CrossesThreshold(1.0f, 9.9f, 10.0f));
        TestFalse(TEXT("threshold 0 → disabled"), Rules::CrossesThreshold(0.0f, 100.0f, 0.0f));
    }

    {
        TMap<FMythicPressureKey, FMythicPressureCellState> Cells;
        const FIntPoint CellA(3, -7);
        const FIntPoint CellB(4, -7);

        Rules::Accumulate(Cells.FindOrAdd(FMythicPressureKey(CellA, TAG_Pressure_Farm)), 12.0f, 0.0, 0.0f);
        Rules::Accumulate(Cells.FindOrAdd(FMythicPressureKey(CellA, TAG_Pressure_Hunt)), 3.0f, 0.0, 0.0f);
        Rules::Accumulate(Cells.FindOrAdd(FMythicPressureKey(CellB, TAG_Pressure_Farm)), 1.0f, 0.0, 0.0f);

        TestEqual(TEXT("three distinct keys"), Cells.Num(), 3);
        TestEqual(TEXT("farm@A untouched by hunt@A"), Cells[FMythicPressureKey(CellA, TAG_Pressure_Farm)].Value, 12.0f);
        TestEqual(TEXT("hunt@A untouched by farm@A"), Cells[FMythicPressureKey(CellA, TAG_Pressure_Hunt)].Value, 3.0f);
        TestEqual(TEXT("farm@B untouched by farm@A"), Cells[FMythicPressureKey(CellB, TAG_Pressure_Farm)].Value, 1.0f);

        TestTrue(TEXT("identical keys equal"), FMythicPressureKey(CellA, TAG_Pressure_Farm) == FMythicPressureKey(CellA, TAG_Pressure_Farm));
        TestTrue(TEXT("identical keys hash equal"),
                 GetTypeHash(FMythicPressureKey(CellA, TAG_Pressure_Farm)) == GetTypeHash(FMythicPressureKey(CellA, TAG_Pressure_Farm)));
        TestFalse(TEXT("channel differs → keys differ"), FMythicPressureKey(CellA, TAG_Pressure_Farm) == FMythicPressureKey(CellA, TAG_Pressure_Hunt));
        TestFalse(TEXT("cell differs → keys differ"), FMythicPressureKey(CellA, TAG_Pressure_Farm) == FMythicPressureKey(CellB, TAG_Pressure_Farm));
    }

    {
        TestEqual(TEXT("no deterrence → base threshold"), Rules::EffectiveRaidThreshold(10.0f, 0.0f, 0.5f), 10.0f);
        TestEqual(TEXT("deterrence 2 @ factor .5 → doubled"), Rules::EffectiveRaidThreshold(10.0f, 2.0f, 0.5f), 20.0f);
        TestTrue(TEXT("threshold monotonic in deterrence"),
                 Rules::EffectiveRaidThreshold(10.0f, 3.0f, 0.5f) > Rules::EffectiveRaidThreshold(10.0f, 1.0f, 0.5f));

        TestEqual(TEXT("at threshold → base pack"), Rules::RaidPackCount(10.0f, 10.0f, 2, 4), 2);
        TestEqual(TEXT("double the threshold → +1"), Rules::RaidPackCount(20.0f, 10.0f, 2, 4), 3);
        TestEqual(TEXT("huge overshoot → capped"), Rules::RaidPackCount(500.0f, 10.0f, 2, 4), 4);
        TestEqual(TEXT("threshold 0 → base"), Rules::RaidPackCount(500.0f, 0.0f, 2, 4), 2);
    }

    {
        TestEqual(TEXT("origin quantizes to (0,0)"), Rules::QuantizeToCell(FVector(0, 0, 0), 1000.0f), FIntPoint(0, 0));
        TestEqual(TEXT("positive floors"), Rules::QuantizeToCell(FVector(1999, 500, 0), 1000.0f), FIntPoint(1, 0));
        TestEqual(TEXT("negative floors (not truncates)"), Rules::QuantizeToCell(FVector(-1, -1001, 0), 1000.0f), FIntPoint(-1, -2));
    }

    return true;
}
