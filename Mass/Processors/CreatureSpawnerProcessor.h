// Mythic Living World — Creature Spawner Processor
// MASS processor that populates TRUE wilderness (unowned, non-settlement cells) with biome-appropriate wildlife.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "CreatureSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
struct FMythicCellCoord;

/**
 * MASS processor that maintains a wildlife population in TRUE wilderness around players.
 *
 * "True wilderness" = cells that are (a) NOT inside a settlement, AND (b) have no dominant faction (DominantFaction
 * invalid). Faction-controlled non-settlement cells are owned by the territory-patrol spawner; settlement cells by the
 * population spawner. These three predicates are mutually exclusive, so the three spawners never fight over a cell.
 *
 * Each throttled tick:
 *  1. Gather player cells, resolve the species table (authored DataTable if set, else the code-default species set),
 *     and pre-bucket species rows by biome — all ONCE per Execute (no per-cell loads/allocs).
 *  2. For each candidate cell in the spawn ring: classify its biome (pure, lock-free Grid->GetBiomeAtCell), skip
 *     settlement / faction-owned cells, and weighted-pick a species among that biome's rows (deterministic from the
 *     cell coord). Spawn up to the per-cell creature target deficit.
 *  3. Despawn creatures beyond CreatureDespawnDistance from every player (its own creature-tagged despawn pass — the
 *     population spawner's despawn excludes creatures because they carry FMythicCreatureTag, not FMythicNPCTag).
 *
 * Creatures are created with {Identity, Creature, Significance, FMythicCreatureTag} — Identity carries Significance so
 * the entity flows through the same proximity force-embodiment gate as everything else, and FMythicCreatureTag routes
 * embodiment to the creature actor class. NO Schedule, NO NPCTag. Faction is left INVALID (wildlife has no faction),
 * which the significance leader-candidate path already guards on IsValid().
 *
 * Server/standalone, game thread, budget-capped. ExecuteAfter the territory-patrol spawner.
 */
UCLASS()
class MYTHIC_API UMythicCreatureSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicCreatureSpawnerProcessor();

    /**
     * Target wildlife count for one wilderness biome cell. = round(MaxCreaturesPerBiomeCell × DensityScale), clamped to
     * [0, SystemCap]. Pure + static so the wilderness-density rule is unit-testable and lock-free.
     * @param MaxCreaturesPerBiomeCell Designer base density per biome cell
     * @param DensityScale             Global multiplier (1.0 = base)
     * @param SystemCap                Hard per-cell cap (Settings->MaxEntitiesPerCell) — shared with the NPC spawners
     */
    static int32 ComputeCreatureTargetDensity(int32 MaxCreaturesPerBiomeCell, float DensityScale, int32 SystemCap);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for existing creature entities (per-cell density count + far-from-player despawn). */
    FMassEntityQuery ExistingCreatureQuery;

    /** Timer accumulator — processor runs at configured interval, not every frame. */
    float TimeSinceLastTick = 0.0f;

    /** Monotonic per-Execute counter folded into each creature's NameHash so creatures in the same cell don't alias. */
    uint32 SpawnCounter = 0;

    /** Monotonic pack-id allocator for herd/pack creatures (wraps within uint16; 0 reserved for "solitary"). */
    uint16 NextPackId = 1;

    /** Allocate a non-zero pack id (0 means "solitary" in FMythicCreatureFragment.PackId), wrapping past 0xFFFF -> 1. */
    uint16 AllocatePackId();
};
