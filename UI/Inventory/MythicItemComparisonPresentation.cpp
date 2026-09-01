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
    return Diff.FormattedDelta;
}

FText FMythicItemComparisonPresentation::BuildMovementGlyph(const FAttributeDiff &Diff) {
    (void)Diff;
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
