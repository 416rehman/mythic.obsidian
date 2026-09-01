// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/ViewModels/MythicItemComparisonTypes.h"

/**
 * Shared, presentation-only policy for compact inline item comparison.
 *
 * Gameplay and itemization own values and verdicts. This policy only decides how those already-resolved facts are
 * encoded in the one ItemDetails card, keeping affix and weapon rows visually identical.
 */
struct MYTHIC_API FMythicItemComparisonPresentation {
    /** True when a diff has a player-visible comparison token. */
    static bool HasVisibleDelta(const FAttributeDiff &Diff);

    /**
     * Returns the single compact token shown beside the candidate value. Shared stats use the canonical signed
     * delta; one-sided stats use NEW or LOST so the candidate value is not printed twice.
     */
    static FText BuildDeltaToken(const FAttributeDiff &Diff);

    /**
     * Returns a directional triangle only for shared numeric changes. NEW and LOST are already self-describing and
     * deliberately receive no second glyph.
     */
    static FText BuildMovementGlyph(const FAttributeDiff &Diff);

    /** Resolves the comparison-only green/red/neutral palette from benefit verdict, including lower-is-better stats. */
    static FLinearColor ResolveOutcomeColor(EMythicComparisonVerdict Verdict);

    /** Resolves one honest color for a multi-channel row; mixed outcomes remain neutral. */
    static FLinearColor ResolveCombinedOutcomeColor(
        TConstArrayView<const FAttributeDiff *> Diffs);
};
