
#pragma once

#include "CoreMinimal.h"
#include "ItemComparisonVM.h"

struct FMythicComparableStat {
    FName Key;
    FText Label;
    float Value = 0.0f;
    bool bLowerIsBetter = false;
    bool bIsPercentage = false;

    FMythicComparableStat() = default;

    FMythicComparableStat(const FName InKey, const FText &InLabel, const float InValue,
                          const bool bInLowerIsBetter = false, const bool bInIsPercentage = false)
        : Key(InKey), Label(InLabel), Value(InValue), bLowerIsBetter(bInLowerIsBetter), bIsPercentage(bInIsPercentage) {}
};

struct FMythicStatDeltaCore {
    static TArray<FAttributeDiff> ComputeDiffs(TConstArrayView<FMythicComparableStat> NewStats,
                                               TConstArrayView<FMythicComparableStat> CurrentStats) {
        struct FFolded {
            FText Label;
            float Value = 0.0f;
            bool bLowerIsBetter = false;
        };
        auto Fold = [](TConstArrayView<FMythicComparableStat> Stats, TMap<FName, FFolded> &OutMap, TArray<FName> &OutOrder) {
            for (const FMythicComparableStat &Stat : Stats) {
                if (FFolded *Existing = OutMap.Find(Stat.Key)) {
                    Existing->Value += Stat.Value;
                }
                else {
                    FFolded Folded;
                    Folded.Label = Stat.Label;
                    Folded.Value = Stat.Value;
                    Folded.bLowerIsBetter = Stat.bLowerIsBetter;
                    OutMap.Add(Stat.Key, Folded);
                    OutOrder.Add(Stat.Key);
                }
            }
        };
        TMap<FName, FFolded> NewMap, CurrentMap;
        TArray<FName> NewOrder, CurrentOrder;
        Fold(NewStats, NewMap, NewOrder);
        Fold(CurrentStats, CurrentMap, CurrentOrder);

        TArray<FAttributeDiff> Diffs;
        Diffs.Reserve(NewOrder.Num() + CurrentOrder.Num());
        auto MakeDiff = [&Diffs](const FFolded &Folded, const float NewValue, const float CurrentValue) {
            FAttributeDiff &Diff = Diffs.AddDefaulted_GetRef();
            Diff.AttributeName = Folded.Label;
            Diff.CurrentValue = CurrentValue;
            Diff.NewValue = NewValue;
            Diff.Delta = NewValue - CurrentValue;
            Diff.bIsUpgrade = Folded.bLowerIsBetter ? (Diff.Delta < 0.0f) : (Diff.Delta > 0.0f);
        };
        for (const FName &Key : NewOrder) {
            const FFolded &NewFolded = NewMap[Key];
            const FFolded *CurrentFolded = CurrentMap.Find(Key);
            MakeDiff(NewFolded, NewFolded.Value, CurrentFolded ? CurrentFolded->Value : 0.0f);
        }
        for (const FName &Key : CurrentOrder) {
            if (NewMap.Contains(Key)) {
                continue;
            }
            const FFolded &CurrentFolded = CurrentMap[Key];
            MakeDiff(CurrentFolded, 0.0f, CurrentFolded.Value);
        }
        return Diffs;
    }

    static int32 ComputeUpgradeScore(const TArray<FAttributeDiff> &Diffs) {
        int32 Score = 0;
        for (const FAttributeDiff &Diff : Diffs) {
            if (FMath::IsNearlyZero(Diff.Delta)) {
                continue;
            }
            Score += Diff.bIsUpgrade ? 1 : -1;
        }
        return Score;
    }
};
