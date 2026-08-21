
#include "Misc/AutomationTest.h"
#include "World/Interactables/MythicLocationObjectiveVolume.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLocationObjectiveGateTest,
    "Mythic.World.LocationObjective.EmitGate",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLocationObjectiveGateTest::RunTest(const FString &Parameters) {
    TestTrue(TEXT("authority + resolved ASC + not-yet-fired → emit"),
             AMythicLocationObjectiveVolume::ShouldEmitReachEvent(true, true, false));
    TestFalse(TEXT("no authority → no emit (clients never fire)"),
              AMythicLocationObjectiveVolume::ShouldEmitReachEvent(false, true, false));
    TestFalse(TEXT("no resolved ASC → no emit (non-player overlapper)"),
              AMythicLocationObjectiveVolume::ShouldEmitReachEvent(true, false, false));
    TestFalse(TEXT("already fired for this player → no re-count"),
              AMythicLocationObjectiveVolume::ShouldEmitReachEvent(true, true, true));
    TestFalse(TEXT("no authority AND no ASC → no emit"),
              AMythicLocationObjectiveVolume::ShouldEmitReachEvent(false, false, false));

    return true;
}
