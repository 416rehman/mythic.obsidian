#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "GameplayTagContainer.h"
#include "World/Farming/MythicFarmingRules.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureRules.h"
#include "World/Gathering/MythicHarvestPressureRules.h"
#include "World/Gathering/MythicYieldQuality.h"
#include "MythicRegionalPressureSubsystem.generated.h"

class AMythicFarmPlot;
class APawn;

UCLASS()
class MYTHIC_API UMythicRegionalPressureSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    void AddPressure(const FVector &Location, const FGameplayTag &Channel, float Amount);

    float QueryPressure(const FVector &Location, const FGameplayTag &Channel);

    void RegisterRatedSource(AActor *Source, const FGameplayTag &Channel, float RatePerSecond);
    void UnregisterRatedSource(AActor *Source, const FGameplayTag &Channel);

    void RegisterFarmPlot(AMythicFarmPlot *Plot);
    void UnregisterFarmPlot(AMythicFarmPlot *Plot);

    void NotifyHuntingKillNear(const FVector &Location);


    /** Records exactly one native authoritative completion; Blueprint cannot emit economy pressure. */
    void ServerRegisterHarvest(const FVector &Location, float Amount = 0.0f);

    /** Returns only the regional pressure multiplier; definition-authored material quantity remains the sole base. */
    float QueryHarvestYieldMultiplier(const FVector &Location);

    /** Applies only regional pressure to a definition-authored base delay; no hardcoded resource-tier curve exists. */
    float ScaledHarvestRespawnDelay(const FVector &Location, float BaseDelay);

    /** Returns whether regional depletion currently gates native regrowth at Location. */
    bool IsHarvestRespawnGated(const FVector &Location);

    /** Degrade a rolled produced-quality tier by the cell's commons depletion (Pristine→Fine→Common, never below
     *  Common). Returns RolledTier unchanged when disabled / pressure 0 / inert quality weight. */
    EMythicYieldQuality DepleteHarvestQuality(const FVector &Location, EMythicYieldQuality RolledTier);

    int32 GetLiveSourceCount() const;

private:
    struct FRatedSource {
        TWeakObjectPtr<AActor> Actor;
        FGameplayTag Channel;
        float RatePerSecond = 0.0f;
        double LastAccrueTime = 0.0;
    };

    struct FPendingFarmRaid {
        FIntPoint Cell = FIntPoint::ZeroValue;
        FVector Center = FVector::ZeroVector;
        double DueTime = 0.0;
        float PressureAtTelegraph = 0.0f;
    };

    void UpdateCheckTimer();
    void HandleCheck();

    void EvaluateFarmCell(const FIntPoint &Cell, const FVector &Center, double Now);
    void ProcessDueRaids(double Now);
    void DispatchFarmRaid(const FPendingFarmRaid &Raid);
    void SubmitFarmChronicle(const FVector &NearLocation, bool bDispatched) const;

    bool IsAuthority() const;
    double Now() const;
    bool IsPacingRestPhase() const;
    FIntPoint CellOf(const FVector &Location) const;
    APawn *NearestPlayerPawn(const FVector &Location, float MaxRadius) const;

    TArray<FRatedSource> Sources;
    TMap<FMythicPressureKey, FMythicPressureCellState> Cells;
    TMap<FIntPoint, FMythicPressureCellState> HabituationByCell;
    TArray<FPendingFarmRaid> PendingRaids;
    TMap<FIntPoint, double> LastRaidTimeByCell;
    FTimerHandle CheckTimerHandle;

    FMythicRegionalPressureConfig Config;
    FMythicFarmingConfig FarmingConfig;
    FMythicHarvestPressureConfig HarvestConfig;
    bool bFarmRaidsEnabled = false;
    bool bHarvestPressureEnabled = true;
    bool bWarnedMissingRaidContent = false;
};
