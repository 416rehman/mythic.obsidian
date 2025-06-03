// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonGameInstance.h"
#include "UObject/UObjectGlobals.h"
#include "MythicGameInstance.generated.h"

UCLASS(Config = Game)
class MYTHIC_API UMythicGameInstance : public UCommonGameInstance {
    GENERATED_BODY()

public:
    UMythicGameInstance(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());
    virtual bool CanJoinRequestedSession() const override;

    UFUNCTION(BlueprintCallable, Category = "Mythic Game Instance")
    void LoadLevel(FName LevelName, TSoftClassPtr<UUserWidget> LoadingScreenWidgetClass);

protected:
    virtual void Init() override;
    virtual void Shutdown() override;

    // Widget to display while loading a level
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mythic Game Instance")
    UUserWidget *LoadingScreenWidget;

    UPROPERTY()
    FName MapName = "";

    void OnLevelLoaded(UWorld * World, const ULevelStreaming * LevelStreaming, ULevel * Level, ELevelStreamingState OldState, ELevelStreamingState NewState);
};
