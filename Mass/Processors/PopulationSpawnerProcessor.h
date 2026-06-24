// Mythic Living World — Population Spawner Processor
// MASS processor that spawns/despawns NPC entities around players based on settlement data.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "PopulationSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicSettlementRegistry;
class UMythicFactionDatabase;
struct FMythicCellCoord;

/**
 * MASS processor that maintains NPC population around players.
 *
 * Each tick:
 * 1. Gather player positions → convert to cell coordinates
 * 2. For each cell within spawn radius, compute target density:
 *    TargetDensity = Min(SettlementMaxDensity, SystemCap) × (FactionPop / FactionCapacity)
 * 3. Spawn new MASS entities (with Identity + Schedule + Significance fragments) if under target
 * 4. Despawn entities in cells beyond despawn radius from all players
 *
 * Runs on the game thread at a configurable interval (not every frame).
 * Budget-capped: MaxSpawnsPerTick / MaxDespawnsPerTick prevent frame spikes.
 */
UCLASS()
class MYTHIC_API UMythicPopulationSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicPopulationSpawnerProcessor();

    /**
     * Compute the target NPC count for a settlement cell, scaled by the governing faction's "health".
     * TargetDensity = Min(SettlementMaxDensity, SystemMaxPerCell) × clamp(FactionPopulation / FactionCapacity, 0, 1),
     * rounded up; 0 when the faction has no capacity. Pure + static so the world-population-density rule is unit-testable.
     * @param SettlementMaxDensity Designer-set max density for the settlement
     * @param SystemMaxPerCell     Global system cap
     * @param FactionPopulation    Current simulated population of the governing faction
     * @param FactionCapacity      Max capacity (ControlledCellCount × PopulationPerCell)
     */
    static int32 ComputeTargetDensity(int32 SettlementMaxDensity, int32 SystemMaxPerCell, int32 FactionPopulation, int32 FactionCapacity);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for existing NPC entities (to get current cell counts) */
    FMassEntityQuery ExistingNPCQuery;

    /** Timer accumulator — processor runs at configured interval, not every frame */
    float TimeSinceLastTick = 0.0f;
};
