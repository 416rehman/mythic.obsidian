// Mythic Living World System — Background Simulation Thread
// Runs faction sim, territory propagation, ideology metabolism, etc. off the game thread.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "HAL/RunnableThread.h"

class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicTerritoryGrid;
class UMythicLivingWorldSettings;

/**
 * Dedicated background thread for world simulation.
 *
 * Ticks at a configurable real-time interval (default 1s real time = ~5-30s game time).
 * Runs: economy, population, diplomacy, territory propagation, ideology metabolism,
 * scheme engine, crystallization, history append.
 *
 * After each tick, commits all write buffers so game thread gets fresh snapshots.
 * Never blocks the game thread — all shared data uses double-buffered reads.
 */
class MYTHIC_API FMythicWorldSimThread : public FRunnable {
#if WITH_AUTOMATION_WORKER
    friend class FLivingWorldSimEconomyTest;
    friend class FLivingWorldSimPopulationTest;
    friend class FLivingWorldSimDiplomacyTest;
#endif

public:
    FMythicWorldSimThread();
    virtual ~FMythicWorldSimThread() override;

    /**
     * Initialize the thread with references to the shared data systems.
     * These UObject pointers must remain valid for the lifetime of the thread.
     */
    void Setup(
        UMythicCausalFabric *InFabric,
        UMythicFactionDatabase *InFactionDB,
        UMythicTerritoryGrid *InTerritoryGrid,
        const UMythicLivingWorldSettings *InSettings,
        float InTickIntervalSeconds,
        FCriticalSection* InSimulationLock
    );

    /** Start the background thread */
    void StartThread();

    /** Signal the thread to stop and wait for completion */
    void StopThread();

    /** Is the thread currently running? */
    bool IsRunning() const { return bRunning.load(std::memory_order_relaxed); }

    // ─── FRunnable Interface ──────────────────────────────

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

private:
    /** Perform one simulation tick. Called on the background thread. */
    void SimTick();

    /** Commit all write buffers so game thread gets fresh data */
    void CommitAllSnapshots();

    // ─── Simulation sub-steps ─────────────────────────────

    void TickEconomy();
    void TickPopulation();
    void TickDiplomacy();
    void TickTerritoryPropagation();
    void TickIdeologyMetabolism();
    void TickFactionEvolution();
    void TickSchemeEngine();
    void TickCrystallization();
    void TickHistoryAppend();

    // ─── State ────────────────────────────────────────────

    /** Thread instance. Owned by this class. */
    FRunnableThread *Thread = nullptr;

    /** Should the thread keep running? Atomic flag for stop signaling. */
    std::atomic<bool> bRunning{false};

    /** Real-time interval between simulation ticks (seconds) */
    float TickIntervalSeconds = 1.0f;

    /** Monotonically increasing tick counter for diagnostics */
    uint64 TickCount = 0;

    // ─── References to shared data (NOT owned) ────────────

    UMythicCausalFabric *Fabric = nullptr;
    UMythicFactionDatabase *FactionDB = nullptr;
    UMythicTerritoryGrid *TerritoryGrid = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;
    FCriticalSection* SimulationLock = nullptr;

    /**
     * Pairwise trade volume accumulator for economic dependency.
     * Indexed as [A * MaxFactions + B]. Background thread only.
     */
    TArray<float> TradeVolume;
    int32 MaxFactions = 0;
};
