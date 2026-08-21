
#include "Misc/AutomationTest.h"

#include "GAS/Executions/MythicDamageApplication.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicBuildupPerProcTest,
    "Mythic.Combat.BuildupPerProc",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicBuildupPerProcTest::RunTest(const FString &Parameters) {
    using Exec = UMythicDamageApplication;

    TestEqual(TEXT("an unmodified attacker applies the authored amount"), Exec::ComputeBuildupPerProc(25.0f, 1.0f), 25.0f);
    TestEqual(TEXT("stacked ailment speed applies proportionally more"), Exec::ComputeBuildupPerProc(25.0f, 1.5f), 37.5f);
    TestEqual(TEXT("doubling the multiplier halves the procs needed"), Exec::ComputeBuildupPerProc(25.0f, 2.0f), 50.0f);
    TestEqual(TEXT("the designer can retune the base without touching code"), Exec::ComputeBuildupPerProc(40.0f, 1.0f), 40.0f);

    // A hit must never drain the meter it is filling, or attacking would cure the ailment.
    TestEqual(TEXT("a negative multiplier cannot drain buildup"), Exec::ComputeBuildupPerProc(25.0f, -3.0f), 0.0f);
    TestEqual(TEXT("a negative base cannot drain buildup"), Exec::ComputeBuildupPerProc(-25.0f, 1.0f), 0.0f);
    TestEqual(TEXT("zeroing the multiplier stops buildup entirely"), Exec::ComputeBuildupPerProc(25.0f, 0.0f), 0.0f);

    return true;
}
