// Mythic Living World — Scheme Engine
// Background-thread system that generates and progresses faction schemes.
// Runs inside the WorldSimThread tick cycle.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/Simulation/SchemeTypes.h"
#include "SchemeEngine.generated.h"

class UMythicFactionDatabase;
class UMythicCausalFabric;
class UMythicTerritoryGrid;
class UMythicLivingWorldSettings;

/**
 * Scheme Engine — generates and progresses faction schemes on the background simulation thread.
 *
 * Threading:
 * - TickSchemes() called from WorldSimThread::SimTick() — background thread only
 * - Reads from FactionDB, TerritoryGrid (lock-free snapshots from game thread)
 * - Writes completed scheme events to CausalFabric (single writer, safe)
 * - Exposes read-only game thread query via GetActiveSchemes() + GetSchemesFence()
 *
 * Performance:
 * - Max 50 total active schemes (MaxTotalSchemes)
 * - Max 5 per faction (MaxSchemesPerFaction)
 * - O(factions × MaxSchemesPerFaction) per tick ≈ O(20 × 5) = 100 operations
 * - Scheme generation only runs every N sim ticks (configurable)
 */
UCLASS()
class MYTHIC_API UMythicSchemeEngine : public UObject {
    GENERATED_BODY()

public:
    /**
     * Initialize with shared data references.
     * Must be called before first tick.
     */
    void Initialize(
        UMythicFactionDatabase* InFactionDB,
        UMythicCausalFabric* InFabric,
        UMythicTerritoryGrid* InTerritoryGrid,
        const UMythicLivingWorldSettings* InSettings);

    // ─── Background Thread Interface ──────────────────────

    /**
     * Progress all active schemes. Called from WorldSimThread.
     * @param SimDeltaTime   Simulation delta time (seconds in game time)
     * @param SimTickIndex   Monotonic tick counter (for generation throttle)
     */
    void TickSchemes(float SimDeltaTime, uint32 SimTickIndex);

    // ─── Game Thread Queries ──────────────────────────────

    /**
     * Get a snapshot of all active schemes. Safe to call from game thread.
     * Returns a copy (not a reference) for thread safety.
     */
    TArray<FMythicScheme> GetActiveSchemes() const;

    /**
     * Get schemes by origin faction.
     */
    TArray<FMythicScheme> GetSchemesByFaction(FMythicFactionId Faction) const;

    /**
     * Get total active scheme count.
     */
    int32 GetActiveSchemeCount() const;

    // ─── Serialization ───────────────────────────────────

    /** Serialize active schemes and state for save/load. */
    void Serialize(FArchive& Ar);

private:
    // ─── Scheme Generation ────────────────────────────────

    /**
     * Evaluate each faction and potentially generate new schemes.
     * Budget-capped: max 1 new scheme per N sim ticks.
     */
    void GenerateSchemes(float SimDeltaTime, uint32 SimTickIndex);

    /**
     * Determine which scheme types are available for a faction based on its state.
     * @param FactionIndex Index into the faction database
     * @param OutEligibleTypes Populated with eligible scheme types
     */
    void GetEligibleSchemeTypes(int32 FactionIndex, TArray<EMythicSchemeType>& OutEligibleTypes) const;

    /**
     * Calculate progress rate for a scheme based on faction resources and military strength.
     */
    float CalculateProgressRate(const FMythicScheme& Scheme, int32 FactionIndex) const;

    /**
     * Calculate detection risk based on target faction's intelligence/strength.
     */
    float CalculateDetectionRisk(const FMythicScheme& Scheme, int32 TargetFactionIndex) const;

    // ─── Scheme Progression ───────────────────────────────

    /**
     * Progress a single scheme and handle state transitions.
     */
    void ProgressScheme(FMythicScheme& Scheme, float SimDeltaTime);

    /**
     * Execute a completed scheme — write event to causal fabric and apply effects.
     */
    void ExecuteScheme(FMythicScheme& Scheme);

    /**
     * Handle scheme discovery — notify target faction, may escalate conflict.
     */
    void OnSchemeDiscovered(FMythicScheme& Scheme);

    // ─── Shared Data References ───────────────────────────

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> Fabric;

    UPROPERTY()
    TObjectPtr<UMythicTerritoryGrid> TerritoryGrid;

    const UMythicLivingWorldSettings* Settings = nullptr;

    // ─── Scheme State ─────────────────────────────────────

    /** All active schemes (accessed from background thread, copies for game thread) */
    TArray<FMythicScheme> ActiveSchemes;

    /** Lock for game thread reads of ActiveSchemes */
    mutable FCriticalSection SchemeLock;

    /** Next scheme ID (monotonically increasing) */
    uint32 NextSchemeId = 1;

    /** Tick interval for scheme generation (generate every N ticks, not every tick) */
    int32 GenerationTickInterval = 10;

    /** Max concurrent active schemes per faction */
    int32 MaxSchemesPerFaction = 5;

    /** Max total active schemes across all factions */
    int32 MaxTotalSchemes = 50;

    /** Base probability for scheme generation per faction per tick */
    float SchemeBaseProbability = 0.05f;
};
