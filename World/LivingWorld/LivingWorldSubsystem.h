// Mythic Living World System — Central Subsystem
// Manages the living world lifecycle: initialization, shared data, background thread.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "LivingWorldSubsystem.generated.h"

class UMythicLivingWorldSettings;
class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicTerritoryGrid;
class UMythicSettlementRegistry;
class AMythicSettlement;
class UMythicPersistentNPCRegistry;
class UMythicFactionDatabaseSettings;
class UMythicTerritoryGridSettings;
class UMythicSocialGraph;
class UMythicSchemeEngine;

/**
 * Central coordinator for the Living World System.
 * As a GameInstanceSubsystem, it lives for the entire game session.
 *
 * Responsibilities:
 * - Owns the shared data objects (Causal Fabric, Faction DB, Territory Grid)
 * - Owns and manages the background simulation thread
 * - Provides game-thread access to the shared data read snapshots
 * - Initializes/shuts down the system cleanly
 */
UCLASS()
class MYTHIC_API UMythicLivingWorldSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:
    // ─── GameInstanceSubsystem Interface ──────────────────

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    /** Destructor — defined in .cpp where FMythicWorldSimThread is a complete type (required by TUniquePtr) */
    virtual ~UMythicLivingWorldSubsystem() override;

    // ─── Shared Data Accessors (Game Thread Safe) ─────────

    /** Get the causal fabric for event queries. Lock-free read. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicCausalFabric *GetCausalFabric() const { return CausalFabric; }

    /** Get the faction database for faction queries. Lock-free read. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicFactionDatabase *GetFactionDatabase() const { return FactionDB; }

    /** Get the territory grid for spatial queries. Lock-free read. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicTerritoryGrid *GetTerritoryGrid() const { return TerritoryGrid; }

    /** Get the settings data asset */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    const UMythicLivingWorldSettings *GetSettings() const { return Settings; }

    /** Get the settlement registry for settlement queries. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicSettlementRegistry *GetSettlementRegistry() const { return SettlementRegistry; }

    /** Get the persistent NPC registry for death tracking. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicPersistentNPCRegistry *GetPersistentNPCRegistry() const { return PersistentNPCRegistry; }

    /** Get the социал graph for NPC relationship queries. */
    UMythicSocialGraph *GetSocialGraph() const { return SocialGraph; }

    /** Get the scheme engine for faction scheme queries. */
    UMythicSchemeEngine *GetSchemeEngine() const { return SchemeEngine; }

    /** Is the living world system initialized and running? */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    bool IsSystemActive() const;

    // ─── Event Writing (from Game Thread) ─────────────────

    /**
     * Submit a world event from the game thread.
     * The event is buffered and will be committed to the fabric
     * on the next background thread tick.
     *
     * This is the primary entry point for gameplay systems to record
     * events (combat, crime, interaction, etc.) into the causal fabric.
     * C++ only — FMythicWorldEvent contains non-BP-compatible types.
     */
    void SubmitWorldEvent(const FMythicWorldEvent &Event);

    /**
     * Register a settlement actor with the system.
     * Called by AMythicSettlement::BeginPlay after spline rasterization.
     */
    void RegisterSettlement(AMythicSettlement *Settlement);

    /**
     * Transfer a settlement to a new faction. Thread-safe (locks simulation).
     * Updates territory control, cell counts, and events.
     */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    void TransferSettlement(int32 SettlementId, FMythicFactionId NewFaction);

    // ─── Save/Load ───────────────────────────────────────

    /**
     * Save the entire Living World state to an archive.
     * Pauses the simulation thread, serializes all systems, then resumes.
     *
     * Call this from your save game flow on the game thread.
     * All five core systems are serialized:
     * - CausalFabric (world event ring buffer)
     * - FactionDatabase (factions, ideology, resources, relationships)
     * - TerritoryGrid (spatial faction control)
     * - SchemeEngine (active faction schemes)
     * - PartySubsystem (companion loyalty and beliefs)
     */
    void SaveLivingWorld(FArchive& Ar);

    /**
     * Load the Living World state from an archive.
     * Pauses the simulation thread, deserializes all systems, then resumes.
     *
     * Prerequisites: Initialize() must have been called first (systems created).
     */
    void LoadLivingWorld(FArchive& Ar);

private:
    /** Load and validate the settings data asset */
    bool LoadSettings();

    /** Create and initialize all shared data objects */
    void InitializeSharedData();

    /** Start the background simulation thread */
    void StartSimulation();

    /** Stop the background simulation thread */
    void StopSimulation();

    /** Seed territory grid from all registered settlements */
    void SeedTerritoryFromSettlements();

    /** Callback when the background thread completes a commit */
    void OnSimCommitted();

    // ─── Owned Data ───────────────────────────────────────

    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSettings> Settings;

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UPROPERTY()
    TObjectPtr<UMythicTerritoryGrid> TerritoryGrid;

    /** Background sim thread — NOT a UObject, manually owned */
    TUniquePtr<FMythicWorldSimThread> SimThread;

    /** Internal cache for Faction DB settings to prevent GC */
    UPROPERTY()
    TObjectPtr<const UMythicFactionDatabaseSettings> FactionConfig;

    /** Internal cache for Territory Grid settings to prevent GC */
    UPROPERTY()
    TObjectPtr<const UMythicTerritoryGridSettings> TerritoryConfig;

    UPROPERTY()
    TObjectPtr<UMythicSettlementRegistry> SettlementRegistry;

    UPROPERTY()
    TObjectPtr<UMythicPersistentNPCRegistry> PersistentNPCRegistry;

    /** Social graph — NPC-to-NPC relationship adjacency list */
    UPROPERTY()
    TObjectPtr<UMythicSocialGraph> SocialGraph;

    /** Scheme engine — background-thread faction scheme generation/progression */
    UPROPERTY()
    TObjectPtr<UMythicSchemeEngine> SchemeEngine;

    /** Server->Client Replication Manager (spawned on server only) */
    UPROPERTY()
    TObjectPtr<class AMythicLivingWorldReplicator> Replicator;

    /** Events submitted from game thread, pending batch write by background thread */
    TArray<FMythicWorldEvent> PendingEvents;

    /** Sync primitive for pending events buffer (game thread writes, background thread reads) */
    FCriticalSection PendingEventsMutex;

    /** Global lock for simulation state (Write Buffers). Held by SimThread during tick, GameThread during writes. */
    FCriticalSection SimulationLock;
};
