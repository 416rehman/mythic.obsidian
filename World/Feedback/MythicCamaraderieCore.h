#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"

namespace MythicCamaraderie {
inline int32 CountAlliesInRadius(const FVector &SelfLoc, TConstArrayView<FVector> AllyLocs, double RadiusSq, int32 MaxStacks) {
    const int32 Cap = FMath::Max(0, MaxStacks);
    if (Cap == 0 || RadiusSq < 0.0) {
        return 0;
    }
    int32 Count = 0;
    for (const FVector &Loc : AllyLocs) {
        if (FVector::DistSquared(SelfLoc, Loc) <= RadiusSq) {
            if (++Count >= Cap) {
                return Cap;
            }
        }
    }
    return Count;
}

inline float EffectiveBonus(int32 Stacks, float PerAllyBonus) {
    if (Stacks <= 0 || PerAllyBonus <= 0.0f) {
        return 0.0f;
    }
    return static_cast<float>(Stacks) * PerAllyBonus;
}
}
