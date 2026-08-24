
#include "Misc/AutomationTest.h"
#include "UI/HUD/MythicHudNotice.h"
#include "World/Feedback/MythicRegionTrackerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRegionTrackerTest,
    "Mythic.World.RegionTracker",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRegionTrackerTest::RunTest(const FString &Parameters) {
    using ETier = EMythicDangerTier;

    TestFalse(TEXT("no change → silent"),
              UMythicRegionTrackerComponent::ShouldAnnounce(ETier::Low, 5, ETier::Low, 5));
    TestTrue(TEXT("tier changed → announce"),
             UMythicRegionTrackerComponent::ShouldAnnounce(ETier::High, 5, ETier::Low, 5));
    TestTrue(TEXT("settlement changed at same tier → announce"),
             UMythicRegionTrackerComponent::ShouldAnnounce(ETier::Low, 7, ETier::Low, 5));
    TestTrue(TEXT("first sample (COUNT sentinel) always announces"),
             UMythicRegionTrackerComponent::ShouldAnnounce(ETier::Safe, INDEX_NONE, ETier::COUNT, INDEX_NONE));

    TestTrue(TEXT("Low→High is an increase"),
             UMythicRegionTrackerComponent::IsDangerIncrease(ETier::High, ETier::Low));
    TestFalse(TEXT("High→Low is NOT an increase"),
              UMythicRegionTrackerComponent::IsDangerIncrease(ETier::Low, ETier::High));
    TestFalse(TEXT("same tier is NOT an increase"),
              UMythicRegionTrackerComponent::IsDangerIncrease(ETier::Moderate, ETier::Moderate));
    TestFalse(TEXT("first sample into Extreme fires no cue (COUNT sentinel)"),
              UMythicRegionTrackerComponent::IsDangerIncrease(ETier::Extreme, ETier::COUNT));

    const FText Avalon = FText::FromString(TEXT("City of Avalon"));
    TestEqual(TEXT("in settlement → DisplayName"),
              UMythicRegionTrackerComponent::ResolveRegionName( true, Avalon, EMythicBiome::Forest).ToString(),
              FString(TEXT("City of Avalon")));
    TestEqual(TEXT("wilderness Forest → \"Forest\""),
              UMythicRegionTrackerComponent::ResolveRegionName(false, FText::GetEmpty(), EMythicBiome::Forest).ToString(),
              FString(TEXT("Forest")));
    TestEqual(TEXT("wilderness Mountain → \"Mountains\""),
              UMythicRegionTrackerComponent::ResolveRegionName(false, FText::GetEmpty(), EMythicBiome::Mountain).ToString(),
              FString(TEXT("Mountains")));
    TestEqual(TEXT("in settlement with empty name → biome fallback"),
              UMythicRegionTrackerComponent::ResolveRegionName(true, FText::GetEmpty(), EMythicBiome::Desert).ToString(),
              FString(TEXT("Desert")));
    TestEqual(TEXT("unhandled biome → \"Wilderness\""),
              UMythicRegionTrackerComponent::ResolveRegionName(false, FText::GetEmpty(), EMythicBiome::COUNT).ToString(),
              FString(TEXT("Wilderness")));

    struct FSample { ETier Tier; int32 SettlementId; };
    const FSample Sequence[] = {
        {ETier::Low, 5},
        {ETier::Low, 5},
        {ETier::Low, 5},
        {ETier::High, 7},
        {ETier::High, 7},
        {ETier::High, INDEX_NONE},
        {ETier::High, INDEX_NONE},
        {ETier::Moderate, INDEX_NONE},
    };
    ETier LastTier = ETier::COUNT;
    int32 LastId = INDEX_NONE;
    int32 Announcements = 0;
    int32 Increases = 0;
    for (const FSample &S : Sequence) {
        if (UMythicRegionTrackerComponent::ShouldAnnounce(S.Tier, S.SettlementId, LastTier, LastId)) {
            ++Announcements;
            if (UMythicRegionTrackerComponent::IsDangerIncrease(S.Tier, LastTier)) {
                ++Increases;
            }
            LastTier = S.Tier;
            LastId = S.SettlementId;
        }
    }
    TestEqual(TEXT("gate fires once per genuine change (4 of 8 samples)"), Announcements, 4);
    TestEqual(TEXT("exactly one danger-increase across the run"), Increases, 1);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicRegionNoticeTest,
    "Mythic.World.RegionTracker.Notice",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicRegionNoticeTest::RunTest(const FString &Parameters) {
    const FText Avalon = FText::FromString(TEXT("City of Avalon"));

    const FMythicHudNotice SafeEntry = UMythicRegionTrackerComponent::BuildRegionNotice(Avalon, EMythicDangerTier::Safe);
    TestTrue(TEXT("region entry routes to the banner"), FMythicHudNoticeRules::GoesToBanner(SafeEntry.Kind));
    TestTrue(TEXT("region entry reaches a surface"), FMythicHudNoticeRules::ReachesSurface(SafeEntry.Kind));
    TestEqual(TEXT("title is the region name"), SafeEntry.Text.ToString(), FString(TEXT("City of Avalon")));
    TestTrue(TEXT("Safe tier gets no detail line"), SafeEntry.Detail.IsEmpty());
    TestFalse(TEXT("stack key is set"), SafeEntry.StackKey.IsNone());

    const FMythicHudNotice HighEntry =
        UMythicRegionTrackerComponent::BuildRegionNotice(FText::FromString(TEXT("Wasteland")), EMythicDangerTier::High);
    TestFalse(TEXT("dangerous tier gets a detail line"), HighEntry.Detail.IsEmpty());
    TestTrue(TEXT("detail names the tier"), HighEntry.Detail.ToString().Contains(TEXT("High")));

    TestTrue(TEXT("two rapid entries merge instead of queueing banners"),
             FMythicHudNoticeRules::CanMerge(SafeEntry, HighEntry));

    return true;
}
