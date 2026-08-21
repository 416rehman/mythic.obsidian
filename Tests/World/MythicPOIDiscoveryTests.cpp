#include "Misc/AutomationTest.h"
#include "Containers/Set.h"
#include "World/POI/MythicPOIDiscoveryRules.h"
#include "Player/FastTravel/MythicFastTravelRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPOIDiscoveryTest,
    "Mythic.World.POIDiscovery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPOIDiscoveryTest::RunTest(const FString &Parameters) {
    const float R = 800.0f;
    const float RSq = R * R;
    TestTrue(TEXT("at the anchor → within"), MythicPOIDiscovery::IsWithinDiscoveryRadius(0.0f, RSq));
    TestTrue(TEXT("just inside the radius → within"), MythicPOIDiscovery::IsWithinDiscoveryRadius((R - 1.0f) * (R - 1.0f), RSq));
    TestTrue(TEXT("exactly at the radius → within (inclusive)"), MythicPOIDiscovery::IsWithinDiscoveryRadius(RSq, RSq));
    TestFalse(TEXT("just beyond the radius → not within"), MythicPOIDiscovery::IsWithinDiscoveryRadius((R + 1.0f) * (R + 1.0f), RSq));
    TestFalse(TEXT("zero radius → never within (disabled zone)"), MythicPOIDiscovery::IsWithinDiscoveryRadius(0.0f, 0.0f));

    TestTrue(TEXT("authority + player + not unlocked → register"),
             MythicPOIDiscovery::ShouldRegisterDiscovery( true, true, false));
    TestFalse(TEXT("no authority → never register"),
              MythicPOIDiscovery::ShouldRegisterDiscovery( false, true, false));
    TestFalse(TEXT("non-player overlapper → never register"),
              MythicPOIDiscovery::ShouldRegisterDiscovery( true, false, false));
    TestFalse(TEXT("already unlocked → never re-register (world-shared one-shot)"),
              MythicPOIDiscovery::ShouldRegisterDiscovery( true, true, true));

    TSet<int32> Unlocked;
    const int32 POI = 5;
    const bool bFirst = MythicPOIDiscovery::ShouldRegisterDiscovery(true, true, Unlocked.Contains(POI));
    TestTrue(TEXT("first overlap registers the POI"), bFirst);
    Unlocked.Add(POI);
    const bool bSecond = MythicPOIDiscovery::ShouldRegisterDiscovery(true, true, Unlocked.Contains(POI));
    TestFalse(TEXT("second overlap of an unlocked POI is idempotent (no re-register)"), bSecond);

    TSet<int32> UnlockedPOIs;
    UnlockedPOIs.Add(5);
    UnlockedPOIs.Add(9);
    TestTrue(TEXT("both POIs unlocked → fast-travel between"),
             MythicFastTravel::CanFastTravelBetween(UnlockedPOIs, 5, 9, false));
    TestFalse(TEXT("undiscovered destination POI → cannot fast-travel"),
              MythicFastTravel::CanFastTravelBetween(UnlockedPOIs, 5, 42, false));
    TestFalse(TEXT("wilderness source (INDEX_NONE) → cannot fast-travel"),
              MythicFastTravel::CanFastTravelBetween(UnlockedPOIs, INDEX_NONE, 9, false));
    TestFalse(TEXT("blocked (combat) → cannot fast-travel between unlocked POIs"),
              MythicFastTravel::CanFastTravelBetween(UnlockedPOIs, 5, 9, true));

    return true;
}
