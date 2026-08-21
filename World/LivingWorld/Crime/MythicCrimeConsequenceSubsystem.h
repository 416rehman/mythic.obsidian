
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicCrimeConsequenceSubsystem.generated.h"

class UMythicActionEventSubsystem;
class UMythicPlayerRegistrySubsystem;
class APawn;

UCLASS()
class MYTHIC_API UMythicCrimeConsequenceSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;


    static float ComputeNotorietyDelta(EMythicMoralSeverity Severity, float BaseDelta);

    static bool ShouldDispatchGuards(float Notoriety, float Threshold, bool bAlreadyDispatched);

private:
    void DrainCrimeQueue();

    int32 DispatchGuardResponse(APawn *PerpPawn, FMythicFactionId OffendedFaction);

    static FString MakeDispatchKey(const FString &PlayerKey, FMythicFactionId Faction);
    TMap<FString, double> LastGuardDispatchTime;

    TWeakObjectPtr<UMythicActionEventSubsystem> CachedActionSubsystem;
    TWeakObjectPtr<UMythicPlayerRegistrySubsystem> CachedPlayerRegistry;

    FTimerHandle DrainTimerHandle;


    float DrainIntervalSeconds = 2.0f;

    int32 MaxCrimesPerDrain = 16;

    float NotorietyBaseDelta = 15.0f;

    float GuardDispatchCooldownSeconds = 30.0f;

    float GuardAlertRadius = 1500.0f;

    int32 GuardAlertMaxResponders = 8;
};
