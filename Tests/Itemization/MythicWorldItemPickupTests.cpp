
#include "Misc/AutomationTest.h"
#include "Itemization/Loot/MythicWorldItem.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldItemPickupTest,
    "Mythic.Itemization.WorldItemPickup",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldItemPickupTest::RunTest(const FString &Parameters) {
    TestTrue(TEXT("a large stack (e.g. 999 gold / mats) auto-picks up"), AMythicWorldItem::ShouldAutoPickup(999));
    TestTrue(TEXT("a small stack (max 2) auto-picks up"), AMythicWorldItem::ShouldAutoPickup(2));
    TestFalse(TEXT("a unique / non-stacking item (max 1) does NOT auto-pick up — prompted pickup only"), AMythicWorldItem::ShouldAutoPickup(1));
    TestFalse(TEXT("an unset stack size (0) does NOT auto-pick up (defensive: never hoover unknowns)"), AMythicWorldItem::ShouldAutoPickup(0));
    TestFalse(TEXT("a negative/garbage stack size does NOT auto-pick up (defensive)"), AMythicWorldItem::ShouldAutoPickup(-5));

    return true;
}
