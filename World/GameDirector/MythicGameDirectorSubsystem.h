
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "MythicGameDirectorSubsystem.generated.h"

USTRUCT()
struct FCachedSpawnedActorData {
    GENERATED_BODY()

    TSoftClassPtr<AActor> ActorType;

    FTransform Transform;
};

UCLASS()
class MYTHIC_API UMythicGameDirectorSubsystem : public ULocalPlayerSubsystem {
    GENERATED_BODY()

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

public:
    TMap<FName, TArray<AActor *>> SpawnedActors;

    TMap<FName, TArray<FCachedSpawnedActorData>> CachedLevelActorData;
};
