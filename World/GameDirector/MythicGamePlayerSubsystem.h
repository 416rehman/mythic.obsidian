
#pragma once

#include "Engine/LevelStreaming.h"
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicGamePlayerSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(FOnLevelStreamingStateChanged, UWorld*, World, const ULevelStreaming*, LevelStreaming, ULevel*, Level, uint8,
                                              PreviousState, uint8, NewState);

UCLASS()
class MYTHIC_API UMythicGamePlayerSubsystem : public ULocalPlayerSubsystem {
    GENERATED_BODY()

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    void OnLevelBeginMakingVisible(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level);
    void OnLevelBeginMakingInvisible(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level);
    void OnLevelStreamingStateChanged(UWorld *World, const ULevelStreaming *LevelStreaming, ULevel *Level, ELevelStreamingState LevelStreamingState,
                                      ELevelStreamingState LevelStreamingState1);
};
