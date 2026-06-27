// Mythic Living World — Territory Patrol Spawner Processor
// MASS processor that populates faction-controlled NON-settlement cells around players with that faction's soldiers /
// patrols (count scaled by MilitaryStrength × Influence) plus occasional roaming travelers. Settlement cells are owned
// by UMythicPopulationSpawnerProcessor; TRUE wilderness (no dominant faction) is owned by the creature spawner. This
// processor fills the third zone: faction-held open country between towns.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TerritoryPatrolSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;
struct FMythicCellCoord;
enum class EMythicBiome : uint8;

/**
 * MASS processor that maintains a faction-soldier / patrol population over faction-controlled, non-settlement
 * territory near players (RDR2/GTA-style "territory has that faction's soldiers").
 *
 * Each throttled tick:
 * 1. Gather player positions → cell coordinates.
 * 2. For each cell within TerritoryPatrolSpawnRadius of a player:
 *    - skip if NOT faction-controlled (true wilderness → creature spawner) or player-owned;
 *    - skip if the cell belongs to a settlement (→ UMythicPopulationSpawnerProcessor);
 *    - skip if the governing faction is dead / not Active.
 *    Compute SoldierTarget = ComputeTerritoryPatrolDensity(MilitaryStrength, Influence, MaxSoldiersPerControlledCell,
 *    MaxEntitiesPerCell) and spawn up to the deficit. Optionally spawn one roaming traveler (TravelerSpawnChancePerCell).
 * 3. Despawn is intentionally NOT implemented here — soldiers carry FMythicNPCTag, so the population spawner's
 *    far-from-player despawn culls them with the rest of the ambient population (single despawn authority).
 *
 * Soldiers compose {Identity, Schedule, Significance, FMythicNPCTag, FMythicSoldierTag}; travelers compose
 * {Identity, Schedule, Significance, FMythicNPCTag, FMythicTravelerTag}. Both carry Significance+Identity, so the
 * SignificanceProcessor's proximity force-embodiment gate promotes them within EmbodimentRadiusCells of a player and
 * the ActorSpawnProcessor embodies them as the humanoid EmbodiedNPCClass. Total embodied count is bounded by
 * MaxCognitiveActors; per-cell count shares MaxEntitiesPerCell with the ambient population (identical query shape).
 *
 * Runs PrePhysics, Server|Standalone, game thread, at TerritoryPatrolSpawnIntervalSeconds. ExecuteAfter the population
 * spawner so settlement cells are filled first and the per-cell count it reads already reflects this tick's ambient
 * spawns where the radii overlap.
 */
UCLASS()
class MYTHIC_API UMythicTerritoryPatrolSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicTerritoryPatrolSpawnerProcessor();

    /**
     * Target soldier count for one faction-controlled cell. Scales with the governing faction's military strength AND
     * the cell's influence (a firmly-held heartland cell fields more soldiers than a contested frontier one), rounded
     * up, then clamped to the per-cell entity cap. Pure + static so the territory-military-density rule is unit-testable.
     * @param MilitaryStrength          Governing faction's MilitaryStrength [0,1]
     * @param Influence                 Cell's dominant-faction influence [0,1]
     * @param MaxSoldiersPerControlledCell  Designer ceiling on soldiers per cell at full strength × influence
     * @param MaxEntitiesPerCell        Global per-cell entity cap (shared safety valve)
     * @return Soldier target in [0, min(MaxSoldiersPerControlledCell, MaxEntitiesPerCell)]
     */
    static int32 ComputeTerritoryPatrolDensity(float MilitaryStrength, float Influence, int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell);

    /**
     * Garrison-density multiplier for a biome. Faction presence over open country is biome-flavored: fertile Plains and
     * defensible Mountain passes hold full garrisons; arid Wasteland/Desert frontier is sparsely held; Forest/Wetland sit
     * between. Pure + static so the frontier-density rule is unit-testable and deterministic. Returns a value in [0,1]
     * that multiplies the ownership-derived soldier target BEFORE the per-cell cap is re-applied.
     * @param Biome  The cell's biome (from UMythicTerritoryGrid::GetBiomeAtCell).
     * @return Multiplier in [0,1]; defaults to 1.0 for any unhandled value.
     */
    static float BiomeGarrisonModifier(EMythicBiome Biome);

    /**
     * Contested-border garrison boost. A faction-controlled cell that borders a cell held by a faction it is AT WAR with
     * (EMythicFactionRelation::Hostile) is a frontline, so it fields more soldiers than a quiet interior cell. Pure +
     * static so the border-war density rule is unit-testable and deterministic (no RNG — identical to a re-clamp when
     * !bContested). Applied to the (already biome-clamped) soldier target and re-clamped to the same per-cell ceilings
     * the base target respects, so the boost can only THICKEN a garrison UP TO min(MaxSoldiersPerControlledCell,
     * MaxEntitiesPerCell), never past it.
     * @param BaseSoldierTarget             Ownership×influence×biome soldier target for the cell (already clamped).
     * @param bContested                    True if a 4-neighbor cell is held by a Hostile (at-war) faction.
     * @param ContestedMultiplier           Designer multiplier applied when contested (floored at 1.0 internally).
     * @param MaxSoldiersPerControlledCell  Designer ceiling on soldiers per cell.
     * @param MaxEntitiesPerCell            Global per-cell entity cap (shared safety valve).
     * @return Boosted soldier target in [0, min(MaxSoldiersPerControlledCell, MaxEntitiesPerCell)].
     */
    static int32 ApplyContestedBorderBoost(int32 BaseSoldierTarget, bool bContested, float ContestedMultiplier,
                                           int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /**
     * Existing-ambient-NPC count query. SAME SHAPE as UMythicPopulationSpawnerProcessor::ExistingNPCQuery
     * (Identity + Significance + FMythicNPCTag All + FMythicEncounterEntityTag None) so the per-cell density count this
     * processor honors is consistent with the ambient population's — soldiers/travelers it spawns also carry NPCTag,
     * so they self-count toward the shared MaxEntitiesPerCell cap on subsequent ticks. Read-only.
     */
    FMassEntityQuery ExistingNPCQuery;

    /** Timer accumulator — processor runs at TerritoryPatrolSpawnIntervalSeconds, not every frame. */
    float TimeSinceLastTick = 0.0f;
};
