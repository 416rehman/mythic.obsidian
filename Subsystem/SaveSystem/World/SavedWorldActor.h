#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SavedWorldActor.generated.h"

USTRUCT(BlueprintType)
struct FSerializedWorldActorData {
    GENERATED_BODY()

    // Unique identifier for the actor (path name for placed, generated for runtime)
    UPROPERTY(BlueprintReadWrite)
    FString ActorId;

    // Class to spawn if actor doesn't exist
    UPROPERTY(BlueprintReadWrite)
    FSoftClassPath ActorClass;

    // Transform to spawn at if actor doesn't exist
    UPROPERTY(BlueprintReadWrite)
    FTransform Transform;

    // Serialized actor state (via Actor::Serialize)
    UPROPERTY(BlueprintReadWrite)
    TArray<uint8> ByteData;

    // Custom serialized data for nested UObjects (via SerializeCustomData)
    UPROPERTY(BlueprintReadWrite)
    TArray<uint8> CustomData;

    // True if this was a runtime-spawned actor (hint for load logic)
    UPROPERTY(BlueprintReadWrite)
    bool bWasRuntimeSpawned = false;
};

struct FSerializedWorldActorHelper {
    static void SerializeAll(UWorld *World, TArray<FSerializedWorldActorData> &OutActors);

    static void DeserializeAll(UWorld *World, const TArray<FSerializedWorldActorData> &InActors);

    static bool ShouldDestroyOnReconcile(bool bIsRuntimeSpawned, bool bSpawnedThisLoad, bool bPresentInSave);
};
