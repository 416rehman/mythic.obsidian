#pragma once

#include "CoreMinimal.h"
#include "UObject/SoftObjectPath.h"
#include "SavedWorldActor.generated.h"

USTRUCT(BlueprintType)
struct FSerializedWorldActorData {
    GENERATED_BODY()

    /** Stable case-sensitive identity: a package path for placed actors or an actor-owned persistent identity at runtime. */
    UPROPERTY(BlueprintReadWrite)
    FString ActorId;

    /** Exact actor class captured by the authority; runtime rows may spawn only this validated saveable class. */
    UPROPERTY(BlueprintReadWrite)
    FSoftClassPath ActorClass;

    /** Finite authoritative transform used when a validated runtime actor must be recreated. */
    UPROPERTY(BlueprintReadWrite)
    FTransform Transform;

    /** Bounded SaveGame archive produced by the actor's native Serialize implementation. */
    UPROPERTY(BlueprintReadWrite)
    TArray<uint8> ByteData;

    /** Bounded actor-owned payload for nested UObject state not represented by the main archive. */
    UPROPERTY(BlueprintReadWrite)
    TArray<uint8> CustomData;

    /** True only for a runtime actor; false denotes one exact placed actor that may not be resident yet. */
    UPROPERTY(BlueprintReadWrite)
    bool bWasRuntimeSpawned = false;
};

/** Authority-only bounded serializer and all-or-nothing preflight for the generic saved-world-actor domain. */
struct FSerializedWorldActorHelper {
    static constexpr int32 AbsoluteMaximumActorRecords = 131072;
    static constexpr int32 AbsoluteMaximumStableIdCharacters = 4096;
    static constexpr int32 AbsoluteMaximumClassPathCharacters = 4096;
    static constexpr int32 AbsoluteMaximumActorArchiveBytes = 16 * 1024 * 1024;
    static constexpr int32 AbsoluteMaximumActorCustomDataBytes = 16 * 1024 * 1024;
    static constexpr int64 AbsoluteMaximumTotalPayloadBytes = 1024ll * 1024ll * 1024ll;

    /**
     * Captures every live saveable actor, enforcing the same hard bounds and identity contract used by restore.
     * CarryForwardPlaced holds placed records whose World Partition cell never became resident this session; each one
     * whose identity was not captured live is re-emitted unchanged, so an unvisited cell cannot lose its saved state.
     */
    static bool SerializeAll(
        UWorld *World,
        const TArray<FSerializedWorldActorData> &CarryForwardPlaced,
        TArray<FSerializedWorldActorData> &OutActors);

    /**
     * Validates the complete actor domain without changing world state. Runtime classes are synchronously resolved and
     * must be concrete, non-transient actors whose default object implements the saveable interface. A placed identity
     * is not required to be resident: World Partition streams its cell in later, so a non-resident placed record is
     * deferred rather than rejected. An identity claimed by two live actors is still ambiguous and rejects.
     */
    static bool PreflightDeserialize(
        UWorld *World,
        const TArray<FSerializedWorldActorData> &InActors,
        FName &OutDiagnosticCode);

    /**
     * Re-runs full preflight, then restores/spawns/reconciles the actor domain. False means preflight rejected before
     * mutation; any impossible archive/spawn/identity failure after mutation begins is fatal to avoid a partial world.
     * OutDeferredPlaced receives every placed record whose actor is not resident yet; the caller must hold it and feed
     * it to both RestoreDeferredPlacedActors and the next SerializeAll.
     */
    static bool DeserializeAll(
        UWorld *World,
        const TArray<FSerializedWorldActorData> &InActors,
        TArray<FSerializedWorldActorData> &OutDeferredPlaced);

    /**
     * Applies any deferred placed record owned by the actors in one newly added level, removing each applied record
     * from InOutDeferred. Scans only that level, so World Partition cell streaming stays O(cell). Returns true when at
     * least one record was applied.
     */
    static bool RestoreDeferredPlacedActors(
        ULevel *Level,
        TArray<FSerializedWorldActorData> &InOutDeferred);

    static bool ShouldDestroyOnReconcile(bool bIsRuntimeSpawned, bool bSpawnedThisLoad, bool bPresentInSave);
};
