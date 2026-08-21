
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


    /** Record ONE completed harvest on the cell at Location: pushes Pressure.Harvest (Amount, or the configured
     *  HarvestPressurePerGather when Amount <= 0). Server-authoritative (AddPressure self-guards). Call from the owner
     *  gather-completion one-liner, or from content/BP. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Harvest Pressure")
    void ServerRegisterHarvest(const FVector &Location, float Amount = 0.0f);

    /** Yield multiplier for a tier-N node at Location after commons depletion (1.0 when disabled / pressure 0 / inert
     *  weights; drops toward the floor as the cell is hammered). */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Harvest Pressure")
    float QueryHarvestYieldMultiplier(const FVector &Location, int32 ResourceTier = 0);

    /** Respawn delay for a tier-N node at Location after commons depletion (baseline when disabled / pressure 0 / inert
     *  weights; lengthens as the cell is hammered). */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Harvest Pressure")
    float ScaledHarvestRespawnDelay(const FVector &Location, float BaseDelay, int32 ResourceTier = 0);

    /** Is regrowth GATED at Location by commons depletion? (false when disabled / pressure below the threshold). A
     *  ruined grove stays gone until it lies fallow enough to decay below RespawnGateThreshold. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Harvest Pressure")
    bool IsHarvestRespawnGated(const FVector &Location);

    /** Degrade a rolled produced-quality tier by the cell's commons depletion (Pristine→Fine→Common, never below
     *  Common). Returns RolledTier unchanged when disabled / pressure 0 / inert quality weight. */
    UFUNCTION(BlueprintCallable, Category = "Mythic|Harvest Pressure")
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
