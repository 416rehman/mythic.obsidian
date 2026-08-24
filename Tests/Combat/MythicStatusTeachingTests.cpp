
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

    // Every notice kind must reach a surface. The routing bug that stranded Status hid here: teaching, lifetime and
    // merge were all covered, but nothing asserted a raised notice is actually shown anywhere.
    {
        const EMythicNoticeKind AllKinds[] = {
            EMythicNoticeKind::Loot, EMythicNoticeKind::Objective, EMythicNoticeKind::Progression,
            EMythicNoticeKind::Combat, EMythicNoticeKind::Warning, EMythicNoticeKind::Celebration,
            EMythicNoticeKind::Status,
        };
        for (const EMythicNoticeKind Kind : AllKinds) {
            TestTrue(*FString::Printf(TEXT("notice kind %d reaches a surface"), static_cast<int32>(Kind)),
                     FMythicHudNoticeRules::ReachesSurface(Kind));
        }

        // A status is a sentence to read, so it belongs on the banner, not the transient glance feed.
        TestTrue(TEXT("a status notice is routed to the banner"),
                 FMythicHudNoticeRules::GoesToBanner(EMythicNoticeKind::Status));
        TestFalse(TEXT("a status notice does not go to the glance feed"),
                  FMythicHudNoticeRules::GoesToFeed(EMythicNoticeKind::Status));
    }

    return true;
}
