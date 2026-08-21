
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Acquaintance/MythicMourningRules.h"
#include "MythicAvengerSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class UObjectiveDefinition;
class AMythicNPCCharacter;
class AMythicPlayerController;

UCLASS()
class MYTHIC_API UMythicAvengerSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    void NotifyNpcKilledByPlayer(uint32 VictimNameHash, const FText &VictimName, FGameplayTag VictimRole,
                                 float Significance, FMythicFactionId VictimFaction, AMythicPlayerController *KillerPC);

    int32 GetPendingAvengerCount() const { return PendingDispatches.Num(); }
    int32 GetTrackedAvengerCount() const;

private:
    struct FPendingAvengerDispatch {
        double DueTime = 0.0;
        FText VictimName;
        FMythicFactionId Faction;
    };

    void HandleAvengerCheck();
    void ProcessDueDispatches(double Now);
    void RefreshAvengerPursuit();

    bool SpawnAvenger(const FString &PlayerKey, APawn *TargetPawn);

    int32 CountLiveAvengers(const FString &PlayerKey);

    UObjectiveDefinition *BuildAvengerObjective(const FText &VictimName);

    void SubmitAvengerChronicle(FMythicFactionId Faction, APawn *NearPawn, bool bDispatched);

    bool IsAuthority() const;

    TMap<FString, FPendingAvengerDispatch> PendingDispatches;
    TMap<FString, double> LastDispatchTime;
    TMap<FString, TArray<TWeakObjectPtr<AMythicNPCCharacter>>> AvengersByPlayer;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UObjectiveDefinition>> ActiveAvengerObjectives;

    FMythicAvengerConfig Config;

    FTimerHandle CheckTimerHandle;

    int32 MaxRootedObjectives = 64;
    bool bWarnedMissingContent = false;
};
