
#include "Misc/AutomationTest.h"

#include "GAS/Executions/MythicDamageApplication.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicChipFloorTest,
    "Mythic.Combat.ChipFloor",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicChipFloorTest::RunTest(const FString &Parameters) {
    auto Chip = [](float Damage, float MinChip) {
        return UMythicDamageApplication::ApplyChipFloor(Damage, MinChip);
    };

    TestEqual(TEXT("armour cannot grind a hit below the floor"), Chip(0.2f, 1.0f), 1.0f);
    TestEqual(TEXT("a hit above the floor is untouched"), Chip(50.0f, 1.0f), 50.0f);
    TestEqual(TEXT("a hit exactly at the floor is untouched"), Chip(1.0f, 1.0f), 1.0f);

    // The floor exists so armour cannot make a target unkillable. It must not resurrect a hit that something
    // else already nullified, or immunity through IncomingDamageMultiplier can never be expressed.
    TestEqual(TEXT("a nullified hit stays nullified"), Chip(0.0f, 1.0f), 0.0f);
    TestEqual(TEXT("a negative hit does not become the floor"), Chip(-5.0f, 1.0f), 0.0f);

    TestEqual(TEXT("no floor configured leaves the hit alone"), Chip(0.2f, 0.0f), 0.2f);

    return true;
}
