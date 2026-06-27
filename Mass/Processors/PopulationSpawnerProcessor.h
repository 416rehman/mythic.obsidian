// Mythic Living World — Population Spawner Processor
// MASS processor that spawns/despawns NPC entities around players based on settlement data.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "PopulationSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicSettlementRegistry;
class UMythicFactionDatabase;
class UMythicRoleDatabase;
struct FMythicCellCoord;
struct FMythicFactionData;
struct FMythicResourceStock;
struct FMythicArchetypeRow;
struct FMythicArchetypeContext;
// Scoped enum forward-decl (fixed uint8 underlying type) — avoids pulling the full Actor-bearing MythicSettlement.h
// into this processor header just for a by-value parameter type. Definition lives in Settlements/MythicSettlement.h.
enum class EMythicSettlementEconomy : uint8;

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

    /**
     * Resolve a settlement's effective economy. If the settlement authored an explicit economy (anything other than
     * Generic), that wins. Otherwise the economy is DERIVED from the governing faction's base production profile — the
     * single dominant resource picks the economic identity (Arms→Military, Food→Farming, Materials→Mining,
     * Wealth→Trade; all-zero → Generic). Pure + static so the derivation is unit-testable and lock-free.
     * @param Authored               The settlement's authored Economy field (Generic = derive)
     * @param FactionBaseProduction  The governing faction's BaseProduction stock (designer-set economic identity)
     */
    static EMythicSettlementEconomy ResolveEconomy(EMythicSettlementEconomy Authored, const FMythicResourceStock& FactionBaseProduction);

    /**
     * Draw a context-aware role tag for an ambient NPC from the archetype catalog — the sim-driven successor to the old
     * hardcoded DeriveRole switch. Each row's effective weight is shaped by the LIVE sim snapshot (faction wealth +
     * military strength, settlement economy, cell biome, time-of-day), then one row is weighted-picked deterministically
     * from the NPC's NameHash. Pure + static + lock-free so the WHO-spawns rule is unit-testable and stable across
     * save/load + the decode-from-seed identity model (same Catalog + Ctx + NameHash → same role forever).
     *
     * Effective weight per row (each factor floored at 0; a 0 in any active factor removes the row from this context):
     *   Eff = max(0, BaseWeight)
     *       × Lerp(1, WealthFavor,    Ctx.WealthNorm)        // rich-favored archetypes
     *       × Lerp(1, WealthDisfavor, 1 - Ctx.WealthNorm)    // poverty-favored archetypes
     *       × Lerp(1, MilitaryFavor,  Ctx.Military)          // militarized factions field soldiers/guards
     *       × (EconomyWeights.IsValidIndex(Eco) ? EconomyWeights[Eco] : 1)
     *       × (BiomeWeights.IsValidIndex(Biome) ? BiomeWeights[Biome] : 1)
     *       × Lerp(NightWeight, DayWeight, Ctx.DayFactor)    // schedule-shaped mix
     * Hard gates (Eff forced to 0): RequiredFactionTags non-empty AND Ctx.FactionTag matches none; OR
     *   Ctx.bWildernessContext AND (bRequiresSettlement OR !bAllowedAlone).
     *
     * Selection: Seed = HashCombine(NameHash, 0x41726368 ('Arch')) & 0xFFFFFF, Roll = Seed/16777216 in [0,1); the
     * winning row is the cumulative-weight bucket Roll×Total lands in. If the catalog is empty or every row gated to 0
     * (Total <= eps), OutChosen is set null and TAG_NPC_ROLE_CIVILIAN is returned (the always-safe fallback).
     *
     * NOTE: this returns the DRAWN role only. The role-DB faction-requirement gate (RequiredFactionTags on
     * UMythicRoleDatabase) is still applied separately by the caller via ApplyFactionGate so the role DB load happens
     * once per Execute, not per NPC — it stays authoritative on top of the draw.
     *
     * @param Catalog    The archetype rows (authored catalog OR code defaults), hoisted once per Execute.
     * @param Ctx        The per-spawn sim context (assembled from already-locked snapshot copies + lock-free reads).
     * @param NameHash   The NPC's globally-unique identity hash (the sole RNG source — no wall-clock reads).
     * @param OutChosen  Set to the winning row (for downstream group/placement metadata), or null on the fallback path.
     */
    static FGameplayTag DeriveArchetype(TConstArrayView<FMythicArchetypeRow> Catalog, const FMythicArchetypeContext& Ctx,
                                        uint32 NameHash, const FMythicArchetypeRow*& OutChosen);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /**
     * Apply the role DB's RequiredFactionTags gate to an already-derived role. If the resolved role definition lists
     * required faction tags and the spawning faction's tag matches NONE of them, the role is demoted to Civilian (a
     * peasant faction shouldn't field Guard Captains). If RoleDB is null OR the role has no requirements OR the role
     * isn't present in the DB, the role passes through unchanged. Static + takes the resolved DB pointer so the
     * caller hoists the (sync) load out of the per-NPC loop.
     * @param RoleDB        Resolved role database (may be null → no gate)
     * @param DerivedRole   The role from DeriveArchetype
     * @param FactionTag    The spawning faction's gameplay tag
     */
    static FGameplayTag ApplyFactionGate(const UMythicRoleDatabase* RoleDB, const FGameplayTag& DerivedRole, const FGameplayTag& FactionTag);

    /** Query for existing NPC entities (to get current cell counts) */
    FMassEntityQuery ExistingNPCQuery;

    /** Timer accumulator — processor runs at configured interval, not every frame */
    float TimeSinceLastTick = 0.0f;
};
