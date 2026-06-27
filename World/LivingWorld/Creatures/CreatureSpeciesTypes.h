// Mythic Living World — Creature species data source
// Schema for the wildlife species table + a built-in code-default species set so the creature spawner RUNS unauthored.
// A "species" answers WHAT creature belongs in a given biome (deer in plains, wolves in forest, etc.) and how it
// packs/territorial-defends. Pure data — no behavior lives here.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "CreatureSpeciesTypes.generated.h"

// ─────────────────────────────────────────────────────────────
// Species Row — one wildlife species
// ─────────────────────────────────────────────────────────────

/**
 * One creature species. Rows are keyed (in an authored DataTable) by an arbitrary RowName; the runtime identity is
 * SpeciesId. The spawner buckets rows by Biome and weighted-picks one per wilderness cell by SpawnWeight.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCreatureSpeciesRow : public FTableRowBase {
    GENERATED_BODY()

    /** Stable runtime species identifier written into FMythicCreatureFragment.SpeciesId. Keep unique per species. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    uint8 SpeciesId = 0;

    /** Human-readable species name (debug/UI). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    FName DisplayName;

    /** Biome this species inhabits. The spawner only considers species whose Biome matches the cell's biome. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    EMythicBiome Biome = EMythicBiome::Plains;

    /** Base aggression [0,1] copied into FMythicCreatureFragment.BaseAggression (herbivores low, predators high). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseAggression = 0.0f;

    /** If true, spawns as a pack/herd (PackId allocated, MinPackSize..MaxPackSize members). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    bool bIsPackAnimal = false;

    /** Cells within which this creature gets its territorial aggression boost (copied to FMythicCreatureFragment). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (ClampMin = "0", ClampMax = "16"))
    uint8 DefaultTerritorialRadius = 2;

    /** Relative weight for the per-cell weighted pick among same-biome species (higher = more common). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (ClampMin = "0.0"))
    float SpawnWeight = 1.0f;

    /** Minimum pack/herd size (inclusive). Ignored when bIsPackAnimal is false (treated as 1). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (ClampMin = "1", ClampMax = "32"))
    uint8 MinPackSize = 1;

    /** Maximum pack/herd size (inclusive). Ignored when bIsPackAnimal is false (treated as 1). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (ClampMin = "1", ClampMax = "32"))
    uint8 MaxPackSize = 1;
};

// ─────────────────────────────────────────────────────────────
// Code-default species set
// ─────────────────────────────────────────────────────────────

/**
 * Built-in species set so the creature ecology runs with ZERO authored data. Covers every EMythicBiome with at least
 * one herbivore + one predator (deer/boar/wolf/goat/bear/scavenger/...). The spawner uses these whenever
 * UMythicLivingWorldSettings::CreatureSpeciesTable is unset. SpeciesIds are stable and unique across the set.
 */
namespace MythicCreatureDefaults {
    /** Returns a view over a static, immutable array of default species rows (safe to call from any thread). */
    MYTHIC_API TConstArrayView<FMythicCreatureSpeciesRow> GetCodeDefaultSpecies();
}
