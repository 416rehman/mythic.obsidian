// Mythic Living World System — Central Subsystem Implementation

#include "World/LivingWorld/LivingWorldSubsystem.h"
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
    StartSimulation();

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
    if (UWorld* World = GetWorld()) {
        if (World->GetNetMode() != NM_Client) {
            FActorSpawnParameters SpawnParams;
            SpawnParams.Name = FName("MythicLivingWorldReplicator");
            Replicator = World->SpawnActor<AMythicLivingWorldReplicator>(SpawnParams);
        }
    }
}

void UMythicLivingWorldSubsystem::StartSimulation() {
    SimThread = MakeUnique<FMythicWorldSimThread>();
    SimThread->Setup(CausalFabric, FactionDB, TerritoryGrid, SettlementRegistry, Settings, Settings->SimTickIntervalSeconds, &SimulationLock, SchemeEngine, &PendingEvents, &PendingEventsMutex);
    SimThread->OnWorldSimCommitted.AddUObject(this, &UMythicLivingWorldSubsystem::OnSimCommitted);
    SimThread->StartThread();
}

void UMythicLivingWorldSubsystem::OnSimCommitted() {
    if (Replicator) {
        // Dispatch to the game thread to safely sync proxies
        TWeakObjectPtr<UMythicLivingWorldSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (UMythicLivingWorldSubsystem* StrongThis = WeakThis.Get()) {
                if (StrongThis->Replicator) {
                    StrongThis->Replicator->SyncProxies(StrongThis);
                }
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

void UMythicLivingWorldSubsystem::SeedTerritoryFromSettlements() {
    if (!SettlementRegistry || !TerritoryGrid || !FactionDB) {
        UE_LOG(LogMythLivingWorld, Warning, TEXT("Cannot seed territory: missing registry, grid, or faction DB."));
        return;
    }

    {
        FScopeLock Lock(&SimulationLock); // Protect WriteBuffer access
        SettlementRegistry->SeedTerritoryFromSettlements(TerritoryGrid, FactionDB);
    }

    // Commit the seeded data so the sim thread and game thread see it
    TerritoryGrid->CommitWrites();
    FactionDB->CommitWrites();

    UE_LOG(LogMythLivingWorld, Log, TEXT("Territory seeding complete."));
}

void UMythicLivingWorldSubsystem::SaveLivingWorld(FArchive& Ar) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicLivingWorld_Save);

    // Master version header for the entire Living World save format
    int32 MasterVersion = 1;
    Ar << MasterVersion;

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

    // PartySubsystem is a WorldSubsystem, not owned by us.
    // Access it through the current world context.
    if (UWorld* World = GetGameInstance()->GetWorld()) {
        if (UMythicPartySubsystem* Party = World->GetSubsystem<UMythicPartySubsystem>()) {
            Party->Serialize(Ar);
        }
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World state saved successfully."));
}

void UMythicLivingWorldSubsystem::LoadLivingWorld(FArchive& Ar) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicLivingWorld_Load);

    int32 MasterVersion = 0;
    Ar << MasterVersion;

    if (MasterVersion != 1) {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Unsupported Living World save version: %d"), MasterVersion);
        return;
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

    if (UWorld* World = GetGameInstance()->GetWorld()) {
        if (UMythicPartySubsystem* Party = World->GetSubsystem<UMythicPartySubsystem>()) {
            Party->Serialize(Ar);
        }
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World state loaded successfully."));
}

