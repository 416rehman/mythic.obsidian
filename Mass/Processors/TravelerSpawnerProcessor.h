// Mythic Living World — Traveler Spawner Processor
// MASS processor that seeds inter-settlement travelers (merchant caravans / soldier patrols) walking BETWEEN towns.
// Distinct from the territory-patrol spawner (which fills a single faction-controlled cell with stationary soldiers):
// a traveler has an ORIGIN and a DESTINATION settlement and crosses the open road between them, advanced by the
// companion UMythicTravelerRouteProcessor.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/LivingWorldTypes.h" // FMythicCellCoord (StepToward signature)
#include "TravelerSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;

/**
 * MASS processor that spawns inter-settlement travelers around players (RDR2/GTA-style "stagecoaches and patrols move
 * between towns").
 *
 * Each throttled tick (bailing early if the active-traveler cap is already met):
 * 1. Gather player cells.
 * 2. Build the near-player settlement set: every registered settlement whose CenterCell is within
 *    TravelerSpawnPlayerRadiusCells of any player (so travelers only appear where a player can see them depart/arrive).
 * 3. Deterministically pair an ORIGIN settlement with a DESTINATION (seeded by origin/dest ids + the current time
 *    window, so the pairing is stable within a spawn window and reproducible). Caravans prefer a friendly/neutral
 *    other settlement; patrols pick a SAME-governing-faction settlement. Reject pairs farther apart than
 *    MaxRouteCellLength cells.
 * 4. Spawn the traveler entity: {Identity, Schedule, Significance, Traveler, FMythicNPCTag, FMythicTravelerTag}.
 *    Schedule.Phase = Work and WorkCell = the FIRST step toward the destination, so the moment it embodies its
 *    cognitive brain's FollowSchedule desire (0.7) routes the AIController toward CellToWorld(WorkCell).
 *
 * Travelers carry FMythicNPCTag → they embody as the humanoid EmbodiedNPCClass via the existing ActorSpawnProcessor
 * humanoid path (NO ActorSpawnProcessor change needed). They carry FMythicTravelerTag → the population spawner's
 * far-from-player despawn EXEMPTS them (so they survive the open road) and the route processor selects them. Total
 * embodied count is bounded by MaxCognitiveActors; active travelers are bounded by MaxActiveTravelers.
 *
 * Runs PrePhysics, Server|Standalone, game thread, at TravelerSpawnIntervalSeconds. ExecuteAfter the population
 * spawner (purely for a stable, deterministic processor ordering relative to the rest of the population pipeline).
 */
UCLASS()
class MYTHIC_API UMythicTravelerSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicTravelerSpawnerProcessor();

    /**
     * One greedy 8-directional cell step from From toward To (Chebyshev/king move): each axis advances by sign(delta),
     * so a diagonal is a single step. Deterministic + pure (same inputs → same output) so the route is reproducible and
     * unit-testable. Returns From unchanged when already at To.
     */
    static FMythicCellCoord StepToward(FMythicCellCoord From, FMythicCellCoord To);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /**
     * Counts currently-active travelers for the MaxActiveTravelers cap. Matches FMythicTravelerTag (All). Registered so
     * the framework caches it; counted via ForEachEntityChunk accumulation (no per-frame manager-bound query alloc).
     */
    FMassEntityQuery ActiveTravelerQuery;

    /** Timer accumulator — processor runs at TravelerSpawnIntervalSeconds, not every frame. */
    float TimeSinceLastTick = 0.0f;
};
