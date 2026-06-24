#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "UObject/ObjectKey.h"
#include "UObject/WeakObjectPtr.h"
#include "MythicPlayerRegistrySubsystem.generated.h"

class AMythicPlayerState;
class AMythicPlayerController;
class APawn;

UCLASS()
class MYTHIC_API UMythicPlayerRegistrySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;

    // register a player state and controller under a canonical key, mapping them bidirectionally in o1 time
    void RegisterPlayer(const FString &CanonicalKey, AMythicPlayerState *PlayerState, AMythicPlayerController *PlayerController);

    // drop a player from the registry by identity to clean up maps in o1 time
    void UnregisterPlayer(const FString &CanonicalKey);

    // unregister player elements using any of the tracked objects in o1 time
    void UnregisterObject(UObject *PlayerObject);

    // resolve a canonical key to the live player state
    AMythicPlayerState *GetPlayerStateForKey(const FString &CanonicalKey) const;

    // resolve a canonical key to the live player controller
    AMythicPlayerController *GetPlayerControllerForKey(const FString &CanonicalKey) const;

    // resolve a canonical key to the active pawn
    APawn *GetPawnForKey(const FString &CanonicalKey) const;

    // resolve an object to its canonical key
    bool GetKeyForObject(UObject *PlayerObject, FString &OutCanonicalKey) const;

    // resolves a key against a map of weak player state pointers, returning nullptr if key is empty, not found, or stale
    static AMythicPlayerState* ResolveRegisteredKey(const TMap<FString, TWeakObjectPtr<AMythicPlayerState>>& Map, const FString& Key);

    // return the number of active registrations in the registry
    int32 GetRegisteredCount() const { return RegisteredPlayerStates.Num(); }

private:
    // canonical key mapping to weak player state pointers
    TMap<FString, TWeakObjectPtr<AMythicPlayerState>> RegisteredPlayerStates;

    // canonical key mapping to weak player controller pointers
    TMap<FString, TWeakObjectPtr<AMythicPlayerController>> RegisteredPlayerControllers;

    // canonical key mapping to weak pawn pointers
    TMap<FString, TWeakObjectPtr<APawn>> RegisteredPawns;

    // reverse lookup map using object keys to prevent linear searches
    TMap<FObjectKey, FString> ObjectToKeyMap;
};

