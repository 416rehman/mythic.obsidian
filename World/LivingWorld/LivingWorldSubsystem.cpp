// Mythic Living World System — Central Subsystem Implementation

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "Settings/MythicDeveloperSettings.h"
#include "AI/Party/PartySubsystem.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "Async/Async.h"

bool UMythicLivingWorldSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    return true;
}

UMythicLivingWorldSubsystem::~UMythicLivingWorldSubsystem() {
    StopSimulation();
}

void UMythicLivingWorldSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World Subsystem initializing..."));

    if (!LoadSettings()) {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Failed to load Living World settings. System will not start."));
        return;
    }

    InitializeSharedData();

    // CLIENT GATE: the living-world simulation is authoritative. On a true client the sim must NOT run — a per-client
    // background thread would diverge from the server and (via OnWorldSimCommitted) pollute the player-facing chronicle
    // that the COND_OwnerOnly relay fills from authority. Clients consume living-world state only through the replicated
    // proxies (AMythicLivingWorldReplicator) and the chronicle relay; no client system reads the local sim/fabric.
    // InitializeSharedData() above is intentionally still run on clients (they hold the empty shared-data objects so the
    // server-shaped accessors stay non-null; the Replicator spawn inside it is already gated to non-NM_Client).
    // A GameInstanceSubsystem has no HasAuthority(), so gate on the world's net mode (NM_Client). NM_ListenServer and
    // NM_Standalone are NOT NM_Client, so listen-server hosts and single-player still run the sim.
    const UWorld *World = GetWorld();
    if (World && World->GetNetMode() == NM_Client) {
        UE_LOG(LogMythLivingWorld, Log,
               TEXT("Living World Subsystem on a client — skipping local simulation (sim is authority-only; client reads replicated proxies + chronicle relay)."
               ));
    }
    else {
        StartSimulation();
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World Subsystem initialized successfully."));
}

void UMythicLivingWorldSubsystem::Deinitialize() {
    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World Subsystem deinitializing..."));

    StopSimulation();

    CausalFabric = nullptr;
    FactionDB = nullptr;
    TerritoryGrid = nullptr;
    SettlementRegistry = nullptr;
    SocialGraph = nullptr;
    SchemeEngine = nullptr;
    FactionConfig = nullptr;
    TerritoryConfig = nullptr;
    Settings = nullptr;

    if (IsValid(Replicator)) {
        Replicator->Destroy();
    }
    Replicator = nullptr;

    Super::Deinitialize();
}

bool UMythicLivingWorldSubsystem::IsSystemActive() const {
    return CausalFabric != nullptr
        && FactionDB != nullptr
        && TerritoryGrid != nullptr
        && SimThread.IsValid()
        && SimThread->IsRunning();
}

void UMythicLivingWorldSubsystem::RegisterEmbodiedActor(FMassEntityHandle Entity, AMythicNPCCharacter *Actor) {
    EmbodiedActors.Add(Entity, Actor);
}

void UMythicLivingWorldSubsystem::UnregisterEmbodiedActor(FMassEntityHandle Entity) {
    EmbodiedActors.Remove(Entity);
}

AMythicNPCCharacter *UMythicLivingWorldSubsystem::FindEmbodiedActor(FMassEntityHandle Entity) const {
    if (const TWeakObjectPtr<AMythicNPCCharacter> *Found = EmbodiedActors.Find(Entity)) {
        return Found->Get();
    }
    return nullptr;
}

void UMythicLivingWorldSubsystem::SubmitWorldEvent(const FMythicWorldEvent &Event) {
    FScopeLock Lock(&PendingEventsMutex);
    PendingEvents.Add(Event);
}

void UMythicLivingWorldSubsystem::RegisterSettlement(AMythicSettlement *Settlement) {
    if (!SettlementRegistry) {
        SettlementRegistry = NewObject<UMythicSettlementRegistry>(this);
    }

    FScopeLock Lock(&SimulationLock); // Protect WriteBuffer access

    const int32 SettlementId = SettlementRegistry->RegisterSettlement(Settlement);

    // Immediately seed this settlement's cells into the territory grid
    if (SettlementId != INDEX_NONE && TerritoryGrid && FactionDB) {
        const FMythicSettlementData *Data = SettlementRegistry->GetSettlementData(SettlementId);
        if (Data && Data->GoverningFaction.IsValid()) {
            for (const FMythicCellCoord &Cell : Data->RasterizedCells) {
                TerritoryGrid->SetCellInfluence(Cell, Data->GoverningFaction, 1.0f);
            }

            FMythicFactionData *FactionData = FactionDB->GetFactionMutable(Data->GoverningFaction);
            if (FactionData) {
                FactionData->ControlledCellCount += Data->RasterizedCells.Num();
            }

            // Commit so sim thread and read snapshots see the seeded data
            TerritoryGrid->CommitWrites();
            FactionDB->CommitWrites();

            UE_LOG(LogMythLivingWorld, Log, TEXT("Settlement '%s' seeded %d cells for faction %d."),
                   *Data->DisplayName.ToString(), Data->RasterizedCells.Num(), Data->GoverningFaction.Index);
        }
    }
}

void UMythicLivingWorldSubsystem::TransferSettlement(int32 SettlementId, FMythicFactionId NewFaction) {
    if (!SettlementRegistry || !TerritoryGrid || !FactionDB) {
        return;
    }

    FScopeLock Lock(&SimulationLock); // Protect WriteBuffer access
    SettlementRegistry->TransferSettlement(SettlementId, NewFaction, TerritoryGrid, FactionDB, CausalFabric);
}

void UMythicLivingWorldSubsystem::ReportLeaderCandidate(FMythicFactionId FactionId, uint32 EntityId, float Score) {
    if (!FactionDB) {
        return;
    }
    // Hold the simulation lock: ReportLeaderCandidate read-modify-writes the faction WriteBuffer's leader fields, which
    // the sim thread ALSO writes under this same lock (succession vacancy clear at WorldSimThread::SimTick +
    // AnnihilateFaction). Without the lock, the game-thread nomination races those writes + the CommitWrites snapshot.
    FScopeLock Lock(&SimulationLock);
    FactionDB->ReportLeaderCandidate(FactionId, EntityId, Score);
}

void UMythicLivingWorldSubsystem::HandleNPCDeathSettlements(uint32 NameHash, double WorldTime) {
    if (!SettlementRegistry) {
        return;
    }
    // Hold the simulation lock: HandleNPCDeath iterates the Settlements TMap (role vacation + shop succession), which the
    // sim thread concurrently rehashes/mutates under this same lock (RegisterSettlement Add, TickShopSuccession,
    // TransferSettlement). An unlocked game-thread walk here races a rehash → TMap use-after-free / torn shop reads.
    FScopeLock Lock(&SimulationLock);
    SettlementRegistry->HandleNPCDeath(NameHash, WorldTime);
}

bool UMythicLivingWorldSubsystem::CopySettlementAtCell(const FMythicCellCoord &Cell, FMythicSettlementData &Out) {
    if (!SettlementRegistry) {
        return false;
    }
    // Hold the simulation lock + COPY by value: GetSettlementAtCell returns a raw pointer into the live Settlements
    // TMap, whose fields the sim thread writes under this same lock (TransferSettlement→GoverningFaction,
    // TickShopSuccession) and whose buckets RegisterSettlement rehashes. A game-thread reader holding the live pointer
    // would tear a conquest-tick faction read (NPCs stamped to the wrong faction) or dangle across a rehash.
    FScopeLock Lock(&SimulationLock);
    if (const FMythicSettlementData *Found = SettlementRegistry->GetSettlementAtCell(Cell)) {
        Out = *Found;
        return true;
    }
    return false;
}

bool UMythicLivingWorldSubsystem::CopySettlementById(int32 SettlementId, FMythicSettlementData &Out) {
    if (!SettlementRegistry) {
        return false;
    }
    // Same SimulationLock copy-out as CopySettlementAtCell, keyed by runtime id: GetSettlementData returns a raw pointer
    // into the live Settlements TMap, which the sim thread mutates/rehashes under this lock.
    FScopeLock Lock(&SimulationLock);
    if (const FMythicSettlementData *Found = SettlementRegistry->GetSettlementData(SettlementId)) {
        Out = *Found;
        return true;
    }
    return false;
}

bool UMythicLivingWorldSubsystem::LoadSettings() {
    // Read the settings asset reference from Project Settings > Game > Mythic > Living World
    const UMythicDeveloperSettings *DevSettings = GetDefault<UMythicDeveloperSettings>();
    if (!DevSettings || DevSettings->LivingWorldSettings.IsNull()) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("Living World Settings not configured. Assign a UMythicLivingWorldSettings asset in Project Settings > Game > Mythic > Living World."));
        return false;
    }

    Settings = DevSettings->LivingWorldSettings.LoadSynchronous();

    if (!Settings) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("Failed to load Living World Settings asset '%s'. Verify the asset exists and is valid."),
               *DevSettings->LivingWorldSettings.ToString());
        return false;
    }

    return true;
}

void UMythicLivingWorldSubsystem::InitializeSharedData() {
    check(Settings);

    // Create Causal Fabric
    CausalFabric = NewObject<UMythicCausalFabric>(this);
    CausalFabric->Initialize(Settings->FabricCapacity);

    // Create Faction Database
    FactionConfig = Settings->FactionSettings.LoadSynchronous();
    if (FactionConfig) {
        FactionDB = NewObject<UMythicFactionDatabase>(this);
        FactionDB->Initialize(FactionConfig);
    }
    else {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Faction Database Settings not loaded. Factions will not function."));
    }

    // Create Territory Grid
    TerritoryConfig = Settings->TerritorySettings.LoadSynchronous();
    if (TerritoryConfig) {
        TerritoryGrid = NewObject<UMythicTerritoryGrid>(this);
        TerritoryGrid->Initialize(TerritoryConfig);
    }
    else {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Territory Grid Settings not loaded. Territory system will not function."));
    }

    // Create Settlement Registry
    SettlementRegistry = NewObject<UMythicSettlementRegistry>(this);

    // Create Persistent NPC Registry
    PersistentNPCRegistry = NewObject<UMythicPersistentNPCRegistry>(this);

    // Create Social Graph (Phase 5)
    SocialGraph = NewObject<UMythicSocialGraph>(this);
    SocialGraph->Initialize(
        Settings->SocialMaxEdgesPerEntity,
        Settings->SocialPruneStrengthThreshold,
        Settings->SocialEdgeDecayRate);

    // Create Scheme Engine (Phase 5)
    SchemeEngine = NewObject<UMythicSchemeEngine>(this);
    SchemeEngine->Initialize(FactionDB, CausalFabric, TerritoryGrid, Settings);

    // Spawn Networking Replicator (Server Only)
    if (UWorld *World = GetWorld()) {
        if (World->GetNetMode() != NM_Client) {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Name = FName("MythicLivingWorldReplicator");
            Replicator = World->SpawnActor<AMythicLivingWorldReplicator>(SpawnParams);
        }
    }
}

void UMythicLivingWorldSubsystem::StartSimulation() {
    SimThread = MakeUnique<FMythicWorldSimThread>();
    SimThread->Setup(CausalFabric, FactionDB, TerritoryGrid, SettlementRegistry, Settings, Settings->SimTickIntervalSeconds, &SimulationLock, SchemeEngine,
                     &PendingEvents, &PendingEventsMutex);
    SimThread->OnWorldSimCommitted.AddUObject(this, &UMythicLivingWorldSubsystem::OnSimCommitted);
    SimThread->StartThread();
}

void UMythicLivingWorldSubsystem::OnSimCommitted() {
    if (Replicator) {
        // Dispatch to the game thread to safely sync proxies
        TWeakObjectPtr<UMythicLivingWorldSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (UMythicLivingWorldSubsystem *StrongThis = WeakThis.Get()) {
                if (StrongThis->Replicator) {
                    StrongThis->Replicator->SyncProxies(StrongThis);
                }
                // Game-thread post-commit point: notify chronicle/other readers AFTER proxies are synced. Fires
                // even if there's no Replicator (dedicated logic) — see below.
                StrongThis->OnWorldSimCommitted.Broadcast();
            }
        });
    }
    else {
        // No replicator (e.g. standalone with no client proxies) — still surface the commit on the game thread so
        // game-thread readers (World Chronicle) get notified.
        TWeakObjectPtr<UMythicLivingWorldSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (UMythicLivingWorldSubsystem *StrongThis = WeakThis.Get()) {
                StrongThis->OnWorldSimCommitted.Broadcast();
            }
        });
    }
}

void UMythicLivingWorldSubsystem::StopSimulation() {
    if (SimThread.IsValid()) {
        SimThread->StopThread();
        SimThread.Reset();
    }
}

void UMythicLivingWorldSubsystem::RegisterClientReplicator(AMythicLivingWorldReplicator *InReplicator) {
    Replicator = InReplicator;
    // A fresh link (or unlink) means the client's proxy snapshot just (dis)appeared — let UI refresh.
    OnLivingWorldProxiesChanged.Broadcast();
}

const FMythicFactionProxyItem *UMythicLivingWorldSubsystem::GetFactionProxy(FMythicFactionId FactionId) const {
    return Replicator ? Replicator->GetFactionProxy(FactionId) : nullptr;
}

bool UMythicLivingWorldSubsystem::GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const {
    return Replicator ? Replicator->GetTerritoryProxy(Cell, OutProxy) : false;
}

const TArray<FMythicFactionProxyItem> &UMythicLivingWorldSubsystem::GetAllFactionProxies() const {
    static const TArray<FMythicFactionProxyItem> Empty;
    return Replicator ? Replicator->GetAllFactionProxies() : Empty;
}

const TArray<FMythicEncounterProxyItem> &UMythicLivingWorldSubsystem::GetAllEncounterProxies() const {
    static const TArray<FMythicEncounterProxyItem> Empty;
    return Replicator ? Replicator->GetAllEncounterProxies() : Empty;
}

// ─── Blueprint-facing proxy reads (thin BlueprintPure wrappers; single-source via the C++ accessors above) ───

bool UMythicLivingWorldSubsystem::K2_GetFactionProxy(FMythicFactionId FactionId, FMythicFactionProxyItem &OutProxy) const {
    if (const FMythicFactionProxyItem *Proxy = GetFactionProxy(FactionId)) {
        OutProxy = *Proxy;
        return true;
    }
    return false;
}

TArray<FMythicFactionProxyItem> UMythicLivingWorldSubsystem::K2_GetAllFactionProxies() const {
    return GetAllFactionProxies();
}

bool UMythicLivingWorldSubsystem::K2_GetTerritoryProxy(FMythicCellCoord Cell, FMythicTerritoryProxyItem &OutProxy) const {
    return GetTerritoryProxy(Cell, OutProxy);
}

TArray<FMythicEncounterProxyItem> UMythicLivingWorldSubsystem::K2_GetAllEncounterProxies() const {
    return GetAllEncounterProxies();
}

void UMythicLivingWorldSubsystem::SeedTerritoryFromSettlements() {
    if (!SettlementRegistry || !TerritoryGrid || !FactionDB) {
        UE_LOG(LogMythLivingWorld, Warning, TEXT("Cannot seed territory: missing registry, grid, or faction DB."));
        return;
    }

    {
        FScopeLock Lock(&SimulationLock); // Protect WriteBuffer access
        SettlementRegistry->SeedTerritoryFromSettlements(TerritoryGrid, FactionDB);

        // Commit INSIDE the lock so the full-buffer snapshot copy (ReadFactions = WriteFactions) can't race a sim-thread
        // WriteFactions mutation. Same lock ordering the sim thread uses (SimulationLock, then each object's SnapshotLock
        // inside CommitWrites), so no inversion. Mirrors RegisterSettlement; the earlier escape was unintended.
        TerritoryGrid->CommitWrites();
        FactionDB->CommitWrites();
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Territory seeding complete."));
}

void UMythicLivingWorldSubsystem::SaveLivingWorld(FArchive &Ar) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicLivingWorld_Save);

    // Master version header for the entire Living World save format.
    // v2 adds a section-presence bitmask: the blob is unframed and its sections are written conditionally (FactionDB and
    // TerritoryGrid are config-gated, so they can be absent), so a load whose optional-subsystem set differs from the
    // save's would read every later section from the wrong byte offset. The mask lets the load detect that and fail safe.
    int32 MasterVersion = 2;
    Ar << MasterVersion;

    // Section-presence bitmask — one bit per subsystem, in the SAME order they are serialized below.
    UMythicPartySubsystem *PartySubsystemForSave = nullptr;
    if (UWorld *World = GetGameInstance()->GetWorld()) {
        PartySubsystemForSave = World->GetSubsystem<UMythicPartySubsystem>();
    }
    int32 SectionMask = 0;
    if (CausalFabric) { SectionMask |= (1 << 0); }
    if (FactionDB) { SectionMask |= (1 << 1); }
    if (TerritoryGrid) { SectionMask |= (1 << 2); }
    if (SchemeEngine) { SectionMask |= (1 << 3); }
    if (PersistentNPCRegistry) { SectionMask |= (1 << 4); }
    if (SettlementRegistry) { SectionMask |= (1 << 5); }
    if (PartySubsystemForSave) { SectionMask |= (1 << 6); }
    Ar << SectionMask;

    // Pause the simulation thread to get a consistent snapshot
    // SimulationLock prevents the background thread from writing during serialization
    FScopeLock Lock(&SimulationLock);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Saving Living World state..."));

    // Serialize all systems in deterministic order
    if (CausalFabric) {
        CausalFabric->Serialize(Ar);
    }

    if (FactionDB) {
        FactionDB->Serialize(Ar);
    }

    if (TerritoryGrid) {
        TerritoryGrid->Serialize(Ar);
    }

    if (SchemeEngine) {
        SchemeEngine->Serialize(Ar);
    }

    if (PersistentNPCRegistry) {
        PersistentNPCRegistry->Serialize(Ar);
    }

    if (SettlementRegistry) {
        SettlementRegistry->Serialize(Ar); // persists conquest-mutated GoverningFaction (was lost on reload)
    }

    // PartySubsystem is a WorldSubsystem, not owned by us.
    // Access it through the current world context.
    if (UWorld *World = GetGameInstance()->GetWorld()) {
        if (UMythicPartySubsystem *Party = World->GetSubsystem<UMythicPartySubsystem>()) {
            Party->Serialize(Ar);
        }
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World state saved successfully."));
}

void UMythicLivingWorldSubsystem::LoadLivingWorld(FArchive &Ar) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicLivingWorld_Load);

    int32 MasterVersion = 0;
    Ar << MasterVersion;

    if (MasterVersion != 1 && MasterVersion != 2) {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Unsupported Living World save version: %d"), MasterVersion);
        return;
    }

    // v2: validate the section-presence bitmask against the LIVE subsystem set. If they differ (e.g. a config asset was
    // renamed/removed so FactionDB/TerritoryGrid is now absent, or now resolves when it didn't), the unframed stream
    // cannot be read in alignment — abort rather than silently corrupt every subsequent section. (v1 had no mask.)
    if (MasterVersion >= 2) {
        int32 SectionMask = 0;
        Ar << SectionMask;

        UMythicPartySubsystem *PartySubsystemForLoad = nullptr;
        if (UWorld *World = GetGameInstance()->GetWorld()) {
            PartySubsystemForLoad = World->GetSubsystem<UMythicPartySubsystem>();
        }
        int32 LiveMask = 0;
        if (CausalFabric) { LiveMask |= (1 << 0); }
        if (FactionDB) { LiveMask |= (1 << 1); }
        if (TerritoryGrid) { LiveMask |= (1 << 2); }
        if (SchemeEngine) { LiveMask |= (1 << 3); }
        if (PersistentNPCRegistry) { LiveMask |= (1 << 4); }
        if (SettlementRegistry) { LiveMask |= (1 << 5); }
        if (PartySubsystemForLoad) { LiveMask |= (1 << 6); }

        if (SectionMask != LiveMask) {
            UE_LOG(LogMythLivingWorld, Error,
                   TEXT("Living World load aborted: saved section set (0x%X) differs from live (0x%X) — cannot align the unframed save stream."),
                   SectionMask, LiveMask);
            return;
        }
    }

    // Pause the simulation thread during deserialization
    FScopeLock Lock(&SimulationLock);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Loading Living World state..."));

    // Deserialize in the same order as save
    if (CausalFabric) {
        CausalFabric->Serialize(Ar);
    }

    if (FactionDB) {
        FactionDB->Serialize(Ar);
    }

    if (TerritoryGrid) {
        TerritoryGrid->Serialize(Ar);
    }

    if (SchemeEngine) {
        SchemeEngine->Serialize(Ar);
    }

    if (PersistentNPCRegistry) {
        PersistentNPCRegistry->Serialize(Ar);
    }

    if (SettlementRegistry) {
        SettlementRegistry->Serialize(Ar); // restores conquest-mutated GoverningFaction (was lost on reload)
    }

    if (UWorld *World = GetGameInstance()->GetWorld()) {
        if (UMythicPartySubsystem *Party = World->GetSubsystem<UMythicPartySubsystem>()) {
            Party->Serialize(Ar);
        }
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World state loaded successfully."));
}
