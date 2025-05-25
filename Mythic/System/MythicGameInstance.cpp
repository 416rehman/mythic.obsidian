// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "Streaming/LevelStreamingDelegates.h"
#include "World/GameDirector/MythicGamePlayerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicGameInstance)

UMythicGameInstance::UMythicGameInstance(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    LoadingScreenWidget = nullptr;
}

void UMythicGameInstance::Init() {
    Super::Init();
}

void UMythicGameInstance::Shutdown() {
    Super::Shutdown();
}

bool UMythicGameInstance::CanJoinRequestedSession() const {
    // Temporary first pass:  Always return true
    // This will be fleshed out to check the player's state
    if (!Super::CanJoinRequestedSession()) {
        return false;
    }
    return true;
}

// Adds a widget to the viewport, tries to load the level async, waits for the level to be loaded async, and then removes the widget
void UMythicGameInstance::LoadLevel(FName LevelName, TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass) {
    // Check if the soft class pointer is valid AND if the asset is loaded
    if (!LoadingScreenWidgetClass.IsValid()) {
        LoadingScreenWidgetClass.LoadSynchronous();
    }
    // Check if the asset is loaded
    if (!LoadingScreenWidgetClass.IsValid()) {
        UE_LOG(LogTemp, Error, TEXT("Failed to load the loading screen widget class"));
        return;
    }

    LoadingScreenWidget = CreateWidget<UUserWidget>(this, LoadingScreenWidgetClass.Get());
    if (LoadingScreenWidget) {
        auto ViewPort = GetGameViewportClient();
        if (ViewPort) {
            ViewPort->AddViewportWidgetForPlayer(GetFirstGamePlayer(), LoadingScreenWidget->TakeWidget(), 1);
        }
        else {
            LoadingScreenWidget->AddToViewport();
        }
    }
    // Get the game player subsystem
    ULocalPlayer *LocalPlayer = GetFirstGamePlayer();
    UMythicGamePlayerSubsystem *GamePlayerSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UMythicGamePlayerSubsystem>() : nullptr;
    if (GamePlayerSubsystem) {
        // Bind to the on level loaded delegate
        FLevelStreamingDelegates::OnLevelStreamingStateChanged.AddUObject(this, &UMythicGameInstance::OnLevelLoaded);

        MapName = LevelName;
        UGameplayStatics::OpenLevel(GetWorld(), LevelName, true, "");
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to get the game player subsystem"));
    }
}

void UMythicGameInstance::OnLevelLoaded(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level, ELevelStreamingState OldState,
                                        ELevelStreamingState NewState) {
    auto IncomingMap = World->GetName();
    if (IncomingMap == MapName && NewState == ELevelStreamingState::LoadedVisible) {
        // Unbind from the on level loaded delegate
        FLevelStreamingDelegates::OnLevelStreamingStateChanged.RemoveAll(this);

        if (LoadingScreenWidget) {
            LoadingScreenWidget->RemoveFromParent();
            LoadingScreenWidget = nullptr;
        }
    }
}
