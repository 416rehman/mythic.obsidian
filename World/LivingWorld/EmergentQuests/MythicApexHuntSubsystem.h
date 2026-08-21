
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/EmergentQuests/MythicApexHuntRules.h"
#include "World/Hunting/MythicSpoorRules.h"
#include "MythicApexHuntSubsystem.generated.h"

class UObjectiveDefinition;
class AMythicPlayerController;
class AMythicNPCCharacter;
class APawn;

UCLASS()
class MYTHIC_API UMythicApexHuntSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    int32 GetActiveOfferCount() const { return ActiveOffers.Num(); }

private:
    struct FMythicActiveApexOffer {
        int32 SpeciesIndex = INDEX_NONE;
        UObjectiveDefinition *Objective = nullptr;
        TWeakObjectPtr<AMythicNPCCharacter> Apex;
        double ExpireTimeSeconds = 0.0;
    };

    void HandleCheck();

    bool TryOfferSpecies(int32 SpeciesIndex);

    UObjectiveDefinition *BuildApexObjective(const FMythicApexHuntSpecies &Species, const FVector &HuntSite);

    void SpawnTrailStart(const FVector &HunterLocation, const FVector &HuntSite);

    UFUNCTION()
    void HandleApexDeath(AActor *DeadActor);

    void RetireOffer(int32 OfferIndex, bool bKilled);

    void SubmitApexChronicle(const FVector &NearLocation, bool bCompleted) const;

    bool IsAuthority() const;
    double Now() const;
    bool IsRainingNow() const;

    TArray<FMythicActiveApexOffer> ActiveOffers;
    TMap<int32, double> LastOfferEndTimeBySpecies;
    FTimerHandle CheckTimerHandle;

    UPROPERTY()
    TArray<TObjectPtr<UObjectiveDefinition>> ActiveObjectiveRoots;

    FMythicApexHuntConfig Config;
    FMythicSpoorConfig SpoorConfig;
    bool bEnabled = false;
    bool bWarnedMissingContent = false;
};
