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

    UPROPERTY()
    TObjectPtr<UMythicSettlementRegistry> SettlementRegistry;

    /** Events submitted from game thread, pending batch write by background thread */
    TArray<FMythicWorldEvent> PendingEvents;

    /** Sync primitive for pending events buffer (game thread writes, background thread reads) */
    FCriticalSection PendingEventsMutex;
};
