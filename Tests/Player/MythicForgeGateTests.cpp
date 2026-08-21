
#include "Misc/AutomationTest.h"
#include "Player/MythicPlayerController.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicForgeGateTest,
    "Mythic.Itemization.ForgeGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicForgeGateTest::RunTest(const FString &Parameters) {
    using PC = AMythicPlayerController;

    TestTrue(TEXT("standing on the station (DistSq 0) is in range"), PC::IsWithinStationRange(0.0f, 10000.0f));
    TestTrue(TEXT("comfortably inside range"), PC::IsWithinStationRange(2500.0f, 10000.0f));
    TestTrue(TEXT("exactly at the range edge is IN range (<=)"), PC::IsWithinStationRange(10000.0f, 10000.0f));
    TestFalse(TEXT("just past the edge is OUT of range"), PC::IsWithinStationRange(10000.01f, 10000.0f));
    TestFalse(TEXT("far away is out of range"), PC::IsWithinStationRange(1000000.0f, 10000.0f));

    TestFalse(TEXT("zero range (unconfigured station) never passes"), PC::IsWithinStationRange(0.0f, 0.0f));
    TestFalse(TEXT("zero range with any distance never passes"), PC::IsWithinStationRange(50.0f, 0.0f));
    TestFalse(TEXT("negative range (invalid) never passes"), PC::IsWithinStationRange(0.0f, -1.0f));

    const float RangeSq = 40000.0f;
    TestTrue(TEXT("near point in range"), PC::IsWithinStationRange(100.0f, RangeSq));
    TestTrue(TEXT("boundary point in range"), PC::IsWithinStationRange(RangeSq, RangeSq));
    TestFalse(TEXT("beyond boundary out of range"), PC::IsWithinStationRange(RangeSq + 1.0f, RangeSq));

    return true;
}
