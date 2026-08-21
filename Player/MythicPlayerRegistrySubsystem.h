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

    void RegisterPlayer(const FString &CanonicalKey, AMythicPlayerState *PlayerState, AMythicPlayerController *PlayerController);

    void UnregisterPlayer(const FString &CanonicalKey);

    void UnregisterObject(UObject *PlayerObject);

    AMythicPlayerState *GetPlayerStateForKey(const FString &CanonicalKey) const;

    AMythicPlayerController *GetPlayerControllerForKey(const FString &CanonicalKey) const;

    APawn *GetPawnForKey(const FString &CanonicalKey) const;

    bool GetKeyForObject(UObject *PlayerObject, FString &OutCanonicalKey) const;

    static AMythicPlayerState* ResolveRegisteredKey(const TMap<FString, TWeakObjectPtr<AMythicPlayerState>>& Map, const FString& Key);

    int32 GetRegisteredCount() const { return RegisteredPlayerStates.Num(); }

private:
    TMap<FString, TWeakObjectPtr<AMythicPlayerState>> RegisteredPlayerStates;

    TMap<FString, TWeakObjectPtr<AMythicPlayerController>> RegisteredPlayerControllers;

    TMap<FString, TWeakObjectPtr<APawn>> RegisteredPawns;

    TMap<FObjectKey, FString> ObjectToKeyMap;
};

