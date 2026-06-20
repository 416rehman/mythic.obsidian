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
