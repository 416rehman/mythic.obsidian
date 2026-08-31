
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/Territory/MythicBiome.h"
#include "CreatureSpeciesTypes.generated.h"

class UMythicEntityIdentityDefinition;

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicCreatureSpeciesRow : public FTableRowBase {
    GENERATED_BODY()

    /** Stable runtime species identifier written into FMythicCreatureFragment.SpeciesId. Keep unique per species. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    uint8 SpeciesId = 0;

    /**
     * Canonical Creature.Species.* identity used by codex, presentation, ecology, and authored rules. It is never an
     * array index or localized string, and must be unique across the species table.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature", meta = (Categories = "Creature.Species"))
    FGameplayTag SpeciesTag;

    /** Player-facing localized species name used when the viewer has not learned an individual creature's name. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    FText DisplayName;

    /**
     * Explicit public species/cover identity used by contextual presentation. SpeciesTag remains canonical ecology
     * truth; leaving this empty presents a generic creature until the viewer learns more.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature|Presentation")
    TSoftObjectPtr<UMythicEntityIdentityDefinition> PublicIdentityDefinition;

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

    /**
     * E4 (ecology arm): is this species drawn to CARRION? When corpses are rotting in a wilderness cell, scavengers'
     * spawn weight is amplified there (and the cell's population deficit widens), so a battlefield or a lazy hunter's
     * leavings physically pull predators in. FALSE (the default) leaves a species completely unaffected by corpses,
     * so an unauthored species table behaves byte-identically.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Creature")
    bool bIsScavenger = false;
};


namespace MythicCreatureDefaults {
    MYTHIC_API TConstArrayView<FMythicCreatureSpeciesRow> GetCodeDefaultSpecies();
}
