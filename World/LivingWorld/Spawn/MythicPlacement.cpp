#include "World/LivingWorld/Spawn/MythicPlacement.h"

#include "Engine/World.h"
#include "NavigationSystem.h"

namespace MythicPlacement {
bool IsOverWater(UWorld*, const FVector&) {
    return false;
}

bool FindValidSpawn(UWorld* World, const FMythicPlacementParams& Params, FTransform& OutTransform) {
    if (!World) {
        return false;
    }

    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!NavSys) {
        return false;
    }

    const FVector ProbeExtent = (Params.NavExtent.IsNearlyZero()) ? INVALID_NAVEXTENT : Params.NavExtent;
    FNavLocation Anchor;
    if (!NavSys->ProjectPointToNavigation(Params.CellCenterXY, Anchor, ProbeExtent)) {
        return false;
    }

    const int32 Tries = FMath::Max(1, Params.RetryBudget);
    const float Scatter = FMath::Max(0.0f, Params.ScatterRadius);
    const float CapRadius = FMath::Max(1.0f, Params.CapsuleRadius);
    const float CapHalfHeight = FMath::Max(CapRadius, Params.CapsuleHalfHeight);
    const FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapRadius, CapHalfHeight);

    FCollisionQueryParams OverlapParams(FName(TEXT("MythicPlacementOverlap")),false);

    for (int32 Try = 0; Try < Tries; ++Try) {
        FVector Candidate = Anchor.Location;

        const bool bUseAnchor = (Scatter <= KINDA_SMALL_NUMBER) || (Try == Tries - 1);
        if (!bUseAnchor) {
            FNavLocation Rolled;
            const bool bRolled = Params.bRequireReachability
                ? NavSys->GetRandomReachablePointInRadius(Anchor.Location, Scatter, Rolled)
                : NavSys->GetRandomPointInNavigableRadius(Anchor.Location, Scatter, Rolled);
            if (!bRolled) {
                Candidate = Anchor.Location;
            } else {
                Candidate = Rolled.Location;
            }
        }

        if (!Params.bWaterCapable && IsOverWater(World, Candidate)) {
            continue;
        }

        const FVector CapsuleCenter = Candidate + FVector(0.0f, 0.0f, CapHalfHeight);
        const bool bBlocked = World->OverlapBlockingTestByChannel(
            CapsuleCenter, FQuat::Identity, ECC_Pawn, Capsule, OverlapParams);
        if (bBlocked) {
            continue;
        }

        const FRotator Yaw(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
        OutTransform = FTransform(Yaw, Candidate);
        return true;
    }

    return false;
}

bool ValidateExistingPoint(UWorld* World, const FVector& FootLocation, float CapsuleRadius, float CapsuleHalfHeight,
                           bool bWaterCapable, FTransform& OutTransform) {
    if (!World) {
        return false;
    }

    const float CapRadius = FMath::Max(1.0f, CapsuleRadius);
    const float CapHalfHeight = FMath::Max(CapRadius, CapsuleHalfHeight);

    if (!bWaterCapable && IsOverWater(World, FootLocation)) {
        return false;
    }

    const FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapRadius, CapHalfHeight);
    FCollisionQueryParams OverlapParams(FName(TEXT("MythicPlacementRevalidate")),false);
    const FVector CapsuleCenter = FootLocation + FVector(0.0f, 0.0f, CapHalfHeight);
    const bool bBlocked = World->OverlapBlockingTestByChannel(
        CapsuleCenter, FQuat::Identity, ECC_Pawn, Capsule, OverlapParams);
    if (bBlocked) {
        return false;
    }

    const FRotator Yaw(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
    OutTransform = FTransform(Yaw, FootLocation);
    return true;
}
}
