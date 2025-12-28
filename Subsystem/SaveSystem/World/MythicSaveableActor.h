#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "MythicSaveableActor.generated.h"

UINTERFACE(MinimalAPI, Blueprintable)
class UMythicSaveableActor : public UInterface {
    GENERATED_BODY()
};

/**
 * Unified Marker Interface for actors that should have their state saved.
 * 
 * Works for BOTH:
 * - Runtime-spawned actors (dropped items) -> will be respawned on load
 * - Placed level actors (environment controller) -> will have state restored
 * 
 * The save system automatically determines which approach to use based on
 * whether the actor exists in the world when loading.
 * 
 * For simple actors: Just implement interface, mark properties with SaveGame.
 * For actors with nested UObjects: Override SerializeCustomData/DeserializeCustomData.
 */
class MYTHIC_API IMythicSaveableActor {
    GENERATED_BODY()

public:
    // Returns a stable unique identifier for this actor
    virtual FString GetSaveableActorId() const {
        if (const AActor *Actor = Cast<AActor>(this)) {
            return Actor->GetPathName();
        }
        return FString();
    }

    // Override to serialize nested UObjects or complex data
    // Called AFTER the standard Actor::Serialize
    virtual void SerializeCustomData(TArray<uint8> &OutCustomData) {
        // Default: no custom data
    }

    // Override to deserialize nested UObjects or complex data
    // Called AFTER the standard Actor::Serialize
    virtual void DeserializeCustomData(const TArray<uint8> &InCustomData) {
        // Default: nothing to do
    }
};
