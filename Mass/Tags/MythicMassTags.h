
#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "MythicMassTags.generated.h"

USTRUCT()
struct MYTHIC_API FMythicNPCTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicCreatureTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicEncounterEntityTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicHydratedTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicCognitiveTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicActorSpawnRequestTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicActorDespawnRequestTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicSoldierTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicTravelerTag : public FMassTag {
    GENERATED_BODY()
};

USTRUCT()
struct MYTHIC_API FMythicGroupMemberTag : public FMassTag {
    GENERATED_BODY()
};
