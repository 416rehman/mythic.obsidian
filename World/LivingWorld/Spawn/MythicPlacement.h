#pragma once

#include "CoreMinimal.h"

class UWorld;

/**
 * Embodiment placement / spawn-validity service (embodiment-service-LOCK-v1 §1, §2 STEP 2).
 *
 * PURPOSE: the ActorSpawnProcessor used to SpawnActor at Grid->CellToWorld(Cell), which returns FVector(X, Y, 0.0f) —
 * a flat Z=0 plane, NOT terrain. That air-dropped every embodied NPC onto the world origin plane. This service replaces
 * the raw spawn position with a RIGOROUSLY VALIDATED one: a position is valid only if it is on walkable navmesh,
 * reachable (not an isolated nav island), free of blocking collision, and not in water (unless the archetype is
 * water-capable). Player-built structures become runtime navmesh obstacles, so a runtime/dynamic navmesh makes
 * checks 1–3 reject build-occupied space automatically (project setting: RuntimeGeneration = Dynamic).
 *
 * DIVISION OF CONCERNS (locked): this service answers "WHERE is a valid spawn near this cell?" only. It does NOT
 * decide whether an actor SHOULD pop (the GTA view-gate) — that lives in SignificanceProcessor (spawn decision) and
 * the ActorSpawnProcessor despawn loop (no-vanish-in-front). Keeping placement and the view-gate decoupled prevents the
 * two concerns from cross-contaminating.
 *
 * NO-HITCH: every query here is a cheap point query (one navmesh projection + up to RetryBudget reachable-point rolls +
 * one capsule overlap test). There is NO async path solve and NO blocking sweep. FindValidSpawn validates exactly ONE
 * spawn request; the PER-TICK budget (MaxPlacementValidationsPerTick) and the per-entity defer cooldown are owned by the
 * caller (ActorSpawnProcessor) — this keeps the service a pure, stateless, unit-testable utility.
 *
 * THREAD SAFETY: GAME-THREAD ONLY. UNavigationSystemV1 queries and UWorld::OverlapBlockingTestByChannel are not
 * thread-safe. The only callers (ActorSpawnProcessor) are bRequiresGameThreadExecution=true, so no locks are needed.
 */
struct FMythicPlacementParams {
    /** Cell-center world position (Grid->CellToWorld(Cell)). The Z is the flat Z=0 plane; the service projects it onto
     *  real navmesh via NavExtent — callers supply only XY meaning. */
    FVector CellCenterXY = FVector::ZeroVector;

    /** Scatter radius (cm) for the reachable-point roll inside the cell. 0 => snap to the projected anchor. */
    float ScatterRadius = 250.0f;

    /** Extent for ProjectPointToNavigation of the (Z=0) cell center. Tall Z so a flat probe finds terrain at any height. */
    FVector NavExtent = FVector(250.0f, 250.0f, 100000.0f);

    /** Capsule radius (cm) for the blocking-overlap test, from the resolved actor class CDO's capsule. */
    float CapsuleRadius = 42.0f;

    /** Capsule half-height (cm) for the blocking-overlap test, from the resolved actor class CDO's capsule. */
    float CapsuleHalfHeight = 96.0f;

    /** Water-capable archetype (per-species/per-archetype, NOT a global setting). When false, an over-water candidate
     *  is rejected. Humanoids default false. */
    bool bWaterCapable = false;

    /** STEP 2 toggle: GetRandomReachablePointInRadius (reachable, same nav island) vs GetRandomPointInNavigableRadius. */
    bool bRequireReachability = true;

    /** Candidate attempts before reporting defer. The LAST try falls back to the projected anchor (no scatter) so
     *  sparse cells get a valid spawn instead of defer-storming. */
    int32 RetryBudget = 6;
};

/**
 * Stateless navmesh placement / validity service. Declared here, defined in MythicPlacement.cpp so NavigationSystem.h is
 * not pulled into every translation unit that merely references FMythicPlacementParams.
 */
namespace MythicPlacement {
    /**
     * Find a navmesh-valid, reachable, non-overlapping, water-correct spawn transform near Params.CellCenterXY.
     *
     * Pipeline (each step rejects a class of bad placement the user explicitly requires us to reject):
     *   STEP 1  ProjectPointToNavigation(CellCenterXY, NavExtent) — snaps the flat Z=0 cell center onto walkable navmesh.
     *           Rejects roofs / walls / furniture / inside-geometry / build-occupied space (with runtime navmesh).
     *   STEP 2  For up to RetryBudget tries: roll a candidate via GetRandomReachablePointInRadius (reachable, same nav
     *           island) when bRequireReachability, else GetRandomPointInNavigableRadius. The LAST try uses the projected
     *           anchor itself (no scatter) so sparse/tight cells still place. ScatterRadius==0 also uses the anchor.
     *   STEP 3  OverlapBlockingTestByChannel(ECC_Pawn, capsule) at the candidate foot+halfheight — rejects a position
     *           already occupied by a pawn or blocking geometry (no actor stacking).
     *   STEP 4  IsOverWater(candidate) — rejects water unless bWaterCapable.
     *
     * @return true with OutTransform (location = first valid candidate, rotation = random yaw) if found within
     *         RetryBudget; false => caller should DEFER (it stamps the per-entity cooldown and re-queues the request).
     *
     * COST (caller charges this against MaxPlacementValidationsPerTick): at most
     *   1 ProjectPointToNavigation + RetryBudget * (1 reachable/navigable-point roll + 1 overlap test).
     */
    MYTHIC_API bool FindValidSpawn(UWorld* World, const FMythicPlacementParams& Params, FTransform& OutTransform);

    /**
     * Settlement spawn-point FAST-PATH validator (Step 3). Re-validates an ALREADY-projected foot position (computed
     * offline by AMythicSettlement::GenerateSpawnPoints via FindValidSpawn, so it was navmesh-valid + reachable at
     * generation time) with ONLY the cheap parts of the pipeline that can change at runtime:
     *   - STEP 3 OverlapBlockingTestByChannel(ECC_Pawn, capsule) — rejects a point now occupied by another pawn/body or
     *     a player-built structure (runtime navmesh obstacle),
     *   - STEP 4 IsOverWater (no-op v1, kept for call-site symmetry).
     * It does NOT re-run ProjectPointToNavigation or the reachable-scatter roll — that's the whole point of the
     * precomputed anchor. On success, fills OutTransform (random yaw, like FindValidSpawn) and returns true. On a blocked
     * point returns false so the caller falls through to the full FindValidSpawn cell-center path (never spawns invalid).
     *
     * Lives here (not in the processor) so FCollisionShape / ECC_Pawn stay out of the ActorSpawnProcessor TU, exactly
     * like FindValidSpawn. GAME-THREAD ONLY (OverlapBlockingTestByChannel), same contract as FindValidSpawn.
     *
     * @param FootLocation     The precomputed, already-projected navmesh foot position (Identity.SpawnOverridePos).
     * @param CapsuleRadius    Capsule radius (cm) from the resolved actor class CDO (same source FindValidSpawn uses).
     * @param CapsuleHalfHeight Capsule half-height (cm) from the resolved actor class CDO.
     * @param bWaterCapable    Per-archetype water flag (humanoids false).
     */
    MYTHIC_API bool ValidateExistingPoint(UWorld* World, const FVector& FootLocation, float CapsuleRadius,
                                          float CapsuleHalfHeight, bool bWaterCapable, FTransform& OutTransform);

    /**
     * STEP 4 explicit water guard.
     *
     * v1: returns false (the contract relies on water authored as a non-walkable nav area, so STEP 1/2 already exclude
     * it). Present-but-stubbed so the call site is FROZEN and can be swapped to an AWaterBody overlap test later WITHOUT
     * changing FindValidSpawn's signature or adding the Water module in v1.
     */
    MYTHIC_API bool IsOverWater(UWorld* World, const FVector& FootLocation);
}
