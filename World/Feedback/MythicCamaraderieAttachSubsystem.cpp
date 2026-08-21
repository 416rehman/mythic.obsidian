
#include "MythicCamaraderieAttachSubsystem.h"

#include "MythicCamaraderieComponent.h"
#include "Player/MythicPlayerState.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"

bool UMythicCamaraderieAttachSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UMythicCamaraderieAttachSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);
    PostLoginHandle = FGameModeEvents::OnGameModePostLoginEvent().AddUObject(this, &UMythicCamaraderieAttachSubsystem::HandlePostLogin);
}

void UMythicCamaraderieAttachSubsystem::Deinitialize() {
    if (PostLoginHandle.IsValid()) {
        FGameModeEvents::OnGameModePostLoginEvent().Remove(PostLoginHandle);
        PostLoginHandle.Reset();
    }
    Super::Deinitialize();
}

bool UMythicCamaraderieAttachSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

void UMythicCamaraderieAttachSubsystem::HandlePostLogin(AGameModeBase *GameMode, APlayerController *NewPlayer) {
    if (!IsAuthority() || !NewPlayer || (GameMode && GameMode->GetWorld() != GetWorld())) {
        return;
    }
    AMythicPlayerState *PS = NewPlayer->GetPlayerState<AMythicPlayerState>();
    if (!PS) {
        return;
    }
    if (PS->FindComponentByClass<UMythicCamaraderieComponent>()) {
        return;
    }
    UMythicCamaraderieComponent *Camaraderie = NewObject<UMythicCamaraderieComponent>(PS);
    Camaraderie->RegisterComponent();
}
