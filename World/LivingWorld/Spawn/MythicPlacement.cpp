#include "World/LivingWorld/Spawn/MythicPlacement.h"

#include "Engine/World.h"                  // UWorld::OverlapBlockingTestByChannel, FCollisionShape, ECC_Pawn (transitive)
#include "NavigationSystem.h"              // UNavigationSystemV1, FNavigationSystem::GetCurrent, FNavLocation, INVALID_NAVEXTENT

namespace MythicPlacement {

bool IsOverWater(UWorld* /*World*/, const FVector& /*FootLocation*/) {
    // v1 stub. Water is expected to be authored as a non-walkable nav area, so STEP 1 (ProjectPointToNavigation) and
    // STEP 2 (reachable/navigable point) already exclude it — a navmesh point is never over open water. Returning false
    // keeps STEP 4 a no-op for now while FREEZING this call site, so it can later become an AWaterBody overlap test
    // (requires the Water module) without touching FindValidSpawn's signature.
    return false;
}

bool FindValidSpawn(UWorld* World, const FMythicPlacementParams& Params, FTransform& OutTransform) {
    if (!World) {
        return false;
    }

    // In-repo idiom (see Itemization/Loot/MythicWorldItem.cpp): FNavigationSystem::GetCurrent<UNavigationSystemV1>.
    UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
    if (!NavSys) {
        // No navigation system => we cannot validate placement. Refuse (defer) rather than air-drop onto Z=0.
        return false;
    }

    // ── STEP 1: project the flat (Z=0) cell center onto walkable navmesh ──────────────────────────────────────────
    // Rejects roofs / walls / furniture / inside-geometry / build-occupied space (with runtime navmesh). The tall Z in
    // NavExtent lets a Z=0 probe find terrain at any height.
    const FVector ProbeExtent = (Params.NavExtent.IsNearlyZero()) ? INVALID_NAVEXTENT : Params.NavExtent;
    FNavLocation Anchor;
    if (!NavSys->ProjectPointToNavigation(Params.CellCenterXY, Anchor, ProbeExtent)) {
        // The cell center isn't near any navmesh (water-only cell, fully built-over, or off-island) => defer.
        return false;
    }

    // ── STEP 2/3/4: roll candidates around the anchor, validate each. ─────────────────────────────────────────────
    const int32 Tries = FMath::Max(1, Params.RetryBudget);
    const float Scatter = FMath::Max(0.0f, Params.ScatterRadius);
    const float CapRadius = FMath::Max(1.0f, Params.CapsuleRadius);
    const float CapHalfHeight = FMath::Max(CapRadius, Params.CapsuleHalfHeight);
    const FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapRadius, CapHalfHeight);

    // Match the in-repo idiom (FName + bTraceComplex ctor) for maximum compile certainty.
    FCollisionQueryParams OverlapParams(FName(TEXT("MythicPlacementOverlap")), /*bTraceComplex=*/false);

    for (int32 Try = 0; Try < Tries; ++Try) {
        FVector Candidate = Anchor.Location;

        // STEP 2: re-roll a scatter point each try EXCEPT the last (and except ScatterRadius==0), which uses the bare
        // projected anchor so sparse/tight cells still place a valid (un-scattered) spawn instead of defer-storming.
        const bool bUseAnchor = (Scatter <= KINDA_SMALL_NUMBER) || (Try == Tries - 1);
        if (!bUseAnchor) {
            FNavLocation Rolled;
            const bool bRolled = Params.bRequireReachability
                ? NavSys->GetRandomReachablePointInRadius(Anchor.Location, Scatter, Rolled)
                : NavSys->GetRandomPointInNavigableRadius(Anchor.Location, Scatter, Rolled);
            if (!bRolled) {
                // Couldn't find a reachable/navigable scatter point this try — fall through to the anchor for this
                // iteration so the overlap/water checks still get a chance (and the last try guarantees the anchor).
                Candidate = Anchor.Location;
            } else {
                Candidate = Rolled.Location;
            }
        }

        // STEP 4: water guard (per-archetype). Cheap; do before the overlap test.
        if (!Params.bWaterCapable && IsOverWater(World, Candidate)) {
            continue;
        }

        // STEP 3: blocking-collision overlap. Test the capsule centered at the candidate's mid-height (foot + half),
        // on the Pawn channel — rejects a position already occupied by a pawn or blocking geometry so actors never
        // spawn overlapping. Player-built structures are blocking collision, so this also rejects build-occupied space.
        const FVector CapsuleCenter = Candidate + FVector(0.0f, 0.0f, CapHalfHeight);
        const bool bBlocked = World->OverlapBlockingTestByChannel(
            CapsuleCenter, FQuat::Identity, ECC_Pawn, Capsule, OverlapParams);
        if (bBlocked) {
            continue;
        }

        // All checks passed. Place the capsule foot at the navmesh candidate; random yaw so a town doesn't face-march.
        const FRotator Yaw(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
        OutTransform = FTransform(Yaw, Candidate);
        return true;
    }

    // Exhausted the retry budget => no valid candidate this attempt. Caller defers (stamps cooldown, re-queues request).
    return false;
}

bool ValidateExistingPoint(UWorld* World, const FVector& FootLocation, float CapsuleRadius, float CapsuleHalfHeight,
                           bool bWaterCapable, FTransform& OutTransform) {
    if (!World) {
        return false;
    }

    const float CapRadius = FMath::Max(1.0f, CapsuleRadius);
    const float CapHalfHeight = FMath::Max(CapRadius, CapsuleHalfHeight);

    // STEP 4: water guard (per-archetype). Cheap; do before the overlap test (mirrors FindValidSpawn ordering).
    if (!bWaterCapable && IsOverWater(World, FootLocation)) {
        return false;
    }

    // STEP 3: blocking-collision overlap at the precomputed point. The anchor was navmesh-valid + reachable at
    // generation time (FindValidSpawn ran offline), so the only thing that can have changed at runtime is occupancy —
    // another pawn or a player-built structure now sitting on it. Re-test exactly that, nothing more.
    const FCollisionShape Capsule = FCollisionShape::MakeCapsule(CapRadius, CapHalfHeight);
    FCollisionQueryParams OverlapParams(FName(TEXT("MythicPlacementRevalidate")), /*bTraceComplex=*/false);
    const FVector CapsuleCenter = FootLocation + FVector(0.0f, 0.0f, CapHalfHeight);
    const bool bBlocked = World->OverlapBlockingTestByChannel(
        CapsuleCenter, FQuat::Identity, ECC_Pawn, Capsule, OverlapParams);
    if (bBlocked) {
        // Occupied — caller falls through to the full FindValidSpawn cell-center path.
        return false;
    }

    const FRotator Yaw(0.0f, FMath::FRandRange(0.0f, 360.0f), 0.0f);
    OutTransform = FTransform(Yaw, FootLocation);
    return true;
}

} // namespace MythicPlacement
