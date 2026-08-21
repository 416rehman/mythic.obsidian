
#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicTerritoryGrid;
class UMythicLivingWorldSettings;
class UMythicSchemeEngine;
struct FMythicWorldEvent;
enum class EMythicFactionRelation : uint8;

DECLARE_MULTICAST_DELEGATE(FOnWorldSimCommitted);

class MYTHIC_API FMythicWorldSimThread : public FRunnable {
#if WITH_AUTOMATION_WORKER
    friend class FLivingWorldSimEconomyTest;
    friend class FLivingWorldSimPopulationTest;
    friend class FLivingWorldSimDiplomacyTest;
#endif

public:
    FMythicWorldSimThread();
    virtual ~FMythicWorldSimThread() override;

    FOnWorldSimCommitted OnWorldSimCommitted;

    void Setup(
        UMythicCausalFabric *InFabric,
        UMythicFactionDatabase *InFactionDB,
        UMythicTerritoryGrid *InTerritoryGrid,
        class UMythicSettlementRegistry *InSettlementRegistry,
        const UMythicLivingWorldSettings *InSettings,
        float InTickIntervalSeconds,
        FCriticalSection *InSimulationLock,
        UMythicSchemeEngine *InSchemeEngine = nullptr,
        TArray<FMythicWorldEvent> *InPendingEvents = nullptr,
        FCriticalSection *InPendingEventsMutex = nullptr
        );

    void StartThread();

    void StopThread();

    bool IsRunning() const { return bRunning.load(std::memory_order_relaxed); }

    uint64 GetTickCount() const { return TickCount; }

    float GetTickIntervalSeconds() const { return TickIntervalSeconds; }


    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;


    static EMythicFactionRelation MapDiplomacyScoreToRelation(
        float Score, EMythicFactionRelation Current,
        float AllyThreshold, float FriendlyThreshold, float UnfriendlyThreshold, float HostileThreshold, float Hyst);

    static float DiplomacyShiftSignificance(EMythicFactionRelation NewRelation);

    static int32 ComputeCappedSpawnPopulation(int32 CurrentPop, int32 CellCount, int32 SpawnRatePerCell, int32 PopulationPerCell);

    static bool ShouldFactionSchism(float InternalDivergence, float IdeologyThreshold, bool bGeographicallyFragmented,
                                    int32 Population, int32 MinSchismPopulation);

    static float DriftTowardClamped(float Current, float Target, float Rate);

private:
    void SimTick();

    void CommitAllSnapshots();


    void TickEconomy();
    void TickPopulation();
    void TickDiplomacy();
    void TickTerritoryPropagation();
    void TickIdeologyMetabolism();
    void TickFactionEvolution();
    void TickSchemeEngine();
    void TickCrystallization();
    void TickHistoryAppend();


    FRunnableThread *Thread = nullptr;

    std::atomic<bool> bRunning{false};

    float TickIntervalSeconds = 1.0f;

    uint64 TickCount = 0;


    UMythicCausalFabric *Fabric = nullptr;
    UMythicFactionDatabase *FactionDB = nullptr;
    UMythicTerritoryGrid *TerritoryGrid = nullptr;
    class UMythicSettlementRegistry *SettlementRegistry = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;
    FCriticalSection *SimulationLock = nullptr;
    UMythicSchemeEngine *SchemeEngine = nullptr;
    TArray<FMythicWorldEvent> *PendingEvents = nullptr;
    FCriticalSection *PendingEventsMutex = nullptr;

    TArray<float> TradeVolume;
    int32 MaxFactions = 0;
};
