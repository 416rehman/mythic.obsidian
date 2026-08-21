#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicStatCounterTypes.h"

struct FMythicStatLedger {
    static int64 ClampFloor(int64 Value) { return Value < 0 ? 0 : Value; }

    static int64 FindValue(TConstArrayView<FMythicStatCounter> Counters, const FGameplayTag &Tag) {
        for (const FMythicStatCounter &C : Counters) {
            if (C.Tag == Tag) {
                return C.Value;
            }
        }
        return 0;
    }

    static int64 ApplyDelta(TArray<FMythicStatCounter> &Counters, const FGameplayTag &Tag, int64 Delta) {
        for (FMythicStatCounter &C : Counters) {
            if (C.Tag == Tag) {
                C.Value = ClampFloor(C.Value + Delta);
                return C.Value;
            }
        }
        FMythicStatCounter &New = Counters.AddDefaulted_GetRef();
        New.Tag = Tag;
        New.Value = ClampFloor(Delta);
        return New.Value;
    }

    static int64 ApplyMax(TArray<FMythicStatCounter> &Counters, const FGameplayTag &Tag, int64 Value, bool *bOutNewRecord = nullptr) {
        const int64 Clamped = ClampFloor(Value);
        for (FMythicStatCounter &C : Counters) {
            if (C.Tag == Tag) {
                const bool bRaised = Clamped > C.Value;
                if (bRaised) {
                    C.Value = Clamped;
                }
                if (bOutNewRecord) {
                    *bOutNewRecord = bRaised;
                }
                return C.Value;
            }
        }
        if (Clamped <= 0) {
            if (bOutNewRecord) {
                *bOutNewRecord = false;
            }
            return 0;
        }
        FMythicStatCounter &New = Counters.AddDefaulted_GetRef();
        New.Tag = Tag;
        New.Value = Clamped;
        if (bOutNewRecord) {
            *bOutNewRecord = true;
        }
        return New.Value;
    }

    static int64 SumByPrefix(TConstArrayView<FMythicStatCounter> Counters, const FGameplayTag &Prefix) {
        int64 Sum = 0;
        for (const FMythicStatCounter &C : Counters) {
            if (C.Tag.MatchesTag(Prefix)) {
                Sum += C.Value;
            }
        }
        return Sum;
    }

    static bool DidCrossThreshold(int64 Old, int64 New, int64 Threshold) {
        return Old < Threshold && New >= Threshold;
    }
};
