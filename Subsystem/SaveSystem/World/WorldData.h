#pragma once

#include "CoreMinimal.h"
#include "SavedWorldActor.h"
#include "SavedWorldFlags.h"
#include "GameplayTagContainer.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"
#include "World/Harvesting/MythicHarvestSaveTypes.h"
#include "WorldData.generated.h"

USTRUCT(BlueprintType)
struct FSerializedWorldData {
    GENERATED_BODY()

    /** Saveable placed and runtime actors; Blueprint may inspect/author the serialized DTO but does not load it. */
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedWorldActorData> SavedActors;

    /** Opaque living-world subsystem payload serialized through its native archive contract. */
    UPROPERTY(SaveGame)
    TArray<uint8> LivingWorldBlob;

    /** Persisted unlocked points of interest. */
    UPROPERTY(SaveGame)
    TArray<FSerializedPOIUnlock> UnlockedPOIs;

    /** Stable digging-site identifiers already consumed in this world. */
    UPROPERTY(SaveGame)
    TArray<int32> ConsumedDigSiteIds;

    /** Authoritative living-world flags captured for the save slot. */
    UPROPERTY(SaveGame)
    FGameplayTagContainer WorldFlags;

    /** Versioned stable-node lifecycle snapshot; contains no paths, transforms, indices, claims, or partial work. */
    UPROPERTY(SaveGame)
    FMythicHarvestWorldSaveV1 HarvestWorld;

    /** Frozen deterministic harvest grants and completion idempotency keys, including unresolved delivery remainder. */
    UPROPERTY(SaveGame)
    FMythicHarvestRewardOutboxSaveV1 HarvestRewardOutbox;
};
