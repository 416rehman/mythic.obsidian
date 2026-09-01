// Copyright Stellar Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Itemization/Inventory/ViewModels/MythicItemComparisonTypes.h"
#include "Stats/MythicStatTypes.h"
#include "UI/ViewModels/MythicStatDisplay.h"

/** One item-local contribution keyed by canonical StatTag or ItemMetric tag. */
struct FMythicComparableStat {
    FGameplayTag Key;
    FText Label;
    float Value = 0.0f;
    float ContributionIdentity = 0.0f;
    EMythicStatComparisonDirection ComparisonDirection =
        EMythicStatComparisonDirection::HigherIsBetter;
    FMythicStatNumberPresentation NumberPresentation;

    FMythicComparableStat() = default;

    FMythicComparableStat(
        const FGameplayTag InKey,
        const FText &InLabel,
        const float InValue,
        const float InContributionIdentity = 0.0f,
        const EMythicStatComparisonDirection InComparisonDirection =
            EMythicStatComparisonDirection::HigherIsBetter,
        const FMythicStatNumberPresentation &InNumberPresentation =
            FMythicStatNumberPresentation())
        : Key(InKey),
          Label(InLabel),
          Value(InValue),
          ContributionIdentity(InContributionIdentity),
          ComparisonDirection(InComparisonDirection),
          NumberPresentation(InNumberPresentation) {}
};

/** Pure canonical aggregation, formatting, movement, and verdict projection for item-local contributions. */
struct FMythicStatDeltaCore {
    static TArray<FAttributeDiff> ComputeDiffs(
        const TConstArrayView<FMythicComparableStat> NewStats,
        const TConstArrayView<FMythicComparableStat> CurrentStats) {
        struct FFolded {
            FGameplayTag Key;
            FText Label;
            // Sum of each contribution relative to its own operation identity. Keeping the offset
            // independent from encounter order lets conflicting rows disclose honest movement.
            float NetOffset = 0.0f;
            float ContributionIdentity = 0.0f;
            EMythicStatComparisonDirection ComparisonDirection =
                EMythicStatComparisonDirection::Neutral;
            FMythicStatNumberPresentation NumberPresentation;
            int32 ContributionCount = 0;
            bool bSemanticConflict = false;
        };

        const auto Fold = [](
            const TConstArrayView<FMythicComparableStat> Stats,
            TMap<FGameplayTag, FFolded> &OutMap,
            TArray<FGameplayTag> &OutOrder) {
            for (const FMythicComparableStat &Stat : Stats) {
                if (!Stat.Key.IsValid()) {
                    continue;
                }

                if (FFolded *Existing = OutMap.Find(Stat.Key)) {
                    const bool bIdentityConflict = !FMath::IsNearlyEqual(
                        Existing->ContributionIdentity, Stat.ContributionIdentity);
                    const bool bDirectionConflict =
                        Existing->ComparisonDirection != Stat.ComparisonDirection;
                    const bool bFormatConflict =
                        !MythicItemComparison::AreNumberPresentationsEquivalent(
                            Existing->NumberPresentation, Stat.NumberPresentation);
                    Existing->bSemanticConflict |=
                        bIdentityConflict || bDirectionConflict || bFormatConflict
                        || !FMath::IsFinite(Stat.Value)
                        || !FMath::IsFinite(Stat.ContributionIdentity);

                    // Fold each contribution around its own authored identity. This retains the numeric net while
                    // semantic disagreement independently forces the player-facing verdict to neutral.
                    Existing->NetOffset += Stat.Value - Stat.ContributionIdentity;
                    ++Existing->ContributionCount;
                    continue;
                }

                FFolded Folded;
                Folded.Key = Stat.Key;
                Folded.Label = Stat.Label;
                Folded.NetOffset = Stat.Value - Stat.ContributionIdentity;
                Folded.ContributionIdentity = Stat.ContributionIdentity;
                Folded.ComparisonDirection = Stat.ComparisonDirection;
                Folded.NumberPresentation = Stat.NumberPresentation;
                Folded.ContributionCount = 1;
                Folded.bSemanticConflict = !FMath::IsFinite(Stat.Value)
                    || !FMath::IsFinite(Stat.ContributionIdentity);
                OutMap.Add(Stat.Key, MoveTemp(Folded));
                OutOrder.Add(Stat.Key);
            }
        };

        TMap<FGameplayTag, FFolded> NewMap;
        TMap<FGameplayTag, FFolded> CurrentMap;
        TArray<FGameplayTag> NewOrder;
        TArray<FGameplayTag> CurrentOrder;
        Fold(NewStats, NewMap, NewOrder);
        Fold(CurrentStats, CurrentMap, CurrentOrder);

        TArray<FAttributeDiff> Diffs;
        Diffs.Reserve(NewOrder.Num() + CurrentOrder.Num());

        const auto MakeAccessibleSummary = [](
            const FAttributeDiff &Diff) -> FText {
            const FText MovementText =
                Diff.Movement == EMythicStatValueMovement::Increase
                    ? NSLOCTEXT("MythicComparison", "AccessibleIncrease", "numeric increase")
                    : Diff.Movement == EMythicStatValueMovement::Decrease
                        ? NSLOCTEXT("MythicComparison", "AccessibleDecrease", "numeric decrease")
                        : NSLOCTEXT("MythicComparison", "AccessibleEqual", "no visible numeric change");
            const FText VerdictText =
                Diff.Verdict == EMythicComparisonVerdict::Better
                    ? NSLOCTEXT("MythicComparison", "AccessibleBetter", "better")
                    : Diff.Verdict == EMythicComparisonVerdict::Worse
                        ? NSLOCTEXT("MythicComparison", "AccessibleWorse", "worse")
                        : NSLOCTEXT("MythicComparison", "AccessibleNeutral", "neutral");
            const FText DeltaText = Diff.FormattedDelta.IsEmpty()
                ? NSLOCTEXT("MythicComparison", "AccessibleNoDelta", "no displayed change")
                : Diff.FormattedDelta;
            return FText::Format(
                NSLOCTEXT(
                    "MythicComparison", "AccessibleComparisonSummary",
                    "{0}: candidate {1}, equipped {2}, change {3}; {4}, {5}."),
                Diff.AttributeName,
                Diff.FormattedNewValue,
                Diff.FormattedCurrentValue,
                DeltaText,
                MovementText,
                VerdictText);
        };

        const auto MakeDiff = [&Diffs, &MakeAccessibleSummary](
            const FFolded &Preferred,
            const FFolded *NewFolded,
            const FFolded *CurrentFolded) {
            FAttributeDiff &Diff = Diffs.AddDefaulted_GetRef();
            Diff.ComparisonTag = Preferred.Key;
            Diff.AttributeName = Preferred.Label;
            Diff.ContributionIdentity = Preferred.ContributionIdentity;
            Diff.NeutralValue = Preferred.ContributionIdentity;
            Diff.ComparisonDirection = Preferred.ComparisonDirection;
            Diff.NumberPresentation = Preferred.NumberPresentation;
            Diff.bCandidateOnly = NewFolded != nullptr && CurrentFolded == nullptr;
            Diff.bBaselineOnly = NewFolded == nullptr && CurrentFolded != nullptr;
            Diff.bAggregatedFromMultipleContributions =
                (NewFolded && NewFolded->ContributionCount > 1)
                || (CurrentFolded && CurrentFolded->ContributionCount > 1);

            // Rebase both sides onto the preferred row identity. This makes numeric movement depend
            // only on net item contribution, even when bad data mixes identities or encounter order.
            Diff.NewValue = Preferred.ContributionIdentity
                + (NewFolded ? NewFolded->NetOffset : 0.0f);
            Diff.CurrentValue = Preferred.ContributionIdentity
                + (CurrentFolded ? CurrentFolded->NetOffset : 0.0f);
            // Comparisons are presentation data. Quantize both endpoints exactly as FormatValue does before
            // deriving movement and the signed delta, so the card can never claim "1 -> 1, +1" or hide a visible
            // bucket crossing such as "0 -> 1" merely because the raw source delta is small.
            const float DisplayNewValue = MythicStatDisplay::QuantizeValueToDisplayPrecision(
                Diff.NewValue, Diff.NumberPresentation);
            const float DisplayCurrentValue = MythicStatDisplay::QuantizeValueToDisplayPrecision(
                Diff.CurrentValue, Diff.NumberPresentation);
            Diff.Delta = DisplayNewValue - DisplayCurrentValue;

            const bool bCrossSideConflict = NewFolded && CurrentFolded
                && (NewFolded->ComparisonDirection != CurrentFolded->ComparisonDirection
                    || !FMath::IsNearlyEqual(
                        NewFolded->ContributionIdentity,
                        CurrentFolded->ContributionIdentity)
                    || !MythicItemComparison::AreNumberPresentationsEquivalent(
                        NewFolded->NumberPresentation,
                        CurrentFolded->NumberPresentation));
            Diff.bSemanticConflict = Preferred.bSemanticConflict
                || (NewFolded && NewFolded->bSemanticConflict)
                || (CurrentFolded && CurrentFolded->bSemanticConflict)
                || bCrossSideConflict
                || !FMath::IsFinite(Diff.NewValue)
                || !FMath::IsFinite(Diff.CurrentValue)
                || !FMath::IsFinite(Diff.Delta);

            const float Epsilon =
                MythicStatPresentation::GetComparisonEpsilon(Diff.NumberPresentation);
            if (FMath::IsFinite(Diff.Delta) && FMath::Abs(Diff.Delta) > Epsilon) {
                Diff.Movement = Diff.Delta > 0.0f
                    ? EMythicStatValueMovement::Increase
                    : EMythicStatValueMovement::Decrease;

                if (!Diff.bSemanticConflict && Diff.ComparisonDirection ==
                    EMythicStatComparisonDirection::HigherIsBetter) {
                    Diff.Verdict = Diff.Movement == EMythicStatValueMovement::Increase
                        ? EMythicComparisonVerdict::Better
                        : EMythicComparisonVerdict::Worse;
                }
                else if (!Diff.bSemanticConflict && Diff.ComparisonDirection ==
                         EMythicStatComparisonDirection::LowerIsBetter) {
                    Diff.Verdict = Diff.Movement == EMythicStatValueMovement::Decrease
                        ? EMythicComparisonVerdict::Better
                        : EMythicComparisonVerdict::Worse;
                }
            }

            Diff.bIsUpgrade = Diff.Verdict == EMythicComparisonVerdict::Better;
            const FText MissingValue =
                NSLOCTEXT("MythicComparison", "MissingComparisonValue", "\u2014");
            Diff.FormattedCurrentValue = Diff.bCandidateOnly
                ? MissingValue
                : MythicStatDisplay::FormatValue(
                    Diff.CurrentValue, Diff.NumberPresentation);
            Diff.FormattedNewValue = Diff.bBaselineOnly
                ? MissingValue
                : MythicStatDisplay::FormatValue(
                    Diff.NewValue, Diff.NumberPresentation);
            Diff.FormattedDelta = MythicStatDisplay::FormatBonus(
                Diff.Delta, Diff.NumberPresentation);
            Diff.AccessibleSummary = MakeAccessibleSummary(Diff);
        };

        for (const FGameplayTag &Key : NewOrder) {
            const FFolded &NewFolded = NewMap[Key];
            const FFolded *CurrentFolded = CurrentMap.Find(Key);
            MakeDiff(NewFolded, &NewFolded, CurrentFolded);
        }
        for (const FGameplayTag &Key : CurrentOrder) {
            if (NewMap.Contains(Key)) {
                continue;
            }
            const FFolded &CurrentFolded = CurrentMap[Key];
            MakeDiff(CurrentFolded, nullptr, &CurrentFolded);
        }
        return Diffs;
    }
};
