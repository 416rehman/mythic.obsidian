
#include "Misc/AutomationTest.h"

#include "GAS/Executions/MythicCombatRoll.h"
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


    // Proc chance stacked past certainty. Every input here is rolled, so a build reaches the cap and keeps going.
    {
        using namespace MythicCombat;
        auto Buildup = [](float Base, float Mult, float Overflow) {
            return UMythicDamageApplication::ComputeBuildupPerProc(Base, Mult, Overflow);
        };

        TestEqual(TEXT("no overflow leaves buildup alone"), Buildup(25.0f, 1.0f, 0.0f), 25.0f);
        TestEqual(TEXT("chance is only overflow once past certainty"), ProbabilityOverflow(0.8f), 0.0f);
        TestEqual(TEXT("exactly certain overflows by nothing"), ProbabilityOverflow(1.0f), 0.0f);
        TestEqual(TEXT("half again as much chance is half again as much overflow"), ProbabilityOverflow(1.5f), 0.5f);

        // The point of the change: a chance stacked to 150% now buys something instead of being discarded.
        TestTrue(TEXT("overflow makes the ailment build faster"), Buildup(25.0f, 1.0f, 0.5f) > Buildup(25.0f, 1.0f, 0.0f));
        TestEqual(TEXT("and by the amount overflowed"), Buildup(25.0f, 1.0f, 0.5f), 37.5f);

        // Overflow multiplies the same total the buildup stat scales, so the two compound rather than one winning.
        TestEqual(TEXT("overflow compounds with the buildup stat"), Buildup(25.0f, 2.0f, 1.0f), 100.0f);

        TestEqual(TEXT("a negative overflow cannot drain the meter"), Buildup(25.0f, 1.0f, -3.0f), 25.0f);
        TestTrue(TEXT("no combination produces negative buildup"), Buildup(-5.0f, -5.0f, -5.0f) >= 0.0f);
    }

    return true;
}
