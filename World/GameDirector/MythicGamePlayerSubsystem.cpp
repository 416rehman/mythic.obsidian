// 


#include "MythicGamePlayerSubsystem.h"

#include "Mythic.h"
#include "Streaming/LevelStreamingDelegates.h"

bool UMythicGamePlayerSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    // Should only create the subsystem on the server
    UWorld *World = Outer->GetWorld();
    if (World->WorldType != EWorldType::None && World->GetNetMode() < NM_Client) {
        UE_LOG(Myth, Log, TEXT("GameDirector created on server"));
        return true;
    }

    UE_LOG(Myth, Warning, TEXT("GameDirector will not be created on client"));
    return false;
}

void UMythicGamePlayerSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    FLevelStreamingDelegates::OnLevelBeginMakingInvisible.AddUObject(this, &UMythicGamePlayerSubsystem::OnLevelBeginMakingInvisible);
    FLevelStreamingDelegates::OnLevelBeginMakingVisible.AddUObject(this, &UMythicGamePlayerSubsystem::OnLevelBeginMakingVisible);
    FLevelStreamingDelegates::OnLevelStreamingStateChanged.AddUObject(this, &UMythicGamePlayerSubsystem::OnLevelStreamingStateChanged);
}

void UMythicGamePlayerSubsystem::Deinitialize() {
    Super::Deinitialize();
}

void UMythicGamePlayerSubsystem::OnLevelBeginMakingVisible(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level) {
    ELevelStreamingState State = LevelStreaming->GetLevelStreamingState();
    UE_LOG(Myth, Log, TEXT("Level %s is being made visible. Level Streaming State: %d"), *LevelStreaming->GetFNameForStatID().ToString(), State);

    UE_LOG(Myth, Log, TEXT("List of all the actors in the level:"));
    for (AActor *Actor : Level->Actors) {
        if (!Actor) {
            continue;
        }
        UE_LOG(Myth, Log, TEXT("Actor: %s"), *Actor->GetName());
    }

    // The location of our player
    if (auto local_player = this->GetLocalPlayer()) {
        auto player_controller = local_player->GetPlayerController(World);
        if (player_controller == nullptr) {
            UE_LOG(Myth, Error, TEXT("Player controller is null"));
            return;
        }

        auto pawn = player_controller->GetPawn();
        if (pawn == nullptr) {
            UE_LOG(Myth, Error, TEXT("Pawn is null"));
            return;
        }
        auto player_location = pawn->GetActorLocation();
        UE_LOG(Myth, Log, TEXT("Player location: %s"), *player_location.ToString());

        // Check if the player is in the level
        auto LevelTransform = LevelStreaming->LevelTransform;
        UE_LOG(Myth, Log, TEXT("Level transform: %s"), *LevelTransform.ToString());
        UE_LOG(Myth, Log, TEXT("Level location: %s"), *LevelTransform.GetLocation().ToString());
        UE_LOG(Myth, Log, TEXT("Level rotation: %s"), *LevelTransform.GetRotation().ToString());
        UE_LOG(Myth, Log, TEXT("Level scale: %s"), *LevelTransform.GetScale3D().ToString());

        auto isPlayerInLevel = LevelTransform.GetLocation().Equals(player_location, 50);
    }
}


void UMythicGamePlayerSubsystem::OnLevelBeginMakingInvisible(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level) {
    UE_LOG(Myth, Log, TEXT("Level %s is being made invisible. Level Streaming State: %d"), *Level->GetName(), LevelStreaming->GetLevelStreamingState());

    UE_LOG(Myth, Log, TEXT("List of all the actors in the level:"));
    for (AActor *Actor : Level->Actors) {
        if (!Actor) {
            continue;
        }
        UE_LOG(Myth, Log, TEXT("Actor: %s"), Actor == nullptr ? TEXT("Actor is null") : *Actor->GetName());
    }
}

void UMythicGamePlayerSubsystem::OnLevelStreamingStateChanged(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level,
                                                              ELevelStreamingState LevelStreamingState, ELevelStreamingState LevelStreamingState1) {
    // The level could be null if it is not loaded
    if (Level) {
        UE_LOG(Myth, Log, TEXT("Level %s has changed state. Level Streaming State: %d"), *Level->GetName(), static_cast<uint8>(LevelStreamingState));
    }
    else {
        UE_LOG(Myth, Log, TEXT("Level may not be loaded. Level Streaming State: %d"), static_cast<uint8>(LevelStreamingState));
    }
}
