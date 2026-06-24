#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SavedWorldActor.generated.h"

/**
 * Serialized data for any actor implementing IMythicSaveableActor.
 * Works for both runtime-spawned and placed level actors.
 */
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
    // Serialize all saveable actors in the world
    static void SerializeAll(UWorld *World, TArray<FSerializedWorldActorData> &OutActors);

    // Deserialize and restore/spawn all saved actors
    static void DeserializeAll(UWorld *World, const TArray<FSerializedWorldActorData> &InActors);

    // Pure subtractive-reconciliation decision: should a live actor be destroyed as a stale orphan after a load?
    // Only runtime-spawned actors are ever removed (level-placed actors are part of the map). An actor SPAWNED by
    // this very load is the restored state — never destroy it (cross-session its fresh path-name id won't match the
    // saved id, so the id-set check alone would wrongly delete the actor we just restored). Otherwise a pre-existing
    // runtime actor is kept iff the save knows about it. Pure + static for unit testing.
    static bool ShouldDestroyOnReconcile(bool bIsRuntimeSpawned, bool bSpawnedThisLoad, bool bPresentInSave);
};
