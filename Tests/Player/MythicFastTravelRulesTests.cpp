
#include "Misc/AutomationTest.h"
#include "Containers/Set.h"
#include "Player/FastTravel/MythicFastTravelRules.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicFastTravelRulesTest,
    "Mythic.Player.FastTravelRules",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicFastTravelRulesTest::RunTest(const FString &Parameters) {
    using namespace MythicFastTravel;

    TSet<int32> Unlocked;
    Unlocked.Add(7);
    Unlocked.Add(12);

    TestFalse(TEXT("undiscovered destination → cannot fast travel"), CanFastTravelTo(Unlocked, 99, false));
    TestTrue(TEXT("discovered destination → can fast travel"), CanFastTravelTo(Unlocked, 7, false));
    TestTrue(TEXT("other discovered destination → can fast travel"), CanFastTravelTo(Unlocked, 12, false));
    TestFalse(TEXT("blocked (combat) → cannot fast travel even if discovered"), CanFastTravelTo(Unlocked, 12, true));
    TestFalse(TEXT("INDEX_NONE destination → cannot fast travel"), CanFastTravelTo(Unlocked, INDEX_NONE, false));
    TestFalse(TEXT("empty unlocked set → cannot fast travel"), CanFastTravelTo(TSet<int32>(), 7, false));

    TestTrue(TEXT("both discovered + unblocked → can fast travel between"),
             CanFastTravelBetween(Unlocked, 7, 12, false));
    TestFalse(TEXT("source undiscovered → cannot fast travel between"),
              CanFastTravelBetween(Unlocked, 42, 12, false));
    TestFalse(TEXT("source INDEX_NONE (wilderness) → cannot fast travel between"),
              CanFastTravelBetween(Unlocked, INDEX_NONE, 12, false));
    TestFalse(TEXT("destination undiscovered → cannot fast travel between"),
              CanFastTravelBetween(Unlocked, 7, 99, false));
    TestFalse(TEXT("destination INDEX_NONE → cannot fast travel between"),
              CanFastTravelBetween(Unlocked, 7, INDEX_NONE, false));
    TestFalse(TEXT("blocked (combat) → cannot fast travel between even if both discovered"),
              CanFastTravelBetween(Unlocked, 7, 12, true));

    return true;
}
