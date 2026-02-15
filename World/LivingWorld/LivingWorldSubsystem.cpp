// Mythic Living World System — Central Subsystem Implementation

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Engine/AssetManager.h"

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
    Settings = nullptr;

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
    const UMythicFactionDatabaseSettings *FactionSettings = Settings->FactionSettings.LoadSynchronous();
    if (FactionSettings) {
        FactionDB = NewObject<UMythicFactionDatabase>(this);
        FactionDB->Initialize(FactionSettings);
    }
    else {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Faction Database Settings not loaded. Factions will not function."));
    }

    // Create Territory Grid
    const UMythicTerritoryGridSettings *GridSettings = Settings->TerritorySettings.LoadSynchronous();
    if (GridSettings) {
        TerritoryGrid = NewObject<UMythicTerritoryGrid>(this);
        TerritoryGrid->Initialize(GridSettings);
    }
    else {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Territory Grid Settings not loaded. Territory system will not function."));
    }

    // Create Settlement Registry
    SettlementRegistry = NewObject<UMythicSettlementRegistry>(this);
}

void UMythicLivingWorldSubsystem::StartSimulation() {
    SimThread = MakeUnique<FMythicWorldSimThread>();
    SimThread->Setup(CausalFabric, FactionDB, TerritoryGrid, Settings, Settings->SimTickIntervalSeconds);
    SimThread->StartThread();
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

    SettlementRegistry->SeedTerritoryFromSettlements(TerritoryGrid, FactionDB);

    // Commit the seeded data so the sim thread and game thread see it
    TerritoryGrid->CommitWrites();
    FactionDB->CommitWrites();

    UE_LOG(LogMythLivingWorld, Log, TEXT("Territory seeding complete."));
}
