
#include "Misc/AutomationTest.h"
#include "GameModes/GameState/MythicGameState.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldTierAdvanceTest,
    "Mythic.Progression.WorldTierAdvance",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldTierAdvanceTest::RunTest(const FString &Parameters) {
    using GS = AMythicGameState;

    TestEqual(TEXT("0 → 1 (default tier advances)"), (int32)GS::ComputeAdvancedWorldTier(0, 4), 1);
    TestEqual(TEXT("1 → 2"), (int32)GS::ComputeAdvancedWorldTier(1, 4), 2);
    TestEqual(TEXT("3 → 4 (last step reaches the cap)"), (int32)GS::ComputeAdvancedWorldTier(3, 4), 4);

    TestEqual(TEXT("at max stays at max"), (int32)GS::ComputeAdvancedWorldTier(4, 4), 4);
    TestEqual(TEXT("already over max clamps back to max"), (int32)GS::ComputeAdvancedWorldTier(5, 4), 4);

    TestEqual(TEXT("equal → unchanged"), (int32)GS::ComputeHighestTier(0, 0), 0);
    TestEqual(TEXT("higher new tier raises highest"), (int32)GS::ComputeHighestTier(2, 3), 3);
    TestEqual(TEXT("lower new tier does NOT lower highest"), (int32)GS::ComputeHighestTier(3, 2), 3);
    TestEqual(TEXT("same at cap"), (int32)GS::ComputeHighestTier(4, 4), 4);
    TestEqual(TEXT("advancing from prev-highest keeps monotonic"), (int32)GS::ComputeHighestTier(1, GS::ComputeAdvancedWorldTier(1, 4)), 2);

    return true;
}
