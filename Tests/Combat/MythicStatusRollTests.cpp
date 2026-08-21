
#include "Misc/AutomationTest.h"

#include "GAS/Effects/MythicStatusRegistry.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusRollTest,
    "Mythic.Combat.StatusRoll",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusRollTest::RunTest(const FString &Parameters) {
    using Reg = UMythicStatusRegistry;

    FRollDefinition Range;
    Range.Min = 10.0f;
    Range.Max = 20.0f;

    TestEqual(TEXT("a low roll lands on the floor of the range"), Reg::RollScaledMagnitude(Range, 0, 1.0f, 0.0f), 10.0f);
    TestEqual(TEXT("a high roll lands on the ceiling"), Reg::RollScaledMagnitude(Range, 0, 1.0f, 1.0f), 20.0f);
    TestEqual(TEXT("a mid roll lands between them, so two drops differ"), Reg::RollScaledMagnitude(Range, 0, 1.0f, 0.5f), 15.0f);

    TestEqual(TEXT("the applier's multiplier scales the result"), Reg::RollScaledMagnitude(Range, 0, 2.0f, 0.0f), 20.0f);
    TestEqual(TEXT("a weaker applier inflicts less"), Reg::RollScaledMagnitude(Range, 0, 0.5f, 1.0f), 10.0f);

    Range.LevelScaling = 1.0f;
    TestEqual(TEXT("level scaling lifts the whole band"), Reg::RollScaledMagnitude(Range, 5, 1.0f, 0.0f), 15.0f);
    Range.LevelScaling = 0.0f;

    // An unauthored range must leave the effect's own authored value alone rather than zeroing it.
    FRollDefinition Unauthored;
    TestEqual(TEXT("an unauthored range yields nothing to override with"), Reg::RollScaledMagnitude(Unauthored, 0, 1.0f, 0.5f), 0.0f);

    // Nothing here may produce a negative magnitude: a healing burn is not a status.
    TestTrue(TEXT("a negative multiplier cannot invert the status"), Reg::RollScaledMagnitude(Range, 0, -2.0f, 1.0f) >= 0.0f);
    TestEqual(TEXT("a roll outside 0..1 is clamped, not extrapolated"), Reg::RollScaledMagnitude(Range, 0, 1.0f, 5.0f), 20.0f);

    return true;
}
