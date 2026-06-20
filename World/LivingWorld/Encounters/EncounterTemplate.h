// Mythic Living World — Encounter Template
// Data-driven encounter definitions for the Encounter Director.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassEntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "EncounterTemplate.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythEncounter, Log, All);

// ─────────────────────────────────────────────────────────────
// Encounter State
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicEncounterState : uint8 {
    /** Waiting for conditions to be met */
    Pending UMETA(DisplayName = "Pending"),

    /** Spawning/setting up encounter entities */
    Spawning UMETA(DisplayName = "Spawning"),

    /** Encounter is active and running */
    Active UMETA(DisplayName = "Active"),

    /** Encounter is winding down / cleanup */
    Completing UMETA(DisplayName = "Completing"),

    /** Encounter finished, awaiting removal */
    Completed UMETA(DisplayName = "Completed")
};

// ─────────────────────────────────────────────────────────────
// Encounter Template — Data-driven definition
// ─────────────────────────────────────────────────────────────

/**
 * Defines the prerequisites and parameters for an encounter type.
 * Stored in data assets. The Encounter Director evaluates templates against
 * current world state to decide which encounters to spawn.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEncounterTemplate {
    GENERATED_BODY()

    /** Unique tag identifying this encounter type */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTag EncounterTag;

    /** Display name for debugging */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FText DisplayName;

    /**
     * World state prerequisites (tag query).
     * Encounter only spawns when the query matches the current world state. The world state is composed of the live
     * Environment.Weather.* , Environment.Time.* , and Environment.Season.* tags (built in
     * EncounterDirector::EvaluateTemplate). An EMPTY query (the default) imposes no constraint.
     * Example: matches Environment.Time.Night AND NOT Environment.Weather.Snow
     *          (a night-only ambush that holds off during snow).
     * NOTE: only the env tags above are currently produced — do NOT author terms that REQUIRE faction/other tags
     * (e.g. an ALL-match on a Faction.* tag) here, as nothing adds them to the container and the query would never match.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    FGameplayTagQuery RequiredWorldState;

    /** Required faction relationship for the encounter (e.g., hostile factions nearby) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    EMythicFactionRelation MinFactionRelation = EMythicFactionRelation::Neutral;

    /** Minimum military strength of the originating faction [0.0, 1.0] */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinMilitaryStrength = 0.0f;

    /** Minimum population of the originating faction */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"))
    int32 MinPopulation = 0;

    /** Cooldown between activations of this encounter type (game time seconds) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float CooldownSeconds = 300.0f;

    /** Max concurrent instances of this encounter type across the world */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "10"))
    int32 MaxConcurrentInstances = 1;

    /** Base probability per evaluation tick [0.0, 1.0]. Gated by prerequisites. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float BaseProbability = 0.1f;

    /** Entity count to spawn for this encounter */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "1", ClampMax = "20"))
    int32 EntityCount = 3;

    /** Max duration before the encounter auto-completes (game time seconds, 0 = infinite) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float MaxDurationSeconds = 600.0f;
};

// ─────────────────────────────────────────────────────────────
// Active Encounter — Runtime instance
// ─────────────────────────────────────────────────────────────

/**
 * A live encounter in the world. Created by the Encounter Director
 * from an FMythicEncounterTemplate when prerequisites are met.
 *
 * ~64 bytes per active encounter (with template tag inline).
 */
struct FMythicActiveEncounter {
    /** Unique ID for this encounter instance */
    uint32 EncounterId = 0;

    /** The template this was spawned from */
    FGameplayTag TemplateTag;

    /** Current state */
    EMythicEncounterState State = EMythicEncounterState::Pending;

    /** Cell where the encounter is centered */
    FMythicCellCoord Cell;

    /** Faction that is driving this encounter (e.g., the patrol faction) */
    FMythicFactionId OriginFaction;

    /** World time when this encounter was activated */
    double ActivationTime = 0.0;

    /** Max duration from template */
    float MaxDurationSeconds = 600.0f;

    /** Number of entities spawned for this encounter */
    int32 EntityCount = 0;

    /** MASS entity handles spawned for this encounter — used for cleanup on completion */
    TArray<FMassEntityHandle> SpawnedEntities;

    /** Whether this encounter has timed out */
    bool HasTimedOut(double CurrentWorldTime) const {
        if (MaxDurationSeconds <= 0.0f) {
            return false;
        }
        return (CurrentWorldTime - ActivationTime) > static_cast<double>(MaxDurationSeconds);
    }
};
