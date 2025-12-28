#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SavedWorldActor.h"
#include "SavedDestructible.h"
#include "WorldData.generated.h"

UENUM(BlueprintType)
enum class EMythicWorldSaveVersion : uint8 {
    InitialVersion = 0,
    LatestVersion,
    VersionPlusOne
};

constexpr EMythicWorldSaveVersion CurrentWorldSaveVersion = EMythicWorldSaveVersion::LatestVersion;

/**
 * World Data (Host Only).
 */
USTRUCT(BlueprintType)
struct FSerializedWorldData {
    GENERATED_BODY()

    UPROPERTY(BlueprintReadWrite)
    float TimeOfDay = 0.0f;

    UPROPERTY(BlueprintReadWrite)
    FGameplayTag CurrentWeatherTag;

    // All saveable actors (both placed and runtime-spawned)
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedWorldActorData> SavedActors;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedDestructibleData> DestroyedResources;
};
