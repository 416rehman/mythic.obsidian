
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Engine/TimerHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Vendetta/MythicVendettaTypes.h"
#include "World/LivingWorld/Vendetta/MythicPlayerThreatLedger.h"
#include "MythicVendettaSubsystem.generated.h"

class UMythicLivingWorldSubsystem;
class UObjectiveDefinition;

UCLASS()
class MYTHIC_API UMythicVendettaSubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Deinitialize() override;

    int32 GetTrackedGrudgeCount() const { return Ledger.GetEntries().Num(); }

    float GetMaxThreatForPlayer(const FString &PlayerKey) const;

private:
    void HandleVendettaTick();

    void ObserveStandingSignals();

    void EvaluateAndExecuteVendettas();

    void ExecuteVendetta(const FString &PlayerKey, FMythicFactionId Faction, EMythicVendettaType Type,
                         const FText &FactionName);

    UObjectiveDefinition *BuildVendettaObjective(EMythicVendettaType Type, const FText &FactionName);

    void SubmitVendettaChronicle(FMythicFactionId Faction, EMythicVendettaType Type, FMythicCellCoord Cell);

    bool IsPacingRestPhase() const;

    bool IsAuthority() const;

    FMythicPlayerThreatLedger Ledger;

    TMap<FMythicThreatKey, float> StandingSnapshot;

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld = nullptr;

    UPROPERTY()
    TArray<TObjectPtr<UObjectiveDefinition>> ActiveVendettaObjectives;

    FMythicVendettaThresholds Thresholds;

    FTimerHandle TickTimerHandle;

    float TickIntervalSeconds = 10.0f;
    float ThreatDecayRatePerSec = 0.5f;
    float ThreatWeightPerStandingLost = 1.0f;
    int32 MaxRootedObjectives = 64;
};
