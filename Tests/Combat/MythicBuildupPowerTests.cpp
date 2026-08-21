
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
    TestEqual(TEXT("stacked buildup speed applies proportionally more"), Exec::ComputeBuildupPerProc(25.0f, 1.5f), 37.5f);
    TestEqual(TEXT("doubling the multiplier halves the procs needed"), Exec::ComputeBuildupPerProc(25.0f, 2.0f), 50.0f);
    TestEqual(TEXT("the designer can retune the base without touching code"), Exec::ComputeBuildupPerProc(40.0f, 1.0f), 40.0f);

    // A hit must never drain the meter it is filling, or attacking would cure the status.
    TestEqual(TEXT("a negative multiplier cannot drain buildup"), Exec::ComputeBuildupPerProc(25.0f, -3.0f), 0.0f);
    TestEqual(TEXT("a negative base cannot drain buildup"), Exec::ComputeBuildupPerProc(-25.0f, 1.0f), 0.0f);
    TestEqual(TEXT("zeroing the multiplier stops buildup entirely"), Exec::ComputeBuildupPerProc(25.0f, 0.0f), 0.0f);


    // Stacked on-hit chance bends toward certainty instead of clamping at it, so gear keeps paying and the roll
    // never disappears.
    {
        using namespace MythicCombat;
        const float Soft = 0.5f;

        TestEqual(TEXT("a chance below the soft cap is exactly what it says"), DiminishProbability(0.25f, Soft), 0.25f);
        TestEqual(TEXT("a chance at the soft cap is untouched"), DiminishProbability(0.5f, Soft), 0.5f);

        const float AtOne = DiminishProbability(1.0f, Soft);
        TestTrue(TEXT("past the cap a chance still beats the cap"), AtOne > Soft);
        TestTrue(TEXT("but no longer its face value"), AtOne < 1.0f);

        // The property the design turns on: more always gives more, and never gives everything.
        const float AtTwo = DiminishProbability(2.0f, Soft);
        const float AtTen = DiminishProbability(10.0f, Soft);
        TestTrue(TEXT("more chance is always more"), AtTwo > AtOne && AtTen > AtTwo);
        TestTrue(TEXT("certainty is never reached, however much is stacked"), AtTen < 1.0f);
        TestTrue(TEXT("and it does get close"), AtTen > 0.99f);
        TestEqual(TEXT("the ceiling holds at any amount of stacking"),
                  DiminishProbability(1000.0f, Soft), MaxEffectiveProbability);
        // The ceiling exists so a roll still exists: the worst roll must still be able to fail.
        TestFalse(TEXT("even a fully stacked chance can fail"),
                  RollSucceeds(DiminishProbability(1000.0f, Soft), 1.0f));

        // Each further point buys less than the one before it.
        TestTrue(TEXT("returns diminish"), (AtTwo - AtOne) < (AtOne - DiminishProbability(0.5f, Soft)));

        TestEqual(TEXT("a negative chance is nothing"), DiminishProbability(-3.0f, Soft), 0.0f);
        TestEqual(TEXT("a soft cap of zero bends everything from the start"), DiminishProbability(0.0f, 0.0f), 0.0f);
        TestTrue(TEXT("a soft cap of one is a plain clamp"), DiminishProbability(5.0f, 1.0f) <= 1.0f);
    }

    return true;
}
