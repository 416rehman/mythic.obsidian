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
class UMythicSchemeEngine;
// Used only as TArray<FMythicWorldEvent>* members below; forward-declared so this header is self-contained
// (the full definition lives in CausalFabric/CausalFabric.h and is included by the .cpp). A unity-build
// regrouping otherwise leaves FMythicWorldEvent undeclared here.
struct FMythicWorldEvent;
// Used by-value in the static diplomacy helpers below; forward-declared with its underlying type (full definition in
// Factions/FactionDatabase.h, included by the .cpp) to keep this header light.
enum class EMythicFactionRelation : uint8;

DECLARE_MULTICAST_DELEGATE(FOnWorldSimCommitted);

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

    /** Fired on the background thread immediately after CommitAllSnapshots finishes */
    FOnWorldSimCommitted OnWorldSimCommitted;

    /**
     * Initialize the thread with references to the shared data systems.
     * These UObject pointers must remain valid for the lifetime of the thread.
     */
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

    /** Start the background thread */
    void StartThread();

    /** Signal the thread to stop and wait for completion */
    void StopThread();

    /** Is the thread currently running? */
    bool IsRunning() const { return bRunning.load(std::memory_order_relaxed); }

    /** Monotonic sim-tick counter (diagnostics). The caller MUST hold the SimulationLock — TickCount is mutated on the
     *  background thread inside SimTick; this getter only exposes it so a SimulationLock-guarded copy-out (the Living
     *  World subsystem's CopySimDiagnostics) can read it race-free. NOT atomic; do not read lock-free. */
    uint64 GetTickCount() const { return TickCount; }

    /** Configured real-time interval between sim ticks (seconds). Set once at Setup; safe to read under SimulationLock. */
    float GetTickIntervalSeconds() const { return TickIntervalSeconds; }

    // ─── FRunnable Interface ──────────────────────────────

    virtual bool Init() override;
    virtual uint32 Run() override;
    virtual void Stop() override;
    virtual void Exit() override;

    // ── Pure diplomacy helpers (static + testable; consumed by TickDiplomacy) ──

    /** Map a composite diplomacy score to a relationship tier WITH HYSTERESIS: entering a NON-current tier requires
     *  overshooting its threshold by Hyst; staying in the current tier needs only the base threshold (sticky → no
     *  relationship flicker). Pure so the core faction-relationship arbitration is unit-testable. */
    static EMythicFactionRelation MapDiplomacyScoreToRelation(
        float Score, EMythicFactionRelation Current,
        float AllyThreshold, float FriendlyThreshold, float UnfriendlyThreshold, float HostileThreshold, float Hyst);

    /** Chronicle significance of a diplomacy shift, scaled by the NEW relationship's extremity — a war (Hostile) or
     *  alliance (Allied) is far more newsworthy than a minor warming/cooling (was a flat 0.5 for every shift). Pure. */
    static float DiplomacyShiftSignificance(EMythicFactionRelation NewRelation);

    /** One spawn-growth step for a creature/spawning faction, BOUNDED by the territory's carrying capacity
     *  (CellCount * PopulationPerCell). Grows by CellCount*SpawnRatePerCell but never past the capacity — and never
     *  FORCES a decline below the current population (so a faction temporarily holding 0 cells, capacity 0, is left
     *  alone instead of being zeroed into annihilation). Without this bound the creature path grew Population every
     *  tick forever → runaway spawn pressure + eventual int32 overflow (the civilian path already caps at capacity).
     *  Pure + static for unit testing. */
    static int32 ComputeCappedSpawnPopulation(int32 CurrentPop, int32 CellCount, int32 SpawnRatePerCell, int32 PopulationPerCell);

    /** Whether a faction splinters this tick: a destabilizing signal (ideological divergence past threshold OR
     *  geographic fragmentation into disconnected territory clusters) AND enough population that BOTH post-split halves
     *  stay viable. The schism transfers half the population, so the gate is `Population >= MinSchismPopulation * 2`
     *  (each half then clears MinSchismPopulation) — NOT just `>= MinSchismPopulation`, which would spawn a stillborn
     *  splinter. Pure + static for unit testing; the divergence/fragmentation signals are computed by the caller. */
    static bool ShouldFactionSchism(float InternalDivergence, float IdeologyThreshold, bool bGeographicallyFragmented,
                                    int32 Population, int32 MinSchismPopulation);

    /** One ideology-axis step: exponential approach of Current toward Target by Rate, clamped to the valid axis range
     *  [-1, 1]. `Rate` 0 = no change, 1 = jump to (clamped) Target. Single source for BOTH event-driven ideology drift
     *  and cross-faction ideology bleed in TickIdeologyMetabolism (identical lerp, was duplicated). Pure + static for
     *  unit testing. */
    static float DriftTowardClamped(float Current, float Target, float Rate);

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
    class UMythicSettlementRegistry *SettlementRegistry = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;
    FCriticalSection *SimulationLock = nullptr;
    UMythicSchemeEngine *SchemeEngine = nullptr;
    TArray<FMythicWorldEvent> *PendingEvents = nullptr;
    FCriticalSection *PendingEventsMutex = nullptr;

    /**
     * Pairwise trade volume accumulator for economic dependency.
     * Indexed as [A * MaxFactions + B]. Background thread only.
     */
    TArray<float> TradeVolume;
    int32 MaxFactions = 0;
};
