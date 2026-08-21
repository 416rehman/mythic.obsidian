#include "MythicPlayerRegistrySubsystem.h"
#include "MythicPlayerState.h"
#include "MythicPlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

bool UMythicPlayerRegistrySubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    if (const UWorld *World = Cast<UWorld>(Outer)) {
        return World->IsGameWorld();
    }
    return false;
}

void UMythicPlayerRegistrySubsystem::RegisterPlayer(const FString &CanonicalKey, AMythicPlayerState *PlayerState, AMythicPlayerController *PlayerController) {
    if (CanonicalKey.IsEmpty()) {
        return;
    }

    UnregisterPlayer(CanonicalKey);

    if (PlayerState) {
        UnregisterObject(PlayerState);
    }
    if (PlayerController) {
        UnregisterObject(PlayerController);
    }
    APawn *Pawn = PlayerState ? PlayerState->GetPawn() : nullptr;
    if (Pawn) {
        UnregisterObject(Pawn);
    }

    if (PlayerState) {
        RegisteredPlayerStates.Add(CanonicalKey, PlayerState);
        ObjectToKeyMap.Add(FObjectKey(PlayerState), CanonicalKey);
    }
    if (PlayerController) {
        RegisteredPlayerControllers.Add(CanonicalKey, PlayerController);
        ObjectToKeyMap.Add(FObjectKey(PlayerController), CanonicalKey);
    }
    if (Pawn) {
        RegisteredPawns.Add(CanonicalKey, Pawn);
        ObjectToKeyMap.Add(FObjectKey(Pawn), CanonicalKey);
    }
}

void UMythicPlayerRegistrySubsystem::UnregisterPlayer(const FString &CanonicalKey) {
    if (CanonicalKey.IsEmpty()) {
        return;
    }

    if (const TWeakObjectPtr<AMythicPlayerState> *FoundPS = RegisteredPlayerStates.Find(CanonicalKey)) {
        if (AMythicPlayerState *PS = FoundPS->Get()) {
            ObjectToKeyMap.Remove(FObjectKey(PS));
        }
    }
    if (const TWeakObjectPtr<AMythicPlayerController> *FoundPC = RegisteredPlayerControllers.Find(CanonicalKey)) {
        if (AMythicPlayerController *PC = FoundPC->Get()) {
            ObjectToKeyMap.Remove(FObjectKey(PC));
        }
    }
    if (const TWeakObjectPtr<APawn> *FoundPawn = RegisteredPawns.Find(CanonicalKey)) {
        if (APawn *Pawn = FoundPawn->Get()) {
            ObjectToKeyMap.Remove(FObjectKey(Pawn));
        }
    }

    RegisteredPlayerStates.Remove(CanonicalKey);
    RegisteredPlayerControllers.Remove(CanonicalKey);
    RegisteredPawns.Remove(CanonicalKey);
}

void UMythicPlayerRegistrySubsystem::UnregisterObject(UObject *PlayerObject) {
    if (!PlayerObject) {
        return;
    }

    FString Key;
    if (GetKeyForObject(PlayerObject, Key)) {
        UnregisterPlayer(Key);
    }
}

AMythicPlayerState *UMythicPlayerRegistrySubsystem::GetPlayerStateForKey(const FString &CanonicalKey) const {
    if (const TWeakObjectPtr<AMythicPlayerState> *Found = RegisteredPlayerStates.Find(CanonicalKey)) {
        return Found->Get();
    }
    return nullptr;
}

AMythicPlayerController *UMythicPlayerRegistrySubsystem::GetPlayerControllerForKey(const FString &CanonicalKey) const {
    if (const TWeakObjectPtr<AMythicPlayerController> *Found = RegisteredPlayerControllers.Find(CanonicalKey)) {
        return Found->Get();
    }
    return nullptr;
}

APawn *UMythicPlayerRegistrySubsystem::GetPawnForKey(const FString &CanonicalKey) const {
    if (const TWeakObjectPtr<APawn> *Found = RegisteredPawns.Find(CanonicalKey)) {
        return Found->Get();
    }
    return nullptr;
}

bool UMythicPlayerRegistrySubsystem::GetKeyForObject(UObject *PlayerObject, FString &OutCanonicalKey) const {
    if (!PlayerObject) {
        return false;
    }
    if (const FString *FoundKey = ObjectToKeyMap.Find(FObjectKey(PlayerObject))) {
        OutCanonicalKey = *FoundKey;
        return true;
    }
    return false;
}

AMythicPlayerState* UMythicPlayerRegistrySubsystem::ResolveRegisteredKey(const TMap<FString, TWeakObjectPtr<AMythicPlayerState>>& Map, const FString& Key) {
    if (Key.IsEmpty()) {
        return nullptr;
    }

    const TWeakObjectPtr<AMythicPlayerState>* Found = Map.Find(Key);
    if (!Found || !Found->IsValid()) {
        return nullptr;
    }

    return Found->Get();
}

