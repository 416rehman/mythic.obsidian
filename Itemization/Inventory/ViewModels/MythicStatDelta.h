#pragma once

#include "CoreMinimal.h"
#include "ItemComparisonVM.h"
#include "Stats/MythicStatTypes.h"

/** One item-local contribution keyed by canonical StatTag or ItemMetric tag. */
struct FMythicComparableStat {
    FGameplayTag Key;
    FText Label;
    float Value = 0.0f;
    float ContributionIdentity = 0.0f;
    EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::HigherIsBetter;
    bool bIsPercentage = false;

    FMythicComparableStat() = default;

    FMythicComparableStat(const FGameplayTag InKey, const FText &InLabel, const float InValue,
                          const float InContributionIdentity = 0.0f,
                          const EMythicStatComparisonDirection InComparisonDirection =
                              EMythicStatComparisonDirection::HigherIsBetter,
                          const bool bInIsPercentage = false)
        : Key(InKey), Label(InLabel), Value(InValue), ContributionIdentity(InContributionIdentity),
          ComparisonDirection(InComparisonDirection), bIsPercentage(bInIsPercentage) {}
};

struct FMythicStatDeltaCore {
    static TArray<FAttributeDiff> ComputeDiffs(TConstArrayView<FMythicComparableStat> NewStats,
                                               TConstArrayView<FMythicComparableStat> CurrentStats) {
        struct FFolded {
            FGameplayTag Key;
            FText Label;
            float Value = 0.0f;
            float ContributionIdentity = 0.0f;
            EMythicStatComparisonDirection ComparisonDirection = EMythicStatComparisonDirection::Neutral;
        };

        auto Fold = [](TConstArrayView<FMythicComparableStat> Stats,
                       TMap<FGameplayTag, FFolded> &OutMap,
                       TArray<FGameplayTag> &OutOrder) {
            for (const FMythicComparableStat &Stat : Stats) {
                if (!Stat.Key.IsValid()) {
                    continue;
                }
                if (FFolded *Existing = OutMap.Find(Stat.Key)) {
                    if (Existing->ComparisonDirection != Stat.ComparisonDirection
                        || !FMath::IsNearlyEqual(Existing->ContributionIdentity, Stat.ContributionIdentity)) {
                        // Contradictory semantics cannot yield an honest green/red verdict.
                        Existing->ComparisonDirection = EMythicStatComparisonDirection::Neutral;
                        continue;
                    }
                    // Fold around the authored neutral. Additive stats use zero; multiplicative stats use one.
                    Existing->Value += Stat.Value - Stat.ContributionIdentity;
                }
                else {
                    FFolded Folded;
                    Folded.Key = Stat.Key;
                    Folded.Label = Stat.Label;
                    Folded.Value = Stat.Value;
                    Folded.ContributionIdentity = Stat.ContributionIdentity;
                    Folded.ComparisonDirection = Stat.ComparisonDirection;
                    OutMap.Add(Stat.Key, Folded);
                    OutOrder.Add(Stat.Key);
                }
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
        auto MakeDiff = [&Diffs](const FFolded &Folded, const float NewValue, const float CurrentValue) {
            FAttributeDiff &Diff = Diffs.AddDefaulted_GetRef();
            Diff.ComparisonTag = Folded.Key;
            Diff.AttributeName = Folded.Label;
            Diff.CurrentValue = CurrentValue;
            Diff.NewValue = NewValue;
            Diff.Delta = NewValue - CurrentValue;
            Diff.ComparisonDirection = Folded.ComparisonDirection;
            Diff.NeutralValue = Folded.ContributionIdentity;
            Diff.bIsUpgrade = Folded.ComparisonDirection == EMythicStatComparisonDirection::HigherIsBetter
                ? Diff.Delta > 0.0f
                : Folded.ComparisonDirection == EMythicStatComparisonDirection::LowerIsBetter
                    ? Diff.Delta < 0.0f
                    : false;
        };
        for (const FGameplayTag &Key : NewOrder) {
            const FFolded &NewFolded = NewMap[Key];
            const FFolded *CurrentFolded = CurrentMap.Find(Key);
            MakeDiff(NewFolded, NewFolded.Value,
                     CurrentFolded ? CurrentFolded->Value : NewFolded.ContributionIdentity);
        }
        for (const FGameplayTag &Key : CurrentOrder) {
            if (NewMap.Contains(Key)) {
                continue;
            }
            const FFolded &CurrentFolded = CurrentMap[Key];
            MakeDiff(CurrentFolded, CurrentFolded.ContributionIdentity, CurrentFolded.Value);
        }
        return Diffs;
    }
};
