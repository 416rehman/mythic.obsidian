
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/Creatures/MythicCreatureCharacter.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/Death/MythicCorpseHazardSubsystem.h"
#include "World/LivingWorld/Settlements/SettlementRegistry.h"
#include "World/LivingWorld/Simulation/WorldSimThread.h"
#include "World/LivingWorld/Social/SocialGraph.h"
#include "World/LivingWorld/Simulation/SchemeEngine.h"
#include "World/LivingWorld/Persistence/PersistentNPCRegistry.h"
#include "World/LivingWorld/Spawn/DesignerSpawnerRegistry.h"
#include "World/LivingWorld/Spawn/MythicDesignerSpawner.h"
#include "World/LivingWorld/Encounters/EncounterDirector.h"
#include "Settings/MythicDeveloperSettings.h"
#include "AI/Party/PartySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "World/Entity/MythicEntityViewerKnowledgeComponent.h"
#include "World/LivingWorld/LivingWorldReplication.h"
#include "World/Entity/MythicEntityPresentationComponent.h"
#include "World/Entity/MythicEntityPresentationRegistry.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "MassEntitySubsystem.h"
#include "MassEntityQuery.h"
#include "MassExecutionContext.h"
#include "Async/Async.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"

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

    const UWorld *World = GetWorld();
    if (World && World->GetNetMode() == NM_Client) {
        UE_LOG(LogMythLivingWorld, Log,
               TEXT("Living World Subsystem on a client — skipping local simulation (sim is authority-only; client reads replicated proxies + chronicle relay)."
               ));
    }
    else {
        StartSimulation();

        WarmEmbodimentPools();
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World Subsystem initialized successfully."));
}

void UMythicLivingWorldSubsystem::WarmEmbodimentPools() {
    if (!Settings || !Settings->bEnableEmbodimentPooling || Settings->EmbodimentPoolWarmCount <= 0) {
        return;
    }

    UClass *HumanoidClass = AMythicNPCCharacter::StaticClass();
    if (!Settings->EmbodiedNPCClass.IsNull()) {
        if (UClass *Loaded = Settings->EmbodiedNPCClass.LoadSynchronous()) {
            HumanoidClass = Loaded;
        }
    }
    WarmEmbodimentPool(HumanoidClass, Settings->EmbodimentPoolWarmCount);

    UClass *CreatureClass = AMythicCreatureCharacter::StaticClass();
    if (!Settings->EmbodiedCreatureClass.IsNull()) {
        CreatureClass = Settings->EmbodiedCreatureClass.LoadSynchronous();
    }
    if (CreatureClass) {
        WarmEmbodimentPool(CreatureClass, Settings->EmbodimentPoolWarmCount);
    }
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


AMythicNPCCharacter *UMythicLivingWorldSubsystem::AcquireEmbodiedActor(UClass *ActorClass, const FVector &Loc, const FRotator &Rot) {
    if (!ActorClass) {
        return nullptr;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return nullptr;
    }

    const bool bPoolingOn = Settings && Settings->bEnableEmbodimentPooling && Settings->EmbodimentPoolMaxPerClass > 0;

    if (bPoolingOn) {
        if (TArray<TWeakObjectPtr<AMythicNPCCharacter>> *Bucket = EmbodimentPool.Find(ActorClass)) {
            while (Bucket->Num() > 0) {
                TWeakObjectPtr<AMythicNPCCharacter> Weak = Bucket->Pop(EAllowShrinking::No);
                AMythicNPCCharacter *Reused = Weak.Get();
                if (!IsValid(Reused)) {
                    continue;
                }
                Reused->SetActorLocationAndRotation(Loc, Rot,false, nullptr, ETeleportType::TeleportPhysics);
                Reused->PrepareForEmbodiment();
                return Reused;
            }
        }
    }

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    AMythicNPCCharacter *Spawned =
        World->SpawnActor<AMythicNPCCharacter>(ActorClass, Loc, Rot, SpawnInfo);
    if (Spawned) {
        Spawned->PrepareForEmbodiment();
    }
    return Spawned;
}

void UMythicLivingWorldSubsystem::ReleaseEmbodiedActor(FMassEntityHandle Entity, AMythicNPCCharacter *Actor) {
    if (!IsValid(Actor)) {
        UnregisterEmbodiedActor(Entity);
        return;
    }

    const bool bPoolingOn = Settings && Settings->bEnableEmbodimentPooling && Settings->EmbodimentPoolMaxPerClass > 0;
    if (bPoolingOn) {
        TArray<TWeakObjectPtr<AMythicNPCCharacter>> &Bucket = EmbodimentPool.FindOrAdd(Actor->GetClass());
        if (Bucket.Num() < Settings->EmbodimentPoolMaxPerClass) {
            Actor->SleepToPool();
            UnregisterEmbodiedActor(Entity);
            Bucket.Add(Actor);
            return;
        }
    }

    Actor->OnReturnedToPool();
    UnregisterEmbodiedActor(Entity);
    Actor->Destroy();
}

void UMythicLivingWorldSubsystem::WarmEmbodimentPool(UClass *ActorClass, int32 Count) {
    if (!ActorClass || Count <= 0) {
        return;
    }
    if (!Settings || !Settings->bEnableEmbodimentPooling || Settings->EmbodimentPoolMaxPerClass <= 0) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return;
    }

    TArray<TWeakObjectPtr<AMythicNPCCharacter>> &Bucket = EmbodimentPool.FindOrAdd(ActorClass);
    const int32 Target = FMath::Min(Count, Settings->EmbodimentPoolMaxPerClass);

    FActorSpawnParameters SpawnInfo;
    SpawnInfo.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

    for (int32 i = Bucket.Num(); i < Target; ++i) {
        AMythicNPCCharacter *Warmed = World->SpawnActor<AMythicNPCCharacter>(ActorClass, FVector::ZeroVector, FRotator::ZeroRotator, SpawnInfo);
        if (!Warmed) {
            break;
        }
        Warmed->SleepToPool();
        Bucket.Add(Warmed);
    }
}

void UMythicLivingWorldSubsystem::SubmitWorldEvent(const FMythicWorldEvent &Event) {
    FScopeLock Lock(&PendingEventsMutex);
    PendingEvents.Add(Event);
}

void UMythicLivingWorldSubsystem::RegisterSettlement(AMythicSettlement *Settlement) {
    if (!SettlementRegistry) {
        SettlementRegistry = NewObject<UMythicSettlementRegistry>(this);
    }

    FScopeLock Lock(&SimulationLock);

    const int32 SettlementId = SettlementRegistry->RegisterSettlement(Settlement);

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

    FScopeLock Lock(&SimulationLock);
    SettlementRegistry->TransferSettlement(SettlementId, NewFaction, TerritoryGrid, FactionDB, CausalFabric);
}

void UMythicLivingWorldSubsystem::ReportLeaderCandidate(
    FMythicFactionId FactionId, const FMythicEntityId &EntityId, float Score) {
    if (!FactionDB || !PersistentNPCRegistry
        || !PersistentNPCRegistry->ContainsEntityIdentity(EntityId)) {
        UE_LOG(LogMythLivingWorld, Warning,
               TEXT("Rejected faction leadership candidate without a registered LivingWorld identity."));
        return;
    }
    FScopeLock Lock(&SimulationLock);
    FactionDB->ReportLeaderCandidate(FactionId, EntityId, Score);
}

void UMythicLivingWorldSubsystem::ReportNpcDeath(FMythicFactionId FactionId, FGameplayTag RoleTag) {
    if (!FactionDB || !Settings || !FactionId.IsValid()) {
        return;
    }

    const int32 PopLoss = FMath::Max(0, Settings->KillPopulationLoss);
    const bool bArmed = RoleTag == TAG_NPC_ROLE_SOLDIER || RoleTag == TAG_NPC_ROLE_GUARD;
    const float ArmsLoss = FMath::Max(0.0f, Settings->KillMilitaryArmsLoss);

    FScopeLock Lock(&SimulationLock);
    FMythicFactionData *F = FactionDB->GetFactionMutable(FactionId);
    if (!F) {
        return;
    }

    F->Population = FMath::Max(0, F->Population - PopLoss);

    if (bArmed && ArmsLoss > 0.0f) {
        F->Reserves.Arms = FMath::Max(0.0f, F->Reserves.Arms - ArmsLoss);
    }

    FactionDB->CommitWrites();
}

void UMythicLivingWorldSubsystem::EnqueuePlayerResourceDelta(FMythicFactionId FactionId, EMythicResourceType Axis, float Delta) {
    if (!FactionId.IsValid() || Delta == 0.0f || !FMath::IsFinite(Delta)) {
        return;
    }
    FScopeLock Lock(&PendingPlayerResourceDeltasMutex);
    for (FMythicPendingResourceDelta &Row : PendingPlayerResourceDeltas) {
        if (Row.FactionId == FactionId && Row.Axis == Axis) {
            Row.Delta += Delta;
            return;
        }
    }
    FMythicPendingResourceDelta Row;
    Row.FactionId = FactionId;
    Row.Axis = Axis;
    Row.Delta = Delta;
    PendingPlayerResourceDeltas.Add(Row);
}

void UMythicLivingWorldSubsystem::DrainPlayerResourceDeltas() {
    if (!FactionDB) {
        return;
    }
    TArray<FMythicPendingResourceDelta> Work;
    {
        FScopeLock QueueLock(&PendingPlayerResourceDeltasMutex);
        if (PendingPlayerResourceDeltas.Num() == 0) {
            return;
        }
        Work = MoveTemp(PendingPlayerResourceDeltas);
        PendingPlayerResourceDeltas.Reset();
    }

    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float MaxPerTick = Dev ? Dev->Trading.MaxReserveInjectionPerAxisPerTick : 5.0f;
    const float MaxReserve = Settings ? FMath::Max(Settings->MaxReserve, 1.0f) : 100.0f;

    TArray<FMythicPendingResourceDelta> Remainder;
    {
        FScopeLock Lock(&SimulationLock);
        bool bAnyApplied = false;
        Remainder = MythicPlayerEconomyDelta::DrainClamped(
            Work, MaxPerTick,
            [this, MaxReserve, &bAnyApplied](const FMythicFactionId &Id, EMythicResourceType Axis, float Applied) {
                FMythicFactionData *F = FactionDB->GetFactionMutable(Id);
                if (!F || !F->bHasEconomy) {
                    return;
                }
                float &Res = F->Reserves.GetResourceMutable(Axis);
                Res = FMath::Clamp(Res + Applied, -MaxReserve, MaxReserve);
                bAnyApplied = true;
            });
        if (bAnyApplied) {
            FactionDB->CommitWrites();
        }
    }

    if (Remainder.Num() > 0) {
        FScopeLock QueueLock(&PendingPlayerResourceDeltasMutex);
        for (const FMythicPendingResourceDelta &Carry : Remainder) {
            bool bMerged = false;
            for (FMythicPendingResourceDelta &Row : PendingPlayerResourceDeltas) {
                if (Row.FactionId == Carry.FactionId && Row.Axis == Carry.Axis) {
                    Row.Delta += Carry.Delta;
                    bMerged = true;
                    break;
                }
            }
            if (!bMerged) {
                PendingPlayerResourceDeltas.Add(Carry);
            }
        }
    }
}

void UMythicLivingWorldSubsystem::HandlePermanentEntityDeath(
    const FMythicEntityId &EntityId, double WorldTime) {
    if (!SettlementRegistry && !FactionDB) {
        return;
    }
    FScopeLock Lock(&SimulationLock);
    if (SettlementRegistry) {
        SettlementRegistry->HandleNPCDeath(EntityId, WorldTime);
    }
    if (FactionDB) {
        if (FactionDB->HandlePermanentEntityDeath(EntityId)) {
            FactionDB->CommitWrites();
        }
    }
}

EMythicEntityRetirementResult
UMythicLivingWorldSubsystem::TryRetireEntityIdentity(
    const FMythicEntityId &EntityId) {
    check(IsInGameThread());
    if (!PersistentNPCRegistry
        || !PersistentNPCRegistry->ContainsEntityIdentity(EntityId)) {
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("Rejected retirement for an invalid or unregistered canonical entity."));
        return EMythicEntityRetirementResult::RejectedInvalid;
    }

    if (PersistentNPCRegistry->IsPermaDead(EntityId)) {
        return EMythicEntityRetirementResult::Tombstoned;
    }

    // World subsystems and player components mutate on the game thread, so these checks remain stable for the rest
    // of this transaction. The simulation-owned references are checked while its lock is held below.
    if (UWorld *World = GetWorld()) {
        if (const UMythicPartySubsystem *Party =
                World->GetSubsystem<UMythicPartySubsystem>();
            Party && Party->ReferencesEntityIdentity(EntityId)) {
            return EMythicEntityRetirementResult::RetainedByDurableReference;
        }

        for (FConstPlayerControllerIterator It =
                 World->GetPlayerControllerIterator();
             It; ++It) {
            const APlayerController *Controller = It->Get();
            const AMythicPlayerState *PlayerState = Controller
                ? Controller->GetPlayerState<AMythicPlayerState>()
                : nullptr;
            const UMythicEntityViewerKnowledgeComponent *Knowledge =
                PlayerState
                    ? PlayerState->GetEntityViewerKnowledgeComponent()
                    : nullptr;
            if (Knowledge && Knowledge->HasLearnedDossierForEntity(EntityId)) {
                return EMythicEntityRetirementResult::RetainedByDurableReference;
            }
        }
    }

    FScopeLock Lock(&SimulationLock);
    if (PersistentNPCRegistry->IsRetainedByLearnedDossier(EntityId)
        || (FactionDB && FactionDB->ReferencesEntityIdentity(EntityId))
        || (SettlementRegistry
            && SettlementRegistry->ReferencesEntityIdentity(EntityId))) {
        return EMythicEntityRetirementResult::RetainedByDurableReference;
    }

    return PersistentNPCRegistry->ReleaseUnreferencedEntityIdentity(EntityId)
               ? EMythicEntityRetirementResult::Retired
               : EMythicEntityRetirementResult::RejectedInvalid;
}

EMythicEntityRetirementResult
UMythicLivingWorldSubsystem::HandleUnrestorableLogicalEntity(
    const FMythicEntityId &EntityId) {
    check(IsInGameThread());
    if (!PersistentNPCRegistry
        || !PersistentNPCRegistry->ContainsEntityIdentity(EntityId)) {
        return EMythicEntityRetirementResult::RejectedInvalid;
    }

    {
        FScopeLock Lock(&SimulationLock);
        if (FactionDB && FactionDB->ClearLeaderReference(EntityId)) {
            FactionDB->CommitWrites();
        }
        if (SettlementRegistry) {
            SettlementRegistry->ClearUnrestorableShopOwner(EntityId);
        }
    }
    return TryRetireEntityIdentity(EntityId);
}

bool UMythicLivingWorldSubsystem::CopySettlementAtCell(const FMythicCellCoord &Cell, FMythicSettlementData &Out) {
    if (!SettlementRegistry) {
        return false;
    }
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
    FScopeLock Lock(&SimulationLock);
    if (const FMythicSettlementData *Found = SettlementRegistry->GetSettlementData(SettlementId)) {
        Out = *Found;
        return true;
    }
    return false;
}

void UMythicLivingWorldSubsystem::CopyAllSettlementIds(TArray<int32> &OutIds) {
    OutIds.Reset();
    if (!SettlementRegistry) {
        return;
    }
    FScopeLock Lock(&SimulationLock);
    SettlementRegistry->GetAllSettlementIds(OutIds);
}

int32 UMythicLivingWorldSubsystem::GetSettlementCountSafe() {
    if (!SettlementRegistry) {
        return 0;
    }
    FScopeLock Lock(&SimulationLock);
    return SettlementRegistry->GetSettlementCount();
}

AMythicSettlement *UMythicLivingWorldSubsystem::GetSettlementActorSafe(int32 SettlementId) {
    if (!SettlementRegistry) {
        return nullptr;
    }
    FScopeLock Lock(&SimulationLock);
    return SettlementRegistry->GetSettlementActor(SettlementId);
}

bool UMythicLivingWorldSubsystem::CopySimDiagnostics(uint64 &OutTickCount, float &OutTickIntervalSeconds, bool &OutRunning) {
    OutTickCount = 0;
    OutTickIntervalSeconds = 0.0f;
    OutRunning = false;
    if (!SimThread.IsValid()) {
        return false;
    }
    FScopeLock Lock(&SimulationLock);
    OutTickCount = SimThread->GetTickCount();
    OutTickIntervalSeconds = SimThread->GetTickIntervalSeconds();
    OutRunning = SimThread->IsRunning();
    return true;
}

bool UMythicLivingWorldSubsystem::LoadSettings() {
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

    CausalFabric = NewObject<UMythicCausalFabric>(this);
    CausalFabric->Initialize(Settings->FabricCapacity);

    FactionConfig = Settings->FactionSettings.LoadSynchronous();
    if (FactionConfig) {
        FactionDB = NewObject<UMythicFactionDatabase>(this);
        FactionDB->Initialize(FactionConfig);
    }
    else {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Faction Database Settings not loaded. Factions will not function."));
    }

    TerritoryConfig = Settings->TerritorySettings.LoadSynchronous();
    if (TerritoryConfig) {
        TerritoryGrid = NewObject<UMythicTerritoryGrid>(this);
        TerritoryGrid->Initialize(TerritoryConfig);
    }
    else {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Territory Grid Settings not loaded. Territory system will not function."));
    }

    SettlementRegistry = NewObject<UMythicSettlementRegistry>(this);

    PersistentNPCRegistry = NewObject<UMythicPersistentNPCRegistry>(this);

    DesignerSpawnerRegistry = NewObject<UMythicDesignerSpawnerRegistry>(this);

    SocialGraph = NewObject<UMythicSocialGraph>(this);
    SocialGraph->Initialize(
        Settings->SocialMaxEdgesPerEntity,
        Settings->SocialPruneStrengthThreshold,
        Settings->SocialEdgeDecayRate);

    SchemeEngine = NewObject<UMythicSchemeEngine>(this);
    SchemeEngine->Initialize(FactionDB, CausalFabric, TerritoryGrid, Settings);

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
        TWeakObjectPtr<UMythicLivingWorldSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (UMythicLivingWorldSubsystem *StrongThis = WeakThis.Get()) {
                StrongThis->ApplySettlementSanitationPressure();
                StrongThis->DrainPlayerResourceDeltas();
                if (StrongThis->Replicator) {
                    StrongThis->Replicator->SyncProxies(StrongThis);
                }
                StrongThis->OnWorldSimCommitted.Broadcast();
            }
        });
    }
    else {
        TWeakObjectPtr<UMythicLivingWorldSubsystem> WeakThis(this);
        AsyncTask(ENamedThreads::GameThread, [WeakThis]() {
            if (UMythicLivingWorldSubsystem *StrongThis = WeakThis.Get()) {
                StrongThis->ApplySettlementSanitationPressure();
                StrongThis->DrainPlayerResourceDeltas();
                StrongThis->OnWorldSimCommitted.Broadcast();
            }
        });
    }
}

void UMythicLivingWorldSubsystem::ApplySettlementSanitationPressure() {
    const UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    const UMythicCorpseHazardSubsystem *Hazard = World->GetSubsystem<UMythicCorpseHazardSubsystem>();
    if (!Hazard || !TerritoryGrid) {
        return;
    }
    if (Hazard->GetRegisteredCorpseCount() <= 0) {
        return;
    }

    TArray<int32> SettlementIds;
    CopyAllSettlementIds(SettlementIds);
    for (const int32 Id : SettlementIds) {
        FMythicSettlementData Data;
        if (!CopySettlementById(Id, Data) || !Data.GoverningFaction.IsValid()) {
            continue;
        }
        const float Penalty = Hazard->GetSanitationPenaltyForLocation(TerritoryGrid->CellToWorld(Data.CenterCell));
        if (Penalty <= 0.0f) {
            continue;
        }
        EnqueuePlayerResourceDelta(Data.GoverningFaction, EMythicResourceType::Food, -Penalty);
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

const TArray<FMythicTerritoryProxyItem> &UMythicLivingWorldSubsystem::GetAllTerritoryProxies() const {
    static const TArray<FMythicTerritoryProxyItem> Empty;
    return Replicator ? Replicator->GetAllTerritoryProxies() : Empty;
}

const TArray<FMythicSettlementProxyItem> &UMythicLivingWorldSubsystem::GetAllSettlementProxies() const {
    static const TArray<FMythicSettlementProxyItem> Empty;
    return Replicator ? Replicator->GetAllSettlementProxies() : Empty;
}


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
        FScopeLock Lock(&SimulationLock);
        SettlementRegistry->SeedTerritoryFromSettlements(TerritoryGrid, FactionDB);

        TerritoryGrid->CommitWrites();
        FactionDB->CommitWrites();
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Territory seeding complete."));
}

void UMythicLivingWorldSubsystem::BeginEntityIdentityRestoreBarrier() {
    check(IsInGameThread());
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return;
    }

    // Recognition is embodiment-scoped and must be revoked before any canonical mapping changes underneath it.
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator();
         It; ++It) {
        APlayerController *Controller = It->Get();
        AMythicPlayerState *PlayerState = Controller
            ? Controller->GetPlayerState<AMythicPlayerState>()
            : nullptr;
        if (UMythicEntityViewerKnowledgeComponent *Knowledge = PlayerState
                ? PlayerState->GetEntityViewerKnowledgeComponent()
                : nullptr) {
            Knowledge->AuthorityClearRecognitionBindings();
        }
    }

    for (TActorIterator<AMythicDesignerSpawner> It(World); It; ++It) {
        It->BeginLivingWorldRestore();
    }
    if (UMythicNPCManager *NPCManager =
            GetGameInstance()->GetSubsystem<UMythicNPCManager>()) {
        NPCManager->ResetForLivingWorldRestore();
    }

    TArray<TPair<FMassEntityHandle, TWeakObjectPtr<AMythicNPCCharacter>>>
        EmbodiedSnapshot;
    EmbodiedSnapshot.Reserve(EmbodiedActors.Num());
    for (const TPair<FMassEntityHandle,
                     TWeakObjectPtr<AMythicNPCCharacter>> &Pair :
         EmbodiedActors) {
        EmbodiedSnapshot.Add(Pair);
    }
    for (const TPair<FMassEntityHandle,
                     TWeakObjectPtr<AMythicNPCCharacter>> &Pair :
         EmbodiedSnapshot) {
        ReleaseEmbodiedActor(Pair.Key, Pair.Value.Get());
    }
    EmbodiedActors.Reset();

    if (UMythicEntityPresentationRegistry *PresentationRegistry =
            World->GetSubsystem<UMythicEntityPresentationRegistry>()) {
        TArray<UMythicEntityPresentationComponent *> Presentations;
        PresentationRegistry->GetRegisteredComponents(Presentations);
        for (UMythicEntityPresentationComponent *Presentation : Presentations) {
            if (Presentation
                && Presentation->GetAuthorityEntityId().GetDomain()
                       == EMythicEntityDomain::LivingWorld) {
                Presentation->AuthorityDeactivateEmbodiment();
            }
        }
        PresentationRegistry->ResetAuthorityDomain(
            EMythicEntityDomain::LivingWorld);
    }

    if (UMassEntitySubsystem *MassSubsystem =
            World->GetSubsystem<UMassEntitySubsystem>()) {
        FMassEntityManager &EntityManager =
            MassSubsystem->GetMutableEntityManager();
        TSharedPtr<FMassEntityManager> EntityManagerView(
            &EntityManager, [](FMassEntityManager *) {});
        FMassEntityQuery IdentityQuery(EntityManagerView);
        IdentityQuery.AddRequirement<FMythicIdentityFragment>(
            EMassFragmentAccess::ReadOnly);

        TArray<FMassEntityHandle> OldLogicalEntities;
        FMassExecutionContext Context(EntityManager);
        IdentityQuery.ForEachEntityChunk(
            Context, [&OldLogicalEntities](FMassExecutionContext &Chunk) {
                const TConstArrayView<FMythicIdentityFragment> Identities =
                    Chunk.GetFragmentView<FMythicIdentityFragment>();
                for (int32 Index = 0; Index < Chunk.GetNumEntities(); ++Index) {
                    if (Identities[Index].EntityId.GetDomain()
                        == EMythicEntityDomain::LivingWorld) {
                        OldLogicalEntities.Add(Chunk.GetEntity(Index));
                    }
                }
            });
        for (const FMassEntityHandle Entity : OldLogicalEntities) {
            if (EntityManager.IsEntityValid(Entity)) {
                EntityManager.DestroyEntity(Entity);
            }
        }
    }

    if (SocialGraph) {
        SocialGraph->ResetForLivingWorldRestore();
    }
    if (UMythicEncounterDirector *EncounterDirector =
            World->GetSubsystem<UMythicEncounterDirector>()) {
        EncounterDirector->ResetForLivingWorldRestore();
    }
}

bool UMythicLivingWorldSubsystem::CompleteEntityIdentityRestoreBarrier() {
    check(IsInGameThread());
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client) {
        return true;
    }

    // Character persistence may have restored dossiers before the LivingWorld blob. Reassert sticky ownership now
    // that the replacement identity registry is installed.
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator();
         It; ++It) {
        APlayerController *Controller = It->Get();
        AMythicPlayerState *PlayerState = Controller
            ? Controller->GetPlayerState<AMythicPlayerState>()
            : nullptr;
        const UMythicEntityViewerKnowledgeComponent *Knowledge = PlayerState
            ? PlayerState->GetEntityViewerKnowledgeComponent()
            : nullptr;
        if (!Knowledge) {
            continue;
        }
        for (const FMythicEntityLearnedDossier &Dossier :
             Knowledge->GetLearnedDossiersForSave()) {
            if (Dossier.EntityId.GetDomain()
                    == EMythicEntityDomain::LivingWorld
                && (!PersistentNPCRegistry
                    || !PersistentNPCRegistry->MarkRetainedByLearnedDossier(
                        Dossier.EntityId))) {
                UE_LOG(LogMythLivingWorld, Error,
                       TEXT("LivingWorld restore rejected a learned dossier whose canonical entity is absent from the matching world snapshot."));
                return false;
            }
        }
    }

    // The current save format can rebuild party people, but it does not yet serialize arbitrary significant Mass
    // state. Keeping a faction leader reference without a reconstructible logical person would create a dangling
    // authority identity, so clear that assignment atomically and let ordinary candidate succession resume.
    TSet<FMythicEntityId> RestorableLogicalIdentities;
    const UMythicPartySubsystem *Party =
        World->GetSubsystem<UMythicPartySubsystem>();
    if (PersistentNPCRegistry && Party) {
        RestorableLogicalIdentities.Reserve(
            PersistentNPCRegistry->GetIdentityCount());
        for (const FMythicPersistentEntityIdentityRecord &Record :
             PersistentNPCRegistry->GetIdentityRecords()) {
            if (Party->ReferencesEntityIdentity(Record.EntityId)) {
                RestorableLogicalIdentities.Add(Record.EntityId);
            }
        }
    }
    if (FactionDB) {
        FScopeLock Lock(&SimulationLock);
        FactionDB->ClearUnrestorableLeaderReferences(
            RestorableLogicalIdentities);
        if (SettlementRegistry) {
            SettlementRegistry->ClearUnrestorableShopOwners(
                RestorableLogicalIdentities);
        }
        FactionDB->CommitWrites();
    }
    else if (SettlementRegistry) {
        FScopeLock Lock(&SimulationLock);
        SettlementRegistry->ClearUnrestorableShopOwners(
            RestorableLogicalIdentities);
    }

    // Logical Mass state is intentionally rebuilt, so discard loaded ambient records that have no durable owner.
    // Tombstones, restorable parties, shop owners, and learned dossiers remain registered.
    if (PersistentNPCRegistry) {
        TArray<FMythicEntityId> LoadedIdentities;
        LoadedIdentities.Reserve(PersistentNPCRegistry->GetIdentityCount());
        for (const FMythicPersistentEntityIdentityRecord &Record :
             PersistentNPCRegistry->GetIdentityRecords()) {
            LoadedIdentities.Add(Record.EntityId);
        }
        for (const FMythicEntityId &EntityId : LoadedIdentities) {
            TryRetireEntityIdentity(EntityId);
        }
    }

    for (TActorIterator<AMythicDesignerSpawner> It(World); It; ++It) {
        It->CompleteLivingWorldRestore();
    }
    return true;
}

void UMythicLivingWorldSubsystem::SaveLivingWorld(FArchive &Ar) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicLivingWorld_Save);

    constexpr int32 CurrentLivingWorldSaveVersion = 5;
    int32 MasterVersion = CurrentLivingWorldSaveVersion;
    Ar << MasterVersion;

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
    if (DesignerSpawnerRegistry) { SectionMask |= (1 << 7); }
    Ar << SectionMask;

    FScopeLock Lock(&SimulationLock);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Saving Living World state..."));

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
        SettlementRegistry->Serialize(Ar);
    }

    if (UWorld *World = GetGameInstance()->GetWorld()) {
        if (UMythicPartySubsystem *Party = World->GetSubsystem<UMythicPartySubsystem>()) {
            Party->Serialize(Ar);
        }
    }

    if (DesignerSpawnerRegistry) {
        DesignerSpawnerRegistry->Serialize(Ar);
    }

    UE_LOG(LogMythLivingWorld, Log, TEXT("Living World state saved successfully."));
}

void UMythicLivingWorldSubsystem::LoadLivingWorld(FArchive &Ar) {
    TRACE_CPUPROFILER_EVENT_SCOPE(MythicLivingWorld_Load);

    int32 MasterVersion = 0;
    Ar << MasterVersion;

    constexpr int32 CurrentLivingWorldSaveVersion = 5;
    if (MasterVersion != CurrentLivingWorldSaveVersion) {
        UE_LOG(LogMythLivingWorld, Error, TEXT("Unsupported Living World save version: %d"), MasterVersion);
        Ar.SetError();
        return;
    }

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
        if (MasterVersion >= 3 && DesignerSpawnerRegistry) { LiveMask |= (1 << 7); }

        if (SectionMask != LiveMask) {
            UE_LOG(LogMythLivingWorld, Error,
                   TEXT("Living World load aborted: saved section set (0x%X) differs from live (0x%X) — cannot align the unframed save stream."),
                   SectionMask, LiveMask);
            Ar.SetError();
            return;
        }
    }

    const bool bRestartSimulation = SimThread.IsValid() && SimThread->IsRunning();
    StopSimulation();
    BeginEntityIdentityRestoreBarrier();

    {
        FScopeLock Lock(&SimulationLock);

        UE_LOG(LogMythLivingWorld, Log, TEXT("Loading Living World state..."));

        if (CausalFabric) {
            CausalFabric->Serialize(Ar);
        }

        if (!Ar.IsError() && FactionDB) {
            FactionDB->Serialize(Ar);
        }

        if (!Ar.IsError() && TerritoryGrid) {
            TerritoryGrid->Serialize(Ar);
        }

        if (!Ar.IsError() && SchemeEngine) {
            SchemeEngine->Serialize(Ar);
        }

        if (!Ar.IsError() && PersistentNPCRegistry) {
            PersistentNPCRegistry->Serialize(Ar);
        }

        if (!Ar.IsError() && SettlementRegistry) {
            SettlementRegistry->Serialize(Ar);
        }

        if (!Ar.IsError()) {
            if (UWorld *World = GetGameInstance()->GetWorld()) {
                if (UMythicPartySubsystem *Party =
                        World->GetSubsystem<UMythicPartySubsystem>()) {
                    Party->Serialize(Ar);
                }
            }
        }

        if (!Ar.IsError() && DesignerSpawnerRegistry) {
            DesignerSpawnerRegistry->Serialize(Ar);
        }
    }

    if (!Ar.IsError() && CompleteEntityIdentityRestoreBarrier()) {
        UE_LOG(LogMythLivingWorld, Log,
               TEXT("Living World state loaded successfully."));
    }
    else {
        Ar.SetError();
        UE_LOG(LogMythLivingWorld, Error,
               TEXT("Living World restore failed after entering the identity barrier; stale public mappings remain revoked."));
    }

    if (bRestartSimulation && !Ar.IsError()) {
        StartSimulation();
    }
}

bool UMythicLivingWorldSubsystem::IsAnyFactionInFamine() const {
    if (!FactionDB) {
        return false;
    }
    bool bFamine = false;
    FactionDB->ForEachAliveFaction([&bFamine](FMythicFactionId, const FMythicFactionData &Data) {
        bFamine |= Data.bFamineActive;
    });
    return bFamine;
}
