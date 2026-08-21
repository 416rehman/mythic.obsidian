
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Bounty/MythicBountyRules.h"
#include "MythicBountySubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class UObjectiveDefinition;
class AMythicNPCCharacter;
class APawn;

UCLASS()
class MYTHIC_API UMythicBountySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    int32 GetPendingTelegraphCount() const { return PendingDispatches.Num(); }
    int32 GetTrackedHunterCount() const;

private:
    struct FPendingBountyDispatch {
        int32 Tier = 0;
        double DueTime = 0.0;
        FMythicFactionId Faction;
    };

    void HandleBountyCheck();

    void ProcessDueDispatches(double Now);

    void EvaluatePlayer(class AMythicPlayerController *PC, double Now);

    void TelegraphHunt(const FString &PlayerKey, class AMythicPlayerController *PC, int32 Tier,
                       FMythicFactionId Faction, double Now);

    int32 SpawnHunters(const FString &PlayerKey, APawn *TargetPawn, int32 Tier);

    void RefreshHunterPursuit();

    int32 CountLiveHunters(const FString &PlayerKey);

    UObjectiveDefinition *BuildBountyObjective(int32 Tier, int32 HunterCount);

    void SubmitBountyChronicle(FMythicFactionId Faction, APawn *NearPawn, bool bDispatched);

    bool IsAuthority() const;

    TMap<FString, FPendingBountyDispatch> PendingDispatches;
    TMap<FString, double> LastDispatchTime;
    TMap<FString, TArray<TWeakObjectPtr<AMythicNPCCharacter>>> HuntersByPlayer;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UObjectiveDefinition>> ActiveBountyObjectives;

    FMythicBountyConfig Config;

    FTimerHandle CheckTimerHandle;

    int32 MaxRootedObjectives = 64;
    bool bWarnedMissingHunterContent = false;
};
