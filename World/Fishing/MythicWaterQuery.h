#pragma once

#include "CoreMinimal.h"

class UWorld;
struct FHitResult;

namespace MythicWaterQuery {
    MYTHIC_API bool TraceWaterDown(UWorld *World, const FVector &From, float Depth, FHitResult &OutHit);

    MYTHIC_API bool IsPointOverWater(UWorld *World, const FVector &Point, float Depth = 500.0f);

    inline bool IsWithinDrinkRange(bool bHitWater, float Distance, float MaxDist) {
        return bHitWater && Distance >= 0.0f && Distance <= MaxDist;
    }

    inline bool WetnessShouldApply(bool bFeetInWater, bool bAlreadyWet) {
        return bFeetInWater && !bAlreadyWet;
    }
}
