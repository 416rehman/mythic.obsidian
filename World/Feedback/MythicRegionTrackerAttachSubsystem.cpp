
#include "MythicRegionTrackerAttachSubsystem.h"

#include "MythicRegionTrackerComponent.h"
#include "Player/MythicPlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

bool UMythicRegionTrackerAttachSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UMythicRegionTrackerAttachSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
    PostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &UMythicRegionTrackerAttachSubsystem::HandlePostLogin);
}

void UMythicRegionTrackerAttachSubsystem::Deinitialize() {
    if (PostLoginHandle.IsValid()) {
        FGameModeEvents::OnGameModePostLoginEvent().Remove(PostLoginHandle);
        PostLoginHandle.Reset();
    }
    Super::Deinitialize();
}

bool UMythicRegionTrackerAttachSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

void UMythicRegionTrackerAttachSubsystem::HandlePostLogin(AGameModeBase *GameMode, APlayerController *NewPlayer) {
    if (!IsAuthority() || !NewPlayer || (GameMode && GameMode->GetWorld() != GetWorld())) {
        return;
    }
    AMythicPlayerState *PS = NewPlayer->GetPlayerState<AMythicPlayerState>();
    if (!PS) {
        return;
    }
    if (PS->FindComponentByClass<UMythicRegionTrackerComponent>()) {
        return;
    }
    UMythicRegionTrackerComponent *Tracker = NewObject<UMythicRegionTrackerComponent>(PS);
    Tracker->RegisterComponent();
}
