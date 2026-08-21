
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/LivingWorld/MythicPlayerEconomyDelta.h"
#include "LivingWorldSubsystem.generated.h"

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

UCLASS()
class MYTHIC_API UMythicLivingWorldSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

public:

    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    virtual ~UMythicLivingWorldSubsystem() override;


    /** Get the causal fabric for event queries. Lock-free read. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicCausalFabric *GetCausalFabric() const { return CausalFabric; }

    /** Get the faction database for faction queries. Lock-free read. */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    UMythicFactionDatabase *GetFactionDatabase() const { return FactionDB; }

    bool IsAnyFactionInFamine() const;


    /** CLIENT-side: fired when the replicated faction/territory proxies change. UI binds this to refresh. */
    UPROPERTY(BlueprintAssignable, Category = "Living World")
    FMythicOnLivingWorldProxiesChanged OnLivingWorldProxiesChanged;

    void RegisterClientReplicator(AMythicLivingWorldReplicator *InReplicator);

    const FMythicFactionProxyItem *GetFactionProxy(FMythicFactionId FactionId) const;

    bool GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const;

    const TArray<FMythicFactionProxyItem> &GetAllFactionProxies() const;

    const TArray<FMythicEncounterProxyItem> &GetAllEncounterProxies() const;

    const TArray<FMythicTerritoryProxyItem> &GetAllTerritoryProxies() const;

    const TArray<FMythicSettlementProxyItem> &GetAllSettlementProxies() const;


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

    UMythicSocialGraph *GetSocialGraph() const { return SocialGraph; }

    UMythicSchemeEngine *GetSchemeEngine() const { return SchemeEngine; }

    /** Is the living world system initialized and running? */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    bool IsSystemActive() const;

    DECLARE_MULTICAST_DELEGATE(FMythicOnWorldSimCommitted);
    FMythicOnWorldSimCommitted OnWorldSimCommitted;

    void RegisterEmbodiedActor(FMassEntityHandle Entity, AMythicNPCCharacter *Actor);
    void UnregisterEmbodiedActor(FMassEntityHandle Entity);
    AMythicNPCCharacter *FindEmbodiedActor(FMassEntityHandle Entity) const;


    AMythicNPCCharacter *AcquireEmbodiedActor(UClass *ActorClass, const FVector &Loc, const FRotator &Rot);

    void ReleaseEmbodiedActor(FMassEntityHandle Entity, AMythicNPCCharacter *Actor);

    void WarmEmbodimentPool(UClass *ActorClass, int32 Count);


    void SubmitWorldEvent(const FMythicWorldEvent &Event);

    void RegisterSettlement(AMythicSettlement *Settlement);

    /**
     * Transfer a settlement to a new faction. Thread-safe (locks simulation).
     * Updates territory control, cell counts, and events.
     */
    UFUNCTION(BlueprintCallable, Category = "Living World")
    void TransferSettlement(int32 SettlementId, FMythicFactionId NewFaction);

    void ReportLeaderCandidate(FMythicFactionId FactionId, uint32 EntityId, float Score);

    void ReportNpcDeath(FMythicFactionId FactionId, FGameplayTag RoleTag);

    void EnqueuePlayerResourceDelta(FMythicFactionId FactionId, EMythicResourceType Axis, float Delta);

    void HandleNPCDeathSettlements(uint32 NameHash, double WorldTime);

    bool CopySettlementAtCell(const FMythicCellCoord &Cell, FMythicSettlementData &Out);

    bool CopySettlementById(int32 SettlementId, FMythicSettlementData &Out);

    void CopyAllSettlementIds(TArray<int32> &OutIds);

    int32 GetSettlementCountSafe();

    AMythicSettlement *GetSettlementActorSafe(int32 SettlementId);

    bool CopySimDiagnostics(uint64 &OutTickCount, float &OutTickIntervalSeconds, bool &OutRunning);


    void SaveLivingWorld(FArchive &Ar);

    void LoadLivingWorld(FArchive &Ar);

private:
    bool LoadSettings();

    void InitializeSharedData();

    void StartSimulation();

    void StopSimulation();

    void SeedTerritoryFromSettlements();

    void WarmEmbodimentPools();

    void OnSimCommitted();

    void DrainPlayerResourceDeltas();

    void ApplySettlementSanitationPressure();

    TMap<FMassEntityHandle, TWeakObjectPtr<AMythicNPCCharacter>> EmbodiedActors;

    TMap<TObjectPtr<UClass>, TArray<TWeakObjectPtr<AMythicNPCCharacter>>> EmbodimentPool;


    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSettings> Settings;

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UPROPERTY()
    TObjectPtr<UMythicTerritoryGrid> TerritoryGrid;

    TUniquePtr<FMythicWorldSimThread> SimThread;

    UPROPERTY()
    TObjectPtr<const UMythicFactionDatabaseSettings> FactionConfig;

    UPROPERTY()
    TObjectPtr<const UMythicTerritoryGridSettings> TerritoryConfig;

    UPROPERTY()
    TObjectPtr<UMythicSettlementRegistry> SettlementRegistry;

    UPROPERTY()
    TObjectPtr<UMythicPersistentNPCRegistry> PersistentNPCRegistry;

    UPROPERTY()
    TObjectPtr<UMythicDesignerSpawnerRegistry> DesignerSpawnerRegistry;

    UPROPERTY()
    TObjectPtr<UMythicSocialGraph> SocialGraph;

    UPROPERTY()
    TObjectPtr<UMythicSchemeEngine> SchemeEngine;

    UPROPERTY()
    TObjectPtr<class AMythicLivingWorldReplicator> Replicator;

    TArray<FMythicWorldEvent> PendingEvents;

    FCriticalSection PendingEventsMutex;

    TArray<FMythicPendingResourceDelta> PendingPlayerResourceDeltas;

    FCriticalSection PendingPlayerResourceDeltasMutex;

    FCriticalSection SimulationLock;
};
