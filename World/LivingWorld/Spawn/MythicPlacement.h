#pragma once

#include "CoreMinimal.h"

class UWorld;

struct FMythicPlacementParams {
    FVector CellCenterXY = FVector::ZeroVector;

    float ScatterRadius = 250.0f;

    FVector NavExtent = FVector(250.0f, 250.0f, 100000.0f);

    float CapsuleRadius = 42.0f;

    float CapsuleHalfHeight = 96.0f;

    bool bWaterCapable = false;

    bool bRequireReachability = true;

    int32 RetryBudget = 6;
};

namespace MythicPlacement {
    MYTHIC_API bool FindValidSpawn(UWorld* World, const FMythicPlacementParams& Params, FTransform& OutTransform);

    MYTHIC_API bool ValidateExistingPoint(UWorld* World, const FVector& FootLocation, float CapsuleRadius,
                                          float CapsuleHalfHeight, bool bWaterCapable, FTransform& OutTransform);

    MYTHIC_API bool IsOverWater(UWorld* World, const FVector& FootLocation);
}
