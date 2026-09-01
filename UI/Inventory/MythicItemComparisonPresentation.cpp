// Copyright Stellar Games. All Rights Reserved.

#include "UI/Inventory/MythicItemComparisonPresentation.h"

#include "UI/MythicUIStyle.h"

bool FMythicItemComparisonPresentation::HasVisibleDelta(const FAttributeDiff &Diff) {
    return Diff.ComparisonTag.IsValid()
        && Diff.Movement != EMythicStatValueMovement::Equal
        && !Diff.FormattedDelta.IsEmpty();
}

FText FMythicItemComparisonPresentation::BuildDeltaToken(const FAttributeDiff &Diff) {
    if (!HasVisibleDelta(Diff)) {
        return FText::GetEmpty();
    }
    if (Diff.bCandidateOnly) {
        return NSLOCTEXT("MythicItemComparison", "CandidateOnly", "NEW");
    }
    if (Diff.bBaselineOnly) {
        return NSLOCTEXT("MythicItemComparison", "BaselineOnly", "LOST");
    }
    return Diff.FormattedDelta;
}

FText FMythicItemComparisonPresentation::BuildMovementGlyph(const FAttributeDiff &Diff) {
    if (!HasVisibleDelta(Diff) || Diff.bCandidateOnly || Diff.bBaselineOnly) {
        return FText::GetEmpty();
    }
    if (Diff.Movement == EMythicStatValueMovement::Increase) {
        return NSLOCTEXT("MythicItemComparison", "MovementIncrease", "\u25B2");
    }
    if (Diff.Movement == EMythicStatValueMovement::Decrease) {
        return NSLOCTEXT("MythicItemComparison", "MovementDecrease", "\u25BC");
    }
    return FText::GetEmpty();
}

FLinearColor FMythicItemComparisonPresentation::ResolveOutcomeColor(
    const EMythicComparisonVerdict Verdict) {
    const UMythicUIStyleSettings &Style = FMythicUIStyle::Get();
    if (Verdict == EMythicComparisonVerdict::Better) {
        return Style.ComparisonBetter;
    }
    if (Verdict == EMythicComparisonVerdict::Worse) {
        return Style.ComparisonWorse;
    }
    return Style.ComparisonNeutral;
}

FLinearColor FMythicItemComparisonPresentation::ResolveCombinedOutcomeColor(
    const TConstArrayView<const FAttributeDiff *> Diffs) {
    bool bHasBetter = false;
    bool bHasWorse = false;
    for (const FAttributeDiff *Diff : Diffs) {
        bHasBetter |= Diff && Diff->Verdict == EMythicComparisonVerdict::Better;
        bHasWorse |= Diff && Diff->Verdict == EMythicComparisonVerdict::Worse;
    }
    if (bHasBetter != bHasWorse) {
        return ResolveOutcomeColor(
            bHasBetter ? EMythicComparisonVerdict::Better : EMythicComparisonVerdict::Worse);
    }
    return ResolveOutcomeColor(EMythicComparisonVerdict::Neutral);
}
