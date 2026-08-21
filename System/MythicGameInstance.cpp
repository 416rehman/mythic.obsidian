// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicGameInstance.h"

#include "Blueprint/UserWidget.h"
#include "Kismet/GameplayStatics.h"
#include "MythicAssetManager.h"
#include "Engine/GameViewportClient.h"
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
    if (!Super::CanJoinRequestedSession()) {
        return false;
    }
    return true;
}

void UMythicGameInstance::LoadLevel(FName LevelName, TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass) {
    MapName = LevelName;

    UMythicAssetManager::LoadAsync(this, LoadingScreenWidgetClass,
                                   [this](TSubclassOf<UUserWidget> LoadedClass) {
                                       OnLoadingScreenClassLoaded(LoadedClass);
                                   });
}

void UMythicGameInstance::OnLoadingScreenClassLoaded(TSubclassOf<UUserWidget> LoadedClass) {
    if (LoadedClass) {
        LoadingScreenWidget = CreateWidget<UUserWidget>(this, LoadedClass);
        if (LoadingScreenWidget) {
            auto ViewPort = GetGameViewportClient();
            if (ViewPort) {
                ViewPort->AddViewportWidgetForPlayer(GetFirstGamePlayer(), LoadingScreenWidget->TakeWidget(), 1);
            }
            else {
                LoadingScreenWidget->AddToViewport();
            }
        }
    }

    ULocalPlayer *LocalPlayer = GetFirstGamePlayer();
    UMythicGamePlayerSubsystem *GamePlayerSubsystem = LocalPlayer ? LocalPlayer->GetSubsystem<UMythicGamePlayerSubsystem>() : nullptr;
    if (GamePlayerSubsystem) {
        FLevelStreamingDelegates::OnLevelStreamingStateChanged.AddUObject(this, &UMythicGameInstance::OnLevelLoaded);
        UGameplayStatics::OpenLevel(GetWorld(), MapName, true, "");
    }
    else {
        UE_LOG(LogTemp, Error, TEXT("Failed to get the game player subsystem"));
    }
}

void UMythicGameInstance::OnLevelLoaded(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level, ELevelStreamingState OldState,
                                        ELevelStreamingState NewState) {
    auto IncomingMap = World->GetName();
    if (IncomingMap == MapName && NewState == ELevelStreamingState::LoadedVisible) {
        FLevelStreamingDelegates::OnLevelStreamingStateChanged.RemoveAll(this);

        if (LoadingScreenWidget) {
            LoadingScreenWidget->RemoveFromParent();
            LoadingScreenWidget = nullptr;
        }
    }
}
