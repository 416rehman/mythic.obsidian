// Mythic Living World System — Central Subsystem
// Manages the living world lifecycle: initialization, shared data, background thread.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "LivingWorldSubsystem.generated.h"

/** Fired (client-side) when the replicated faction/territory proxies change — for UI to refresh. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FMythicOnLivingWorldProxiesChanged);

class AMythicNPCCharacter;
class UMythicLivingWorldSettings;
class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicTerritoryGrid;
class UMythicSettlementRegistry;
struct FMythicSettlementData;
class AMythicSettlement;
class UMythicPersistentNPCRegistry;
class UMythicDesignerSpawnerRegistry;
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

    // ─── Client-replicated Living World proxies (faction wealth/status/population, territory control) ───
    // The server sim isn't run on clients; these accessors read the replicated proxy snapshot instead. They work
    // on the server too (the Replicator holds the same data). All return empty/null if no Replicator is present.

    /** CLIENT-side: fired when the replicated faction/territory proxies change. UI binds this to refresh. */
    UPROPERTY(BlueprintAssignable, Category = "Living World")
    FMythicOnLivingWorldProxiesChanged OnLivingWorldProxiesChanged;

    /** Called by AMythicLivingWorldReplicator::BeginPlay on the CLIENT to link itself (server sets it at spawn).
     *  Pass nullptr on the replicator's EndPlay to unlink. Broadcasts OnLivingWorldProxiesChanged on link. */
    void RegisterClientReplicator(AMythicLivingWorldReplicator *InReplicator);

    /** The replicated faction proxy for a faction (wealth/status/population/strength), or null if not replicated. */
    const FMythicFactionProxyItem *GetFactionProxy(FMythicFactionId FactionId) const;

    /** The replicated territory proxy for a cell (controlling faction). Returns false if that cell isn't synced. */
    bool GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const;

    /** All currently-replicated faction proxies (active factions). Empty if no Replicator. */
    const TArray<FMythicFactionProxyItem> &GetAllFactionProxies() const;

    /** All currently-replicated active encounters (type/state/cell/faction — for a client map/HUD). Empty if no
     *  Replicator. Client reads the replicated snapshot since the EncounterDirector is server-only. */
    const TArray<FMythicEncounterProxyItem> &GetAllEncounterProxies() const;

    /** All currently-replicated territory proxies (changed/owned cells). Empty if no Replicator. The war-map subsystem
     *  reads these (delta-accumulated) to build the client territory texture. */
    const TArray<FMythicTerritoryProxyItem> &GetAllTerritoryProxies() const;

    /** All currently-replicated settlements (center cell / governing faction / name / capital — for the client
     *  war-map markers). Empty if no Replicator. */
    const TArray<FMythicSettlementProxyItem> &GetAllSettlementProxies() const;

    // ─── Blueprint-facing proxy reads ───
    // BlueprintPure wrappers over the C++ accessors above (which return pointers / const refs UHT can't reflect).
    // UMG binds these + OnLivingWorldProxiesChanged to drive the faction panel / territory map / encounter markers.

    /** BP: the replicated faction proxy for a faction. Returns false if that faction isn't currently replicated. */
    UFUNCTION(BlueprintPure, Category = "Living World", meta = (DisplayName = "Get Faction Proxy"))
    bool K2_GetFactionProxy(FMythicFactionId FactionId, FMythicFactionProxyItem &OutProxy) const;

    /** BP: all currently-replicated faction proxies (active factions). */
    UFUNCTION(BlueprintPure, Category = "Living World", meta = (DisplayName = "Get All Faction Proxies"))
    TArray<FMythicFactionProxyItem> K2_GetAllFactionProxies() const;

    /** BP: the replicated territory proxy (controlling faction) for a cell. Returns false if that cell isn't synced. */
    UFUNCTION(BlueprintPure, Category = "Living World", meta = (DisplayName = "Get Territory Proxy"))
    bool K2_GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const;

    /** BP: all currently-replicated active encounters (type/state/cell/faction — for a client map/HUD). */
    UFUNCTION(BlueprintPure, Category = "Living World", meta = (DisplayName = "Get All Encounter Proxies"))
    TArray<FMythicEncounterProxyItem> K2_GetAllEncounterProxies() const;

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

    /** Get the designer-spawner registry (per-DesignerId SpawnsEver / perma-death counters). Server-mutated only. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicDesignerSpawnerRegistry *GetDesignerSpawnerRegistry() const { return DesignerSpawnerRegistry; }

    /** Get the социал graph for NPC relationship queries. */
    UMythicSocialGraph *GetSocialGraph() const { return SocialGraph; }

    /** Get the scheme engine for faction scheme queries. */
    UMythicSchemeEngine *GetSchemeEngine() const { return SchemeEngine; }

    /** Is the living world system initialized and running? */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    bool IsSystemActive() const;

    // Fired on the GAME THREAD after every background sim commit (the same point that re-syncs the replicated
    // proxies). Game-thread readers (e.g. the World Chronicle) can safely query the CausalFabric here. Native
    // (not dynamic) — bound in C++ by UMythicWorldChronicleSubsystem.
    DECLARE_MULTICAST_DELEGATE(FMythicOnWorldSimCommitted);
    FMythicOnWorldSimCommitted OnWorldSimCommitted;

    // ─── MASS->actor bridge: entity<->embodied-actor reverse link (server-only) ───
    // Populated by the actor-spawn bridge when an entity is embodied, used to find the actor for despawn on
    // de-promotion. Not a UPROPERTY (FMassEntityHandle isn't a UPROPERTY-compatible key; TWeakObjectPtr already
    // guards dangling).
    void RegisterEmbodiedActor(FMassEntityHandle Entity, AMythicNPCCharacter *Actor);
    void UnregisterEmbodiedActor(FMassEntityHandle Entity);
    AMythicNPCCharacter *FindEmbodiedActor(FMassEntityHandle Entity) const;

    // ─── Embodiment actor pool (embodiment-service-LOCK-v1 §1) ───
    // A bounded, class-keyed reuse pool for embodied NPC/creature actors, so the ActorSpawnProcessor recycles bodies
    // instead of SpawnActor/Destroy churning them every time the proximity/significance set shifts. GAME-THREAD ONLY
    // (the spawn/despawn processors are bRequiresGameThreadExecution=true and this subsystem is game-thread) — no lock.
    // Keyed by CONCRETE UClass so a creature class and a humanoid class never share a bucket (the two-query routing is
    // preserved at the pool layer). All three honor bEnableEmbodimentPooling + EmbodimentPoolMaxPerClass from settings.

    /**
     * Get an embodied actor of ActorClass at (Loc,Rot), reusing a parked one from the free list if available
     * (un-park + reposition + WakeFromPool) or SpawnActor'ing a fresh one on a miss. The CALLER still does
     * InitializeFromMassEntity + RegisterEmbodiedActor afterward (unchanged from the raw-spawn path). Returns null
     * only if the spawn genuinely fails (bad class / no world). Pooling disabled => always a fresh SpawnActor.
     */
    AMythicNPCCharacter *AcquireEmbodiedActor(UClass *ActorClass, const FVector &Loc, const FRotator &Rot);

    /**
     * Return Actor (embodied from Entity) to the pool: UnregisterEmbodiedActor(Entity) FIRST (a pool-return bypasses
     * EndPlay, which is where the reverse-link is normally cleared), then — if pooling is enabled and the actor's
     * class bucket is under cap — SleepToPool() + push onto the free list; otherwise Actor->Destroy(). Idempotent on a
     * null / already-pending-kill Actor.
     */
    void ReleaseEmbodiedActor(FMassEntityHandle Entity, AMythicNPCCharacter *Actor);

    /**
     * Pre-spawn Count actors of ActorClass, park them (SleepToPool), and push onto the free list — so the first town's
     * embodiment pays no cold-spawn hitch. Called once per resolved class at subsystem init. Respects the per-class cap
     * and the pooling kill-switch (no-op if pooling is disabled). Server-only (no parked actors on a pure client).
     */
    void WarmEmbodimentPool(UClass *ActorClass, int32 Count);

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

    /**
     * Nominate a leadership candidate for a faction. Thread-safe (locks simulation): this is a read-modify-write of
     * the faction WriteBuffer's leader fields, which the SIM thread also writes (succession vacancy clear +
     * AnnihilateFaction) under the same lock. Game-thread callers (the SignificanceProcessor) MUST route through here.
     */
    void ReportLeaderCandidate(FMythicFactionId FactionId, uint32 EntityId, float Score);

    /**
     * Feed an embodied NPC's perma-death back into the world simulation (kill → sim feedback). Called from the existing
     * death path (AMythicNPCCharacter::HandleNPCDeath) AFTER the PersistentNPCRegistry::RegisterDeath perma-death record,
     * so it composes with — never duplicates — the kill→standing + perma-death + role-succession contract.
     *
     * Durably decrements the governing faction's Population by Settings->KillPopulationLoss, and (only for armed roles —
     * Soldier/Guard) its Reserves.Arms by Settings->KillMilitaryArmsLoss. Population is delta-applied and Reserves.Arms is
     * the durable economic SOURCE the sim derives MilitaryStrength from, so both edits persist; we deliberately do NOT
     * touch MilitaryStrength (recomputed absolutely each sim tick — a direct write is silently wiped).
     *
     * Thread-safe (locks SimulationLock): this is a read-modify-write of the faction WriteBuffer + CommitWrites, which the
     * sim thread also writes under this same lock (TickFactionEvolution births/deaths, economy recompute). No-op if the
     * faction id is invalid (factionless NPC), so no call-site guard is needed.
     */
    void ReportNpcDeath(FMythicFactionId FactionId, FGameplayTag RoleTag);

    /**
     * Process role-vacation / shop-succession for a dead NPC. Thread-safe (locks simulation): SettlementRegistry's
     * HandleNPCDeath WALKS the Settlements TMap, which the SIM thread concurrently rehashes/mutates under the same lock
     * (RegisterSettlement Add, TickShopSuccession, TransferSettlement). Game-thread death callers MUST route here.
     */
    void HandleNPCDeathSettlements(uint32 NameHash, double WorldTime);

    /**
     * Thread-safe copy-out of the settlement at a cell. Takes SimulationLock and copies the FMythicSettlementData BY
     * VALUE — GetSettlementAtCell returns a raw pointer into the live Settlements TMap, which the SIM thread mutates
     * (TransferSettlement writes GoverningFaction; TickShopSuccession; RegisterSettlement rehashes) under this same lock.
     * Game-thread readers (the population spawner) MUST snapshot through here, not hold the live pointer.
     * @return true + fills Out if a settlement governs the cell; false otherwise.
     */
    bool CopySettlementAtCell(const FMythicCellCoord &Cell, FMythicSettlementData &Out);

    /**
     * Thread-safe copy-out of the settlement with a given runtime SettlementId (the by-ID sibling of CopySettlementAtCell).
     * Takes SimulationLock + copies BY VALUE — GetSettlementData returns a raw pointer into the live Settlements TMap.
     * @return true + fills Out if the id exists; false otherwise.
     */
    bool CopySettlementById(int32 SettlementId, FMythicSettlementData &Out);

    /**
     * Thread-safe enumeration of all registered settlement IDs. Takes SimulationLock before GetAllSettlementIds, which
     * does a bare Settlements.GetKeys walk — the sim thread rehashes/mutates that TMap under this same lock
     * (RegisterSettlement Add, TransferSettlement, TickShopSuccession). Game-thread enumerators (the debugger) MUST
     * route through here, not call the registry's unlocked getter directly.
     */
    void CopyAllSettlementIds(TArray<int32> &OutIds);

    /**
     * Thread-safe settlement count. Takes SimulationLock before GetSettlementCount (a bare Settlements.Num() read),
     * for the same reason as CopyAllSettlementIds.
     */
    int32 GetSettlementCountSafe();

    /**
     * Thread-safe resolve of a settlement's actor by runtime SettlementId. Takes SimulationLock before
     * GetSettlementActor (a bare SettlementActors.Find), which the sim thread rehashes under this same lock.
     * @return the actor, or nullptr if the id is unknown or the actor was destroyed.
     */
    AMythicSettlement *GetSettlementActorSafe(int32 SettlementId);

    /**
     * Thread-safe copy-out of the background sim thread's tick diagnostics. Takes SimulationLock before reading
     * TickCount (mutated on the background thread inside SimTick under this same lock) — a lock-free read of that
     * non-atomic counter would be a data race. Added for the Living World gameplay debugger (World State pane).
     * @return true + fills the outs if a sim thread exists; false (and OutRunning=false) if none.
     */
    bool CopySimDiagnostics(uint64 &OutTickCount, float &OutTickIntervalSeconds, bool &OutRunning);

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
    void SaveLivingWorld(FArchive &Ar);

    /**
     * Load the Living World state from an archive.
     * Pauses the simulation thread, deserializes all systems, then resumes.
     *
     * Prerequisites: Initialize() must have been called first (systems created).
     */
    void LoadLivingWorld(FArchive &Ar);

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

    /** Resolve the embodied humanoid + creature classes from settings (mirroring the ActorSpawnProcessor's resolve
     *  order) and pre-warm EmbodimentPoolWarmCount parked actors of each at init, so the first town has no cold-spawn
     *  hitch. Server-only; no-op if pooling is disabled. Called once from Initialize after the sim is started. */
    void WarmEmbodimentPools();

    /** Callback when the background thread completes a commit */
    void OnSimCommitted();

    /** Reverse link entity->embodied cognitive actor; server-only, populated by the MASS->actor bridge. */
    TMap<FMassEntityHandle, TWeakObjectPtr<AMythicNPCCharacter>> EmbodiedActors;

    /** Class-keyed free list of PARKED (hidden, non-colliding, non-ticking, brain-stopped) embodied actors available
     *  for reuse. Game-thread only, no lock. The key TObjectPtr<UClass> keeps the class alive; the values are weak so a
     *  parked actor destroyed out-from-under us (level teardown) is skipped rather than handed out dangling. Not a
     *  UPROPERTY (the value is a TArray of weak ptrs, which a UPROPERTY can't reflect) — that is fine: parked actors are
     *  hidden, not unreferenced, so they are not GC candidates, and the weak guard covers the teardown edge anyway. */
    TMap<TObjectPtr<UClass>, TArray<TWeakObjectPtr<AMythicNPCCharacter>>> EmbodimentPool;

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

    /** Designer-spawner persistence registry (per-DesignerId SpawnsEver / perma-death / last-death). Server-mutated only. */
    UPROPERTY()
    TObjectPtr<UMythicDesignerSpawnerRegistry> DesignerSpawnerRegistry;

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
