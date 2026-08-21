
#include "Misc/AutomationTest.h"
#include "Objectives/ObjectiveTracker.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicObjectiveRepeatTest,
    "Mythic.Objectives.RepeatCooldown",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicObjectiveRepeatTest::RunTest(const FString &Parameters) {
    using T = UObjectiveTracker;

    TestFalse(TEXT("non-repeatable never repeats (no cooldown)"), T::CanRepeatObjective(false, 0.0f, 999.0f, 0.0f));
    TestFalse(TEXT("non-repeatable never repeats (cooldown elapsed)"), T::CanRepeatObjective(false, 0.0f, 999.0f, 10.0f));

    TestTrue(TEXT("repeatable, zero cooldown → immediate"), T::CanRepeatObjective(true, 100.0f, 100.0f, 0.0f));
    TestTrue(TEXT("repeatable, negative cooldown treated as none"), T::CanRepeatObjective(true, 100.0f, 100.0f, -5.0f));

    TestFalse(TEXT("still on cooldown (30 of 60s)"), T::CanRepeatObjective(true, 0.0f, 30.0f, 60.0f));
    TestTrue(TEXT("cooldown exactly elapsed (60 of 60s)"), T::CanRepeatObjective(true, 0.0f, 60.0f, 60.0f));
    TestTrue(TEXT("cooldown well past (100 of 60s)"), T::CanRepeatObjective(true, 0.0f, 100.0f, 60.0f));
    TestFalse(TEXT("just completed, cooldown not started"), T::CanRepeatObjective(true, 100.0f, 100.0f, 60.0f));

    TestTrue(TEXT("elapsed from stamp: 200 - 100 = 100 >= 60"), T::CanRepeatObjective(true, 100.0f, 200.0f, 60.0f));
    TestFalse(TEXT("elapsed from stamp: 150 - 100 = 50 < 60"), T::CanRepeatObjective(true, 100.0f, 150.0f, 60.0f));

    return true;
}
