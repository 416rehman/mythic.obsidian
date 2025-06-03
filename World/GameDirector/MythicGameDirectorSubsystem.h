// 

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicGameDirectorSubsystem.generated.h"

USTRUCT()
struct FCachedSpawnedActorData {
    GENERATED_BODY()

    // The actor type - soft class reference
    TSoftClassPtr<AActor> ActorType;

    // The actor's transform
    FTransform Transform;
};

/**
 * Stores the runtime data and state of the world and communicates with GamePlayerSubsystem
 */
UCLASS()
class MYTHIC_API UMythicGameDirectorSubsystem : public ULocalPlayerSubsystem {
    GENERATED_BODY()

    /** GameInstanceSubsystem*/
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

public:
    /**
     * State of the world
     */
    // Map of spawned actors in their levels
    TMap<FName, TArray<AActor *>> SpawnedActors;

    // Storage for game director. This map uses the level ID as the key, and an array of CachedLevelActorData as the value
    // When the level is streamed in, the actors in this map will be spawned
    // When the level is streamed out, the spawned actors' data is cached in this map
    TMap<FName, TArray<FCachedSpawnedActorData>> CachedLevelActorData;
};
