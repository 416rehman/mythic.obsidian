#pragma once

#include "CoreMinimal.h"
#include "SavedWorldActor.h"
#include "SavedDestructible.h"
#include "WorldData.generated.h"

/**
 * World Data (Host Only).
 */
USTRUCT(BlueprintType)
struct FSerializedWorldData {
    GENERATED_BODY()

    // NOTE: time-of-day + weather are NOT stored here — they persist via AMythicEnvironmentController's own SaveGame
    // fields (the SavedActors path). The former TimeOfDay (saved-but-never-loaded) + CurrentWeatherTag (wholly unused)
    // dead fields were removed, along with the unversioned-and-unused EMythicWorldSaveVersion enum above.

    // All saveable actors (both placed and runtime-spawned)
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedWorldActorData> SavedActors;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedDestructibleData> DestroyedResources;

    // Living World system state (serialized blob from all LW subsystems)
    UPROPERTY()
    TArray<uint8> LivingWorldBlob;
};
