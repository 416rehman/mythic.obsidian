#pragma once

#include "CoreMinimal.h"
#include "SavedWorldActor.h"
#include "SavedDestructible.h"
#include "SavedWorldFlags.h"
#include "GameplayTagContainer.h"
#include "WorldData.generated.h"

USTRUCT(BlueprintType)
struct FSerializedWorldData {
    GENERATED_BODY()


    // All saveable actors (both placed and runtime-spawned)
    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedWorldActorData> SavedActors;

    UPROPERTY(BlueprintReadWrite)
    TArray<FSerializedDestructibleData> DestroyedResources;

    UPROPERTY()
    TArray<uint8> LivingWorldBlob;


    UPROPERTY()
    TArray<FSerializedPOIUnlock> UnlockedPOIs;

    UPROPERTY()
    TArray<int32> ConsumedDigSiteIds;

    UPROPERTY()
    FGameplayTagContainer WorldFlags;
};
