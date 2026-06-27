// Mythic Living World — Biome layer
// A biome is a PURE function of cell coordinate + an immutable world seed (deterministic procedural value noise).
// It carries no entities and is never mutated at runtime. The creature spawner consumes it to decide WHAT wildlife
// belongs in a given wilderness cell; the debugger reports it at the player's cell.

#pragma once

#include "CoreMinimal.h"
#include "MythicBiome.generated.h"

// ─────────────────────────────────────────────────────────────
// Biome — climate/terrain classification of a territory cell
// ─────────────────────────────────────────────────────────────

/**
 * The single, authoritative biome enum for the Living World (resolves the prior two divergent definitions).
 * Six biomes: Plains/Forest/Mountain/Wetland/Wasteland/Desert. Wasteland (dry, low) and Desert (dry, high band)
 * are DISTINCT. Drives wildlife species selection per cell.
 */
UENUM(BlueprintType)
enum class EMythicBiome : uint8 {
    Plains = 0 UMETA(DisplayName = "Plains"),
    Forest UMETA(DisplayName = "Forest"),
    Mountain UMETA(DisplayName = "Mountain"),
    Wetland UMETA(DisplayName = "Wetland"),
    Wasteland UMETA(DisplayName = "Wasteland"),
    Desert UMETA(DisplayName = "Desert"),
    COUNT UMETA(Hidden)
};

/** Number of real (non-sentinel) biomes. */
static constexpr int32 MythicBiomeCount = static_cast<int32>(EMythicBiome::COUNT);

// ─────────────────────────────────────────────────────────────
// Biome Thresholds — designer-tunable classification bands
// ─────────────────────────────────────────────────────────────

/**
 * Threshold bands that map two procedural noise channels (Elevation, Moisture) to a biome. All values are in [0,1].
 * Classification order (see UMythicTerritoryGrid::ComputeBiome):
 *   ELEV > MountainElevation                                  -> Mountain
 *   MOIST < WastelandMoisture && ELEV > DesertElevation       -> Desert   (dry + high-but-below-mountain band)
 *   MOIST > WetlandMoisture                                   -> Wetland
 *   MOIST > ForestMoisture                                    -> Forest
 *   MOIST < WastelandMoisture                                 -> Wasteland
 *   else                                                      -> Plains
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicBiomeThresholds {
    GENERATED_BODY()

    /** Elevation above which a cell is Mountain. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MountainElevation = 0.72f;

    /** Moisture above which a cell is Wetland. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WetlandMoisture = 0.70f;

    /** Moisture above which a (non-wetland) cell is Forest. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float ForestMoisture = 0.45f;

    /** Moisture below which a cell is arid (Wasteland, or Desert if also elevated). */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float WastelandMoisture = 0.18f;

    /** Elevation a dry cell must exceed (while below MountainElevation) to be Desert rather than Wasteland. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DesertElevation = 0.55f;
};
