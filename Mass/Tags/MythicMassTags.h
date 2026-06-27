// Mythic Living World — MASS ECS Tag Definitions
// Tags are zero-size markers for fast archetype filtering. No data, just identity.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MythicMassTags.generated.h"

/** Marks an entity as a humanoid NPC (as opposed to a creature) */
USTRUCT()
struct MYTHIC_API FMythicNPCTag : public FMassTag {
    GENERATED_BODY()
};

/** Marks an entity as a creature (wolf, deer, bird, etc.) */
USTRUCT()
struct MYTHIC_API FMythicCreatureTag : public FMassTag {
    GENERATED_BODY()
};

/** Marks an entity spawned + owned by the EncounterDirector (a raid/ambush member). Excludes it from the ambient
 *  PopulationSpawnerProcessor (both its density count and its far-from-player despawn) so the EncounterDirector is the
 *  SOLE lifecycle authority for its entities — otherwise the population despawner silently destroys live encounters. */
USTRUCT()
struct MYTHIC_API FMythicEncounterEntityTag : public FMassTag {
    GENERATED_BODY()
};

/** Marks that this entity has been hydrated with Tier 1+ fragments (Psychodynamic, Personality, Social) */
USTRUCT()
struct MYTHIC_API FMythicHydratedTag : public FMassTag {
    GENERATED_BODY()
};

/** Marks that this entity has been promoted to Tier 2+ and is bound to a cognitive actor */
USTRUCT()
struct MYTHIC_API FMythicCognitiveTag : public FMassTag {
    GENERATED_BODY()
};

/** Marks an entity that requested an actor spawn */
USTRUCT()
struct MYTHIC_API FMythicActorSpawnRequestTag : public FMassTag {
    GENERATED_BODY()
};

/** Marks an embodied entity that requested its cognitive actor be despawned (de-promotion / dehydration) */
USTRUCT()
struct MYTHIC_API FMythicActorDespawnRequestTag : public FMassTag {
    GENERATED_BODY()
};

/**
 * KIND marker for a faction soldier / patrol member spawned over faction-controlled territory.
 * Soldiers ALSO carry FMythicNPCTag (so they embody as the humanoid EmbodiedNPCClass and are counted by the
 * population spawner's per-cell density + far-from-player despawn). This tag is purely for fast kind tallies and
 * archetype filtering (e.g. the debugger's soldier-by-faction count) — it NEVER replaces the humanoid tag.
 */
USTRUCT()
struct MYTHIC_API FMythicSoldierTag : public FMassTag {
    GENERATED_BODY()
};

/**
 * KIND marker for an inter-settlement traveler (caravan/merchant or escorting patrol) moving between towns.
 * Travelers ALSO carry FMythicNPCTag (so they embody as the humanoid class). This tag does double duty:
 *   1. Despawn EXEMPTION — the ambient PopulationSpawnerProcessor excludes FMythicTravelerTag entities from its
 *      far-from-player despawn so a traveler isn't culled mid-journey on the open road.
 *   2. Route-query marker — the traveler route processor selects exactly these entities to advance along their path.
 * It NEVER replaces the humanoid tag.
 */
USTRUCT()
struct MYTHIC_API FMythicTravelerTag : public FMassTag {
    GENERATED_BODY()
};

/**
 * KIND marker for an NPC spawned as part of a group (a noble's retinue, a merchant's barter party, a friend trio).
 * Group members ALSO carry FMythicNPCTag (so they embody as the humanoid EmbodiedNPCClass, are counted by the population
 * spawner's per-cell density, and are culled by its far-from-player despawn — the SINGLE despawn authority) and carry the
 * data-bearing FMythicGroupFragment. This tag is purely for fast kind tallies / archetype filtering (e.g. the debugger's
 * group-member count) — it NEVER replaces the humanoid tag and adds NO embodiment branch.
 */
USTRUCT()
struct MYTHIC_API FMythicGroupMemberTag : public FMassTag {
    GENERATED_BODY()
};
