// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#include "Itemization/Inventory/ViewModels/MythicTags_ItemMetrics.h"
#include "UI/Inventory/MythicItemComparisonPresentation.h"
#include "UI/MythicUIStyle.h"

namespace {

FAttributeDiff MakeVisibleDiff(
    const FString &Delta,
    const EMythicStatValueMovement Movement,
    const EMythicComparisonVerdict Verdict) {
    FAttributeDiff Diff;
    Diff.ComparisonTag = ITEM_METRIC_WEAPON_DAMAGE_PER_SECOND.GetTag();
    Diff.FormattedDelta = FText::FromString(Delta);
    Diff.Movement = Movement;
    Diff.Verdict = Verdict;
    return Diff;
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCompactItemComparisonPresentationTest,
    "Mythic.UI.ItemComparison.CompactPresentation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCompactItemComparisonPresentationTest::RunTest(const FString &Parameters) {
    FAttributeDiff SharedIncrease = MakeVisibleDiff(
        TEXT("+6.8%"),
        EMythicStatValueMovement::Increase,
        EMythicComparisonVerdict::Better);
    TestEqual(
        TEXT("a shared stat renders only its canonical signed delta"),
        FMythicItemComparisonPresentation::BuildDeltaToken(SharedIncrease).ToString(),
        FString(TEXT("+6.8%")));
    TestEqual(
        TEXT("a shared increase has one compact non-color direction glyph"),
        FMythicItemComparisonPresentation::BuildMovementGlyph(SharedIncrease).ToString(),
        FString(TEXT("\u25B2")));

    FAttributeDiff CandidateOnly = SharedIncrease;
    CandidateOnly.bCandidateOnly = true;
    TestEqual(
        TEXT("a candidate-only affix does not repeat its rolled value as a delta"),
        FMythicItemComparisonPresentation::BuildDeltaToken(CandidateOnly).ToString(),
        FString(TEXT("NEW")));
    TestTrue(
        TEXT("NEW needs no redundant direction glyph"),
        FMythicItemComparisonPresentation::BuildMovementGlyph(CandidateOnly).IsEmpty());

    FAttributeDiff BaselineOnly = MakeVisibleDiff(
        TEXT("-61.3%"),
        EMythicStatValueMovement::Decrease,
        EMythicComparisonVerdict::Worse);
    BaselineOnly.bBaselineOnly = true;
    TestEqual(
        TEXT("a removed equipped affix uses one loss token instead of an equipped-value sentence"),
        FMythicItemComparisonPresentation::BuildDeltaToken(BaselineOnly).ToString(),
        FString(TEXT("LOST")));
    TestTrue(
        TEXT("LOST needs no redundant direction glyph"),
        FMythicItemComparisonPresentation::BuildMovementGlyph(BaselineOnly).IsEmpty());

    FAttributeDiff Equal = SharedIncrease;
    Equal.Movement = EMythicStatValueMovement::Equal;
    Equal.FormattedDelta = FText::GetEmpty();
    TestFalse(
        TEXT("an equal stat emits no comparison chrome"),
        FMythicItemComparisonPresentation::HasVisibleDelta(Equal));
    TestTrue(
        TEXT("an equal stat has no token"),
        FMythicItemComparisonPresentation::BuildDeltaToken(Equal).IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemComparisonColorSemanticsTest,
    "Mythic.UI.ItemComparison.ColorSemantics",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemComparisonColorSemanticsTest::RunTest(const FString &Parameters) {
    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    const FLinearColor Better = FMythicItemComparisonPresentation::ResolveOutcomeColor(
        EMythicComparisonVerdict::Better);
    const FLinearColor Worse = FMythicItemComparisonPresentation::ResolveOutcomeColor(
        EMythicComparisonVerdict::Worse);
    const FLinearColor Neutral = FMythicItemComparisonPresentation::ResolveOutcomeColor(
        EMythicComparisonVerdict::Neutral);

    TestEqual(TEXT("better uses the comparison-specific green"), Better, Style.ComparisonBetter);
    TestEqual(TEXT("worse uses the comparison-specific red"), Worse, Style.ComparisonWorse);
    TestEqual(TEXT("neutral uses the comparison-specific neutral"), Neutral, Style.ComparisonNeutral);
    TestTrue(TEXT("the improvement color is visibly green"), Better.G > Better.R && Better.G > Better.B);
    TestTrue(TEXT("the downgrade color is visibly red"), Worse.R > Worse.G && Worse.R > Worse.B);

    FAttributeDiff LowerIsBetterDecrease = MakeVisibleDiff(
        TEXT("-4.8%"),
        EMythicStatValueMovement::Decrease,
        EMythicComparisonVerdict::Better);
    TestEqual(
        TEXT("a lower-is-better decrease remains green even though its direction glyph points down"),
        FMythicItemComparisonPresentation::ResolveOutcomeColor(LowerIsBetterDecrease.Verdict),
        Style.ComparisonBetter);
    TestEqual(
        TEXT("numeric direction remains independently disclosed"),
        FMythicItemComparisonPresentation::BuildMovementGlyph(LowerIsBetterDecrease).ToString(),
        FString(TEXT("\u25BC")));

    FAttributeDiff BetterDiff = MakeVisibleDiff(
        TEXT("+1"),
        EMythicStatValueMovement::Increase,
        EMythicComparisonVerdict::Better);
    FAttributeDiff WorseDiff = MakeVisibleDiff(
        TEXT("-1"),
        EMythicStatValueMovement::Decrease,
        EMythicComparisonVerdict::Worse);
    const TArray<const FAttributeDiff *> Mixed = {&BetterDiff, &WorseDiff};
    TestEqual(
        TEXT("a mixed multi-channel row cannot lie with a single green or red color"),
        FMythicItemComparisonPresentation::ResolveCombinedOutcomeColor(Mixed),
        Style.ComparisonNeutral);
    return true;
}
