
#include "Misc/AutomationTest.h"

#include "GAS/Effects/MythicStatusRegistry.h"
#include "UI/HUD/MythicHudNotice.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusTeachingTest,
    "Mythic.Combat.StatusTeaching",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusTeachingTest::RunTest(const FString &Parameters) {
    using Reg = UMythicStatusRegistry;

    // The teaching moment is a first meeting, for a player, with something authored to read.
    TestTrue(TEXT("a player meeting a described status for the first time is taught"), Reg::ShouldTeachStatus(true, false, true));
    TestFalse(TEXT("a status already known is not taught again"), Reg::ShouldTeachStatus(true, true, true));
    TestFalse(TEXT("an enemy is never taught"), Reg::ShouldTeachStatus(false, false, true));
    TestFalse(TEXT("a status with no authored description says nothing"), Reg::ShouldTeachStatus(true, false, false));
    TestFalse(TEXT("no combination teaches an enemy"), Reg::ShouldTeachStatus(false, true, true));

    // A status banner is read, not glanced at, so it must outlast every other kind on screen.
    {
        const float StatusLife = FMythicHudNoticeRules::LifetimeFor(EMythicNoticeKind::Status);
        TestTrue(TEXT("a status notice outlasts a loot pop"),
                 StatusLife > FMythicHudNoticeRules::LifetimeFor(EMythicNoticeKind::Loot));
        TestTrue(TEXT("a status notice outlasts a combat flash"),
                 StatusLife > FMythicHudNoticeRules::LifetimeFor(EMythicNoticeKind::Combat));
        TestTrue(TEXT("a status notice outlasts a celebration, being the only one with a sentence to read"),
                 StatusLife > FMythicHudNoticeRules::LifetimeFor(EMythicNoticeKind::Celebration));
    }

    // Two banners for one status must merge rather than stack, whatever the caller does.
    {
        FMythicHudNotice A;
        A.Kind = EMythicNoticeKind::Status;
        A.StackKey = FName(TEXT("Frozen"));
        FMythicHudNotice B = A;
        TestTrue(TEXT("the same status merges instead of stacking"), FMythicHudNoticeRules::CanMerge(A, B));

        FMythicHudNotice Other = A;
        Other.StackKey = FName(TEXT("Burning"));
        TestFalse(TEXT("a different status does not merge into it"), FMythicHudNoticeRules::CanMerge(A, Other));
    }

    return true;
}
