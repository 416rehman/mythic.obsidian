
#include "Misc/AutomationTest.h"
#include "World/Fishing/MythicWaterQuery.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWaterProximityTest,
    "Mythic.Fishing.Water",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWaterProximityTest::RunTest(const FString &Parameters) {
    using namespace MythicWaterQuery;

    {
        const float Max = 250.0f;

        TestFalse(TEXT("no water hit → false"), IsWithinDrinkRange(false, 10.0f, Max));
        TestFalse(TEXT("no water hit, huge distance → false"), IsWithinDrinkRange(false, 100000.0f, Max));

        TestTrue(TEXT("water hit, close → true"), IsWithinDrinkRange(true, 10.0f, Max));
        TestTrue(TEXT("water hit at the surface (0) → true"), IsWithinDrinkRange(true, 0.0f, Max));
        TestTrue(TEXT("water hit exactly at max → true (boundary allowed)"), IsWithinDrinkRange(true, 250.0f, Max));
        TestFalse(TEXT("water hit just past max → false"), IsWithinDrinkRange(true, 251.0f, Max));

        TestFalse(TEXT("negative distance → false"), IsWithinDrinkRange(true, -1.0f, Max));
    }

    {
        TestTrue(TEXT("feet in water AND dry → apply"), WetnessShouldApply(true, false));
        TestFalse(TEXT("feet in water BUT already wet → no re-apply (idempotent)"), WetnessShouldApply(true, true));
        TestFalse(TEXT("feet NOT in water, dry → no apply"), WetnessShouldApply(false, false));
        TestFalse(TEXT("feet NOT in water, already wet → no apply"), WetnessShouldApply(false, true));
    }

    return true;
}
