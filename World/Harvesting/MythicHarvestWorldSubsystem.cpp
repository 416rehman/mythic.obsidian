#include "World/Harvesting/MythicHarvestWorldSubsystem.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Animation/AnimMontage.h"
#include "Engine/World.h"
#include "GameModes/GameState/MythicGameState.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/MythicTags_GAS.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"
#include "Itemization/Inventory/Fragments/Passive/HarvestToolFragment.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Affixes/MythicItemizationHash.h"
#include "Mythic.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "Player/MythicPlayerState.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Resources/MythicResourceISM.h"
#include "Serialization/BufferArchive.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "World/Harvesting/MythicHarvestRewardPlanner.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"
#include "World/Harvesting/MythicHarvestToolTypeDefinition.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestWorldSubsystem)

namespace {
bool IsAuthorityWorld(const UWorld *World) {
    return World && World->GetNetMode() != NM_Client;
}

uint32 AdvanceNonZeroRevision(const uint32 Value) {
    const uint32 Advanced = Value + 1;
    return Advanced == 0 ? 1 : Advanced;
}

bool IsFiniteLocation(const FVector &Value) {
    return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y)
        && FMath::IsFinite(Value.Z);
}

bool IsValidReplicatedHarvestNodeItem(
    const FMythicHarvestReplicatedNodeItem &Item) {
    switch (Item.State) {
        case EMythicHarvestNodeState::Available:
        case EMythicHarvestNodeState::Depleted:
        case EMythicHarvestNodeState::Regrowing:
            break;
        default:
            return false;
    }
    return Item.PresentationStream.IsValid() && Item.NodeId.IsValid()
        && Item.Generation != 0 && Item.Revision != 0
        && Item.QuantizedRemainingWork <= Item.QuantizedMaxWork
        && FMath::IsFinite(Item.RespawnServerDeadline)
        && Item.RespawnServerDeadline >= 0.0;
}

bool ValidateHarvestWorldSnapshotForDeployment(
    const FMythicHarvestWorldSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!Settings) {
        OutDiagnosticCode = TEXT("HarvestSettingsUnavailable");
        return false;
    }
    return FMythicHarvestWorldSaveV1::Validate(
        Snapshot, OutDiagnosticCode,
        Settings->RestoreMaximumTouchedNodes,
        Settings->RestoreMaximumContributorsPerNode,
        Settings->RestoreMaximumTotalContributors,
        Settings->RestoreMaximumReplicationCells,
        Settings->RestoreMaximumCellCoordinateMagnitude);
}

bool BuildHarvestWorldSnapshotFingerprint(
    const FMythicHarvestWorldSaveV1 &Snapshot,
    FSHA256Signature &OutFingerprint) {
    FMythicHarvestWorldSaveV1 Canonical = Snapshot;
    Canonical.SortCanonical();
    FBufferArchive Payload;
    FObjectAndNameAsStringProxyArchive Archive(Payload, false);
    Archive.ArIsSaveGame = true;
    FMythicHarvestWorldSaveV1::StaticStruct()->SerializeItem(
        Archive, &Canonical, nullptr);
    return !Archive.IsError() && !Payload.IsEmpty()
        && MythicItemizationHash::Sha256(
            MakeArrayView(Payload), OutFingerprint);
}
}

bool UMythicHarvestWorldSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    return World && World->IsGameWorld();
}

void UMythicHarvestWorldSubsystem::Initialize(
    FSubsystemCollectionBase &Collection) {
    // The outbox is deliberately server-only; client instances still exist to
    // reconcile spatial node deltas and must not try to instantiate it.
    const bool bAuthorityWorld = IsAuthorityWorld(GetWorld());
    if (bAuthorityWorld) {
        Collection.InitializeDependency<UMythicPlayerRegistrySubsystem>();
        Collection.InitializeDependency<UMythicHarvestClaimMembershipSubsystem>();
        Collection.InitializeDependency<UMythicHarvestRewardOutboxSubsystem>();
    }
    Super::Initialize(Collection);
    WorldEpoch = bAuthorityWorld ? FGuid::NewGuid() : FGuid();
    AuthorityPresentationStream = FMythicHarvestPresentationStreamToken();
    if (bAuthorityWorld
        && !FMythicHarvestPresentationStreamToken::TryMakeInitial(
            FGuid::NewGuid(), AuthorityPresentationStream)) {
        UE_LOG(Myth, Error,
               TEXT("Harvest authority failed to initialize its opaque presentation stream."));
    }
    ClientPresentationStream = FMythicHarvestPresentationStreamToken();
    PresentationCoordinator.Reset();
    RespawnQueue.Reset();
    bWorldHasBegunPlay = false;
    bWorldReady = false;
    SaveRestorePhase = ESaveRestorePhase::Idle;
    PreflightCreatedAuthorityCellCoordinates.Reset();
    PreflightOutboxSnapshotSequence = 0;
    bHasPreflightSnapshotBinding = false;
    bSaveCaptureInProgress = false;
    bHarvestTransactionInProgress = false;
    bShuttingDown = false;
    NextRewardOutboxRetryServerTime = 0.0;
}

void UMythicHarvestWorldSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    bWorldHasBegunPlay = true;
    if (AMythicGameState *GameState =
            InWorld.GetGameState<AMythicGameState>()) {
        RegisterPresentationCoordinator(*GameState);
    }
    RefreshAuthorityReadyState();
}

void UMythicHarvestWorldSubsystem::RegisterPresentationCoordinator(
    AMythicGameState &Coordinator) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !World || Coordinator.GetWorld() != World
        || World->GetGameState() != &Coordinator) {
        return;
    }

    PresentationCoordinator = &Coordinator;
    if (IsAuthorityWorld(World)) {
        if (!AuthorityPresentationStream.IsValid()
            || !Coordinator.SetHarvestPresentationStreamToken(
                AuthorityPresentationStream)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest authority could not synchronize the always-relevant presentation coordinator."));
        }
        RefreshAuthorityReadyState();
        return;
    }

    if (Coordinator.HarvestPresentationStreamToken.IsValid()) {
        ActivateClientPresentationStream(
            Coordinator.HarvestPresentationStreamToken);
    }
}

bool UMythicHarvestWorldSubsystem::ActivateClientPresentationStream(
    const FMythicHarvestPresentationStreamToken &Token) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !World || World->GetNetMode() != NM_Client
        || !Token.IsValid()) {
        return false;
    }

    if (ClientPresentationStream.IsValid()) {
        const EMythicHarvestPresentationStreamOrder Order =
            FMythicHarvestPresentationStreamToken::Compare(
                Token, ClientPresentationStream);
        if (Order == EMythicHarvestPresentationStreamOrder::Same) {
            ReplayClientPresentationCells();
            return true;
        }
        if (Order == EMythicHarvestPresentationStreamOrder::Older) {
            return false;
        }
        if (Order != EMythicHarvestPresentationStreamOrder::Newer) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest client rejected invalid or conflicting presentation-stream coordinator serial %u."),
                   Token.GetSerial());
            return false;
        }
    }

    // This game-thread barrier makes the stream transition indivisible to presentation consumers. Old hidden state,
    // sources, and tombstones are discarded before any lower generation/revision from the new stream is replayed.
    ResetClientPresentationStreamState();
    ClientPresentationStream = Token;
    ReplayClientPresentationCells();
    return true;
}

UMythicHarvestWorldSubsystem::EClientPresentationStreamDisposition
UMythicHarvestWorldSubsystem::ClassifyClientPresentationStream(
    const FMythicHarvestPresentationStreamToken &Token) const {
    if (!Token.IsValid()) {
        return EClientPresentationStreamDisposition::Rejected;
    }
    if (!ClientPresentationStream.IsValid()) {
        return EClientPresentationStreamDisposition::Future;
    }
    const EMythicHarvestPresentationStreamOrder Order =
        FMythicHarvestPresentationStreamToken::Compare(
            Token, ClientPresentationStream);
    switch (Order) {
        case EMythicHarvestPresentationStreamOrder::Same:
            return EClientPresentationStreamDisposition::Current;
        case EMythicHarvestPresentationStreamOrder::Newer:
            return EClientPresentationStreamDisposition::Future;
        case EMythicHarvestPresentationStreamOrder::Conflict:
        case EMythicHarvestPresentationStreamOrder::Invalid:
            UE_LOG(Myth, Error,
                   TEXT("Harvest client rejected an invalid or conflicting spatial presentation stream serial %u."),
                   Token.GetSerial());
            return EClientPresentationStreamDisposition::Rejected;
        default:
            return EClientPresentationStreamDisposition::Rejected;
    }
}

void UMythicHarvestWorldSubsystem::ResetClientPresentationStreamState() {
    TArray<FMythicHarvestNodeId> PreviouslyChangedNodes;
    ClientNodeState.GenerateKeyArray(PreviouslyChangedNodes);
    TMap<UMythicResourceISM *,
         TArray<FMythicHarvestNodePresentationUpdate>> UpdatesByProvider;
    for (const FMythicHarvestNodeId &NodeId : PreviouslyChangedNodes) {
        if (const TWeakObjectPtr<UMythicResourceISM> *WeakProvider =
                ClientPresentationProviderByNode.Find(NodeId)) {
            if (UMythicResourceISM *Provider = WeakProvider->Get()) {
                UpdatesByProvider.FindOrAdd(Provider).Add({NodeId, true});
            }
        }
    }
    for (TPair<UMythicResourceISM *,
               TArray<FMythicHarvestNodePresentationUpdate>> &Pair :
         UpdatesByProvider) {
        if (!Pair.Key->ApplyNodeAvailabilityBatch(Pair.Value)) {
            UE_LOG(Myth, Error,
                   TEXT("Client harvest provider '%s' failed its stale-presentation clear batch during stream rotation."),
                   *Pair.Key->GetPathName());
        }
    }

    ClientNodeState.Reset();
    ClientNodeHighWater.Reset();
    ClientNodeSourcesByNode.Reset();
    ClientNodeIdsByCell.Reset();
    ClientDeferredPresentationCells.Reset();
}

void UMythicHarvestWorldSubsystem::ReplayClientPresentationCells() {
    if (!ClientPresentationStream.IsValid()) {
        return;
    }

    TArray<TWeakObjectPtr<AMythicHarvestReplicationCell>> Cells;
    Cells.Reserve(ClientCells.Num());
    for (const TWeakObjectPtr<AMythicHarvestReplicationCell> &WeakCell :
         ClientCells) {
        Cells.Add(WeakCell);
    }
    ClientDeferredPresentationCells.Reset();
    for (const TWeakObjectPtr<AMythicHarvestReplicationCell> &WeakCell :
         Cells) {
        if (AMythicHarvestReplicationCell *Cell = WeakCell.Get()) {
            HandleClientCellBatch(*Cell);
        }
    }
}

bool UMythicHarvestWorldSubsystem::HasReadyAuthorityPresentationCoordinator()
    const {
    const UWorld *World = GetWorld();
    const AMythicGameState *Coordinator = PresentationCoordinator.Get();
    return IsAuthorityWorld(World) && AuthorityPresentationStream.IsValid()
        && Coordinator && Coordinator->GetWorld() == World
        && World->GetGameState() == Coordinator && Coordinator->HasAuthority()
        && Coordinator->HarvestPresentationStreamToken
            == AuthorityPresentationStream;
}

void UMythicHarvestWorldSubsystem::RefreshAuthorityReadyState() {
    UWorld *World = GetWorld();
    if (!IsAuthorityWorld(World)) {
        bWorldReady = bWorldHasBegunPlay && !bShuttingDown;
        return;
    }
    bWorldReady = bWorldHasBegunPlay && !bShuttingDown
        && SaveRestorePhase == ESaveRestorePhase::Idle
        && !bSaveCaptureInProgress
        && HasReadyAuthorityPresentationCoordinator();
}

void UMythicHarvestWorldSubsystem::Deinitialize() {
    bShuttingDown = true;
    SaveRestorePhase = ESaveRestorePhase::Idle;
    PreflightCreatedAuthorityCellCoordinates.Reset();
    PreflightOutboxSnapshotSequence = 0;
    bHasPreflightSnapshotBinding = false;
    bSaveCaptureInProgress = false;
    bHarvestTransactionInProgress = false;
    bWorldHasBegunPlay = false;
    bWorldReady = false;
    for (const TWeakObjectPtr<AMythicHarvestReplicationCell> &WeakCell : ClientCells) {
        if (AMythicHarvestReplicationCell *Cell = WeakCell.Get()) {
            Cell->OnReplicationBatchReceived.RemoveAll(this);
            Cell->OnCellEndingPlay.RemoveAll(this);
        }
    }
    ClientCells.Reset();
    ClientNodeState.Reset();
    ClientNodeHighWater.Reset();
    ClientDeferredPresentationCells.Reset();
    ClientNodeIdsByCell.Reset();
    ClientNodeSourcesByNode.Reset();
    ClientPresentationNodesByProvider.Reset();
    ClientPresentationProviderByNode.Reset();
    AttackCycles.Reset();
    AuthorityCells.Reset();
    RespawnQueue.Reset();
    NodesByProvider.Reset();
    Nodes.Reset();
    LoadedDefinitionClosure.Reset();
    LastRejectedFeedbackTime.Reset();
    NextRewardOutboxRetryServerTime = 0.0;
    AuthorityPresentationStream = FMythicHarvestPresentationStreamToken();
    ClientPresentationStream = FMythicHarvestPresentationStreamToken();
    PresentationCoordinator.Reset();
    WorldEpoch.Invalidate();
    Super::Deinitialize();
}

TStatId UMythicHarvestWorldSubsystem::GetStatId() const {
    RETURN_QUICK_DECLARE_CYCLE_STAT(UMythicHarvestWorldSubsystem,
                                    STATGROUP_Tickables);
}

void UMythicHarvestWorldSubsystem::Tick(const float /*DeltaTime*/) {
    UWorld *World = GetWorld();
    if (!bWorldReady || bShuttingDown || !World) {
        return;
    }
    if (World->GetNetMode() == NM_Client) {
        return;
    }
    if (!IsAuthorityWorld(World)) {
        return;
    }
    const double ServerNow = World->GetTimeSeconds();
    TickRespawns(ServerNow);
    PruneAttackCycles(ServerNow);
    TickRewardOutbox(ServerNow);
}

bool UMythicHarvestWorldSubsystem::RefreshResourceProvider(
    UMythicResourceISM &Provider) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !IsAuthorityWorld(World)
        || Provider.GetWorld() != World
        || !Provider.HarvestableDefinition
        || !Provider.HasValidHarvestCollisionContract()) {
        return false;
    }

    TArray<FMythicHarvestProviderNode> ProviderRows;
    Provider.GetHarvestProviderNodes(ProviderRows);
    if (ProviderRows.IsEmpty()) {
        return false;
    }

    FMythicHarvestWork AuthoredMaximum;
    if (!FMythicHarvestWork::TryFromWorkUnits(
            Provider.HarvestableDefinition->MaxWork, AuthoredMaximum)
        || AuthoredMaximum.IsZero()) {
        return false;
    }

    UMythicHarvestRewardOutboxSubsystem *RewardOutbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    if (!RewardOutbox || !WorldEpoch.IsValid()) {
        return false;
    }

    TSet<FMythicHarvestNodeId> IncomingIds;
    TMap<FMythicHarvestNodeId, uint32> InitialGenerations;
    IncomingIds.Reserve(ProviderRows.Num());
    InitialGenerations.Reserve(ProviderRows.Num());
    for (const FMythicHarvestProviderNode &Row : ProviderRows) {
        if (!Row.NodeId.IsValid() || !Row.PrimitiveInstanceId.IsValid()
            || !IsFiniteLocation(Row.OriginalWorldTransform.GetLocation())
            || IncomingIds.Contains(Row.NodeId)) {
            return false;
        }
        const FRuntimeNode *Existing = Nodes.Find(Row.NodeId);
        if (Existing && Existing->Provider.IsValid()
            && Existing->Provider.Get() != &Provider) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest provider '%s' duplicated stable node %s already owned by '%s'."),
                   *Provider.GetPathName(), *Row.NodeId.GetGuid().ToString(),
                   *GetNameSafe(Existing->Provider.Get()));
            return false;
        }
        if (Existing && Existing->Definition.IsValid()
            && Existing->Definition.Get() != Provider.HarvestableDefinition) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest node %s changed direct definition between provider lifetimes; registration rejected."),
                   *Row.NodeId.GetGuid().ToString());
            return false;
        }
        const FIntPoint IncomingCellCoordinate =
            MakeCellCoordinate(Row.OriginalWorldTransform.GetLocation());
        if (Existing && Existing->bHasReplicationCellCoordinate
            && Existing->ReplicationCellCoordinate
                != IncomingCellCoordinate) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest node %s changed cooked replication cell from (%d,%d) to (%d,%d); registration rejected."),
                   *Row.NodeId.GetGuid().ToString(),
                   Existing->ReplicationCellCoordinate.X,
                   Existing->ReplicationCellCoordinate.Y,
                   IncomingCellCoordinate.X, IncomingCellCoordinate.Y);
            return false;
        }
        if (!Existing) {
            uint32 InitialGeneration = 0;
            if (!RewardOutbox->TryResolveNextGeneration(
                    WorldEpoch, Row.NodeId, InitialGeneration)) {
                UE_LOG(Myth, Error,
                       TEXT("Harvest provider '%s' could not resolve a never-issued generation for node %s."),
                       *Provider.GetPathName(),
                       *Row.NodeId.GetGuid().ToString());
                return false;
            }
            InitialGenerations.Add(Row.NodeId, InitialGeneration);
        }
        IncomingIds.Add(Row.NodeId);
    }

    // Presentation is preflighted before the authority registry is changed. A
    // component-side failure quarantines this provider and schedules a bounded
    // re-registration; no partial node batch becomes authoritative.
    TArray<FMythicHarvestNodePresentationUpdate> PresentationUpdates;
    PresentationUpdates.Reserve(ProviderRows.Num());
    for (const FMythicHarvestProviderNode &Row : ProviderRows) {
        const FRuntimeNode *Existing = Nodes.Find(Row.NodeId);
        PresentationUpdates.Add(
            {Row.NodeId,
             !Existing
                 || Existing->State
                     == EMythicHarvestNodeState::Available});
    }
    if (!Provider.ApplyNodeAvailabilityBatch(PresentationUpdates)) {
        UE_LOG(Myth, Error,
               TEXT("Harvest provider '%s' failed its atomic presentation preflight batch; registration rejected."),
               *Provider.GetPathName());
        return false;
    }

    // Harvestable definitions are a tiny Asset-Manager-backed closure. Keeping each
    // direct source resident preserves exact-definition collision checks after World
    // Partition unloads the component that supplied an unavailable durable node.
    LoadedDefinitionClosure.Add(Provider.HarvestableDefinition);

    const TWeakObjectPtr<UMythicResourceISM> ProviderKey(&Provider);
    if (TSet<FMythicHarvestNodeId> *OldIds = NodesByProvider.Find(ProviderKey)) {
        for (const FMythicHarvestNodeId &OldId : *OldIds) {
            if (!IncomingIds.Contains(OldId)) {
                if (FRuntimeNode *OldNode = Nodes.Find(OldId)) {
                    OldNode->Provider.Reset();
                    OldNode->RuntimeInstanceId = FPrimitiveInstanceId();
                }
            }
        }
    }

    for (const FMythicHarvestProviderNode &Row : ProviderRows) {
        FRuntimeNode *Node = Nodes.Find(Row.NodeId);
        if (!Node) {
            FRuntimeNode NewNode;
            NewNode.NodeId = Row.NodeId;
            NewNode.Provider = &Provider;
            NewNode.Definition = Provider.HarvestableDefinition;
            NewNode.RuntimeInstanceId = Row.PrimitiveInstanceId;
            NewNode.OriginalWorldLocation =
                Row.OriginalWorldTransform.GetLocation();
            NewNode.ReplicationCellCoordinate =
                MakeCellCoordinate(NewNode.OriginalWorldLocation);
            NewNode.bHasReplicationCellCoordinate = true;
            NewNode.MaximumWork = AuthoredMaximum;
            NewNode.RemainingWork = AuthoredMaximum;
            NewNode.Generation = InitialGenerations.FindChecked(Row.NodeId);
            Node = &Nodes.Add(Row.NodeId, MoveTemp(NewNode));
        }
        else {
            Node->Provider = &Provider;
            Node->Definition = Provider.HarvestableDefinition;
            Node->RuntimeInstanceId = Row.PrimitiveInstanceId;
            Node->OriginalWorldLocation =
                Row.OriginalWorldTransform.GetLocation();
            Node->ReplicationCellCoordinate =
                MakeCellCoordinate(Node->OriginalWorldLocation);
            Node->bHasReplicationCellCoordinate = true;
        }

        // The first accepted hit freezes this generation's maximum-work contract. Streaming or a live balance edit
        // cannot rescale progress after durable work XP/contribution exists; new authoring takes effect on untouched
        // or future generations only.
        const FMythicHarvestWork PreviousMaximum = Node->MaximumWork;
        const FMythicHarvestWork PreviousRemaining = Node->RemainingWork;
        if (Node->State != EMythicHarvestNodeState::Available) {
            Node->MaximumWork = AuthoredMaximum;
            Node->RemainingWork = FMythicHarvestWork();
        }
        else if (PreviousMaximum.IsZero()
                 || PreviousRemaining.IsZero()
                 || PreviousRemaining.GetQuanta() >= PreviousMaximum.GetQuanta()) {
            Node->MaximumWork = AuthoredMaximum;
            Node->RemainingWork = AuthoredMaximum;
        }

        if (bWorldReady
            && (Node->State != EMythicHarvestNodeState::Available
                || Node->RemainingWork != Node->MaximumWork)) {
            if (!PublishNodeDelta(*Node)) {
                UE_LOG(Myth, Fatal,
                       TEXT("Harvest provider refresh could not publish prevalidated node %s."),
                       *Node->NodeId.GetGuid().ToString());
            }
        }
    }
    NodesByProvider.Add(ProviderKey, MoveTemp(IncomingIds));
    return true;
}

bool UMythicHarvestWorldSubsystem::RefreshClientPresentationProvider(
    UMythicResourceISM &Provider) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !World || World->GetNetMode() != NM_Client
        || Provider.GetWorld() != World || !Provider.HarvestableDefinition
        || !Provider.HasValidHarvestCollisionContract()) {
        return false;
    }

    // World Partition normally unregisters components explicitly. Prune any
    // lifetime that was nevertheless garbage-collected during teardown so a
    // stale weak key cannot grow the client-only reverse index forever.
    for (auto It = ClientPresentationNodesByProvider.CreateIterator(); It;
         ++It) {
        const TWeakObjectPtr<UMythicResourceISM> StaleProvider = It.Key();
        if (StaleProvider.IsValid()) {
            continue;
        }
        for (const FMythicHarvestNodeId &StaleNodeId : It.Value()) {
            const TWeakObjectPtr<UMythicResourceISM> *IndexedProvider =
                ClientPresentationProviderByNode.Find(StaleNodeId);
            if (IndexedProvider && *IndexedProvider == StaleProvider) {
                ClientPresentationProviderByNode.Remove(StaleNodeId);
            }
        }
        It.RemoveCurrent();
    }

    TArray<FMythicHarvestProviderNode> ProviderRows;
    Provider.GetHarvestProviderNodes(ProviderRows);
    if (ProviderRows.IsEmpty()) {
        return false;
    }

    TSet<FMythicHarvestNodeId> IncomingIds;
    IncomingIds.Reserve(ProviderRows.Num());
    for (const FMythicHarvestProviderNode &Row : ProviderRows) {
        if (!Row.NodeId.IsValid() || !Row.PrimitiveInstanceId.IsValid()
            || !IsFiniteLocation(Row.OriginalWorldTransform.GetLocation())
            || IncomingIds.Contains(Row.NodeId)) {
            return false;
        }
        if (const TWeakObjectPtr<UMythicResourceISM> *ExistingProvider =
                ClientPresentationProviderByNode.Find(Row.NodeId);
            ExistingProvider && ExistingProvider->IsValid()
            && ExistingProvider->Get() != &Provider) {
            UE_LOG(Myth, Error,
                   TEXT("Client harvest presentation provider '%s' duplicated stable node %s already presented by '%s'."),
                   *Provider.GetPathName(), *Row.NodeId.GetGuid().ToString(),
                   *GetNameSafe(ExistingProvider->Get()));
            return false;
        }
        IncomingIds.Add(Row.NodeId);
    }

    TArray<FMythicHarvestNodePresentationUpdate> PresentationUpdates;
    PresentationUpdates.Reserve(ProviderRows.Num());
    for (const FMythicHarvestProviderNode &Row : ProviderRows) {
        const FMythicHarvestReplicatedNodeItem *ClientState =
            ClientNodeState.Find(Row.NodeId);
        PresentationUpdates.Add(
            {Row.NodeId,
             !ClientState
                 || ClientState->State
                     == EMythicHarvestNodeState::Available});
    }
    if (!Provider.ApplyNodeAvailabilityBatch(PresentationUpdates)) {
        UE_LOG(Myth, Error,
               TEXT("Client harvest presentation provider '%s' failed its atomic stable-node preflight batch; registration rejected."),
               *Provider.GetPathName());
        return false;
    }

    const TWeakObjectPtr<UMythicResourceISM> ProviderKey(&Provider);
    if (TSet<FMythicHarvestNodeId> *OldIds =
            ClientPresentationNodesByProvider.Find(ProviderKey)) {
        for (const FMythicHarvestNodeId &OldId : *OldIds) {
            if (IncomingIds.Contains(OldId)) {
                continue;
            }
            const TWeakObjectPtr<UMythicResourceISM> *IndexedProvider =
                ClientPresentationProviderByNode.Find(OldId);
            if (IndexedProvider && IndexedProvider->Get() == &Provider) {
                ClientPresentationProviderByNode.Remove(OldId);
            }
        }
    }

    for (const FMythicHarvestProviderNode &Row : ProviderRows) {
        ClientPresentationProviderByNode.Add(Row.NodeId, &Provider);
    }
    ClientPresentationNodesByProvider.Add(ProviderKey, MoveTemp(IncomingIds));

    return true;
}

void UMythicHarvestWorldSubsystem::UnregisterResourceProvider(
    UMythicResourceISM &Provider) {
    if (!IsAuthorityWorld(GetWorld()) || Provider.GetWorld() != GetWorld()) {
        return;
    }
    const TWeakObjectPtr<UMythicResourceISM> ProviderKey(&Provider);
    TSet<FMythicHarvestNodeId> DetachedIds;
    if (!NodesByProvider.RemoveAndCopyValue(ProviderKey, DetachedIds)) {
        return;
    }
    for (const FMythicHarvestNodeId &NodeId : DetachedIds) {
        if (FRuntimeNode *Node = Nodes.Find(NodeId);
            Node && Node->Provider.Get() == &Provider) {
            if (Node->State == EMythicHarvestNodeState::Available) {
                const bool bHasPartialWork =
                    !Node->MaximumWork.IsZero()
                    && !Node->RemainingWork.IsZero()
                    && Node->RemainingWork < Node->MaximumWork;
                bool bHasExplicitAvailableRow = false;
                if (Node->bHasReplicationCellCoordinate) {
                    if (const TWeakObjectPtr<AMythicHarvestReplicationCell>
                            *WeakCell = AuthorityCells.Find(
                                Node->ReplicationCellCoordinate)) {
                        if (const AMythicHarvestReplicationCell *Cell =
                                WeakCell->Get()) {
                            const FMythicHarvestReplicatedNodeItem *Delta =
                                Cell->FindNodeDelta(NodeId);
                            bHasExplicitAvailableRow = Delta
                                && Delta->State
                                    == EMythicHarvestNodeState::Available
                                && Delta->Generation == Node->Generation
                                && Delta->Revision == Node->Revision;
                        }
                    }
                }

                // Untouched Available rows are normally implicit. An explicit
                // Available row inside a mixed-state live cell must retain its
                // exact version until that cell/channel retires, otherwise a
                // later provider lifetime could publish below the cell's row.
                if (!FMythicHarvestStreamingPolicy::ShouldRetainDetachedNode(
                        Node->State, Node->Generation, bHasPartialWork)
                    && !bHasExplicitAvailableRow) {
                    Nodes.Remove(NodeId);
                    continue;
                }
            }
            // Soft claims are provider-lifetime leases. A partial contributor
            // ledger, however, is authoritative session state and must survive
            // World Partition churn with its exact remaining work.
            Node->ClaimOwner = FMythicHarvestClaimIdentity();
            Node->ClaimExpiryServerTime = 0.0;
            if (Node->State != EMythicHarvestNodeState::Available
                || Node->RemainingWork == Node->MaximumWork) {
                Node->Contributors.Reset();
            }
            Node->Provider.Reset();
            Node->RuntimeInstanceId = FPrimitiveInstanceId();
        }
    }
}

void UMythicHarvestWorldSubsystem::UnregisterClientPresentationProvider(
    UMythicResourceISM &Provider) {
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() != NM_Client
        || Provider.GetWorld() != World) {
        return;
    }

    const TWeakObjectPtr<UMythicResourceISM> ProviderKey(&Provider);
    TSet<FMythicHarvestNodeId> DetachedIds;
    if (!ClientPresentationNodesByProvider.RemoveAndCopyValue(
            ProviderKey, DetachedIds)) {
        return;
    }
    for (const FMythicHarvestNodeId &NodeId : DetachedIds) {
        const TWeakObjectPtr<UMythicResourceISM> *IndexedProvider =
            ClientPresentationProviderByNode.Find(NodeId);
        if (IndexedProvider && IndexedProvider->Get() == &Provider) {
            ClientPresentationProviderByNode.Remove(NodeId);
        }
    }
}

bool UMythicHarvestWorldSubsystem::BeginSaveRestore(
    FName &OutDiagnosticCode) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !IsAuthorityWorld(World)) {
        OutDiagnosticCode = TEXT("HarvestAuthorityWorldUnavailable");
        return false;
    }
    if (bHarvestTransactionInProgress || HasActiveNodeMutation()) {
        OutDiagnosticCode = TEXT("HarvestTransactionInProgress");
        return false;
    }
    if (SaveRestorePhase != ESaveRestorePhase::Idle
        || bSaveCaptureInProgress) {
        OutDiagnosticCode = TEXT("HarvestRestoreAlreadyInProgress");
        return false;
    }

    SaveRestorePhase = ESaveRestorePhase::Acquired;
    PreflightCreatedAuthorityCellCoordinates.Reset();
    PreflightOutboxSnapshotSequence = 0;
    bHasPreflightSnapshotBinding = false;
    bWorldReady = false;
    AttackCycles.Reset();
    OutDiagnosticCode = NAME_None;
    return true;
}

void UMythicHarvestWorldSubsystem::AbortSaveRestore() {
    if (SaveRestorePhase == ESaveRestorePhase::Idle) {
        return;
    }
    if (SaveRestorePhase == ESaveRestorePhase::Applied) {
        UE_LOG(Myth, Error,
               TEXT("Harvest restore abort rejected after apply; only CompleteSaveRestore may release an applied transaction."));
        return;
    }
    DiscardPreflightCreatedReplicationCells();
    PreflightOutboxSnapshotSequence = 0;
    bHasPreflightSnapshotBinding = false;
    SaveRestorePhase = ESaveRestorePhase::Idle;
    RefreshAuthorityReadyState();
}

bool UMythicHarvestWorldSubsystem::CompleteSaveRestore(
    FName &OutDiagnosticCode) {
    if (SaveRestorePhase != ESaveRestorePhase::Applied
        || bHarvestTransactionInProgress
        || HasActiveNodeMutation() || bShuttingDown
        || !IsAuthorityWorld(GetWorld())) {
        OutDiagnosticCode = TEXT("HarvestRestoreCompletionInvalid");
        return false;
    }
    SaveRestorePhase = ESaveRestorePhase::Idle;
    PreflightCreatedAuthorityCellCoordinates.Reset();
    PreflightOutboxSnapshotSequence = 0;
    bHasPreflightSnapshotBinding = false;
    RefreshAuthorityReadyState();
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestWorldSubsystem::BeginSaveCapture(
    FName &OutDiagnosticCode) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !bWorldReady || !IsAuthorityWorld(World)) {
        OutDiagnosticCode = TEXT("HarvestWorldNotSaveReady");
        return false;
    }
    if (bHarvestTransactionInProgress || HasActiveNodeMutation()) {
        OutDiagnosticCode = TEXT("HarvestTransactionInProgress");
        return false;
    }
    if (SaveRestorePhase != ESaveRestorePhase::Idle
        || bSaveCaptureInProgress) {
        OutDiagnosticCode = TEXT("HarvestSaveGateAlreadyInProgress");
        return false;
    }

    bSaveCaptureInProgress = true;
    bWorldReady = false;
    AttackCycles.Reset();
    OutDiagnosticCode = NAME_None;
    return true;
}

void UMythicHarvestWorldSubsystem::EndSaveCapture() {
    if (!bSaveCaptureInProgress) {
        return;
    }
    bSaveCaptureInProgress = false;
    RefreshAuthorityReadyState();
}

bool UMythicHarvestWorldSubsystem::BuildSaveSnapshot(
    FMythicHarvestWorldSaveV1 &OutSnapshot,
    FName &OutDiagnosticCode) const {
    OutSnapshot = FMythicHarvestWorldSaveV1();
    const UWorld *World = GetWorld();
    if ((!bWorldReady && !bSaveCaptureInProgress)
        || SaveRestorePhase != ESaveRestorePhase::Idle
        || bHarvestTransactionInProgress
        || HasActiveNodeMutation() || bShuttingDown
        || !IsAuthorityWorld(World) || !WorldEpoch.IsValid()) {
        OutDiagnosticCode = TEXT("HarvestWorldNotSaveReady");
        return false;
    }

    OutSnapshot.WorldEpoch = WorldEpoch;
    const double ServerNow = World->GetTimeSeconds();
    OutSnapshot.Nodes.Reserve(Nodes.Num());
    for (const TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        const FRuntimeNode &Node = Pair.Value;
        const bool bHasPartialWork =
            Node.State == EMythicHarvestNodeState::Available
            && !Node.MaximumWork.IsZero()
            && !Node.RemainingWork.IsZero()
            && Node.RemainingWork < Node.MaximumWork;
        if (Node.State == EMythicHarvestNodeState::Available
            && !bHasPartialWork) {
            continue;
        }
        if (!Node.bHasReplicationCellCoordinate
            || !FMath::IsFinite(Node.OriginalWorldLocation.Z)) {
            OutSnapshot = FMythicHarvestWorldSaveV1();
            OutDiagnosticCode = TEXT("HarvestSpatialRoutingUnavailable");
            return false;
        }

        FMythicSavedHarvestNodeV1 &Saved =
            OutSnapshot.Nodes.AddDefaulted_GetRef();
        Saved.NodeGuid = Node.NodeId.GetGuid();
        Saved.WorldEpoch = WorldEpoch;
        Saved.Generation = Node.Generation;
        Saved.Revision = Node.Revision;
        Saved.ReplicationCellCoordinate =
            Node.ReplicationCellCoordinate;
        Saved.ReplicationCellCenterZ =
            static_cast<float>(Node.OriginalWorldLocation.Z);
        Saved.State = Node.State;
        if (bHasPartialWork) {
            Saved.RemainingRespawnSeconds = 0.0;
            Saved.CapturedMaximumWorkQuanta =
                Node.MaximumWork.GetQuanta();
            Saved.RemainingWorkQuanta = Node.RemainingWork.GetQuanta();
            Saved.Contributors.Reserve(Node.Contributors.Num());
            for (const TPair<FString, FMythicHarvestParticipantSnapshot> &ContributorPair :
                 Node.Contributors) {
                const FMythicHarvestParticipantSnapshot &Participant =
                    ContributorPair.Value;
                FMythicSavedHarvestContributorV1 &Contributor =
                    Saved.Contributors.AddDefaulted_GetRef();
                Contributor.ContributorKey = Participant.ContributorKey;
                Contributor.ContributionQuanta =
                    Participant.ContributionQuanta;
                Contributor.ItemLevel = Participant.ItemLevel;
                Contributor.QuantityMultiplierQuanta =
                    Participant.QuantityMultiplierQuanta;
                Contributor.ProficiencyLevel =
                    Participant.ProficiencyLevel;
                Contributor.WorkRewardContract =
                    Participant.WorkRewardContract;
            }
        }
        else {
            Saved.RemainingRespawnSeconds = FMath::Max(
                0.0, Node.RespawnServerDeadline - ServerNow);
        }
    }
    OutSnapshot.SortCanonical();
    if (!ValidateHarvestWorldSnapshotForDeployment(
            OutSnapshot, OutDiagnosticCode)) {
        OutSnapshot = FMythicHarvestWorldSaveV1();
        return false;
    }
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestWorldSubsystem::PreflightSaveRestore(
    const FMythicHarvestWorldSaveV1 &WorldSnapshot,
    const FMythicHarvestRewardOutboxSaveV1 &OutboxSnapshot,
    FName &OutDiagnosticCode) {
    UWorld *World = GetWorld();
    if (SaveRestorePhase != ESaveRestorePhase::Acquired || bShuttingDown
        || bHarvestTransactionInProgress || HasActiveNodeMutation()
        || !IsAuthorityWorld(World)) {
        OutDiagnosticCode = TEXT("HarvestRestoreGateNotAcquired");
        return false;
    }
    if (!ValidateHarvestWorldSnapshotForDeployment(
            WorldSnapshot, OutDiagnosticCode)
        || !UMythicHarvestRewardOutboxSubsystem::ValidateSaveSnapshot(
            OutboxSnapshot, OutDiagnosticCode)
        || WorldSnapshot.WorldEpoch != OutboxSnapshot.WorldEpoch) {
        if (OutDiagnosticCode.IsNone()) {
            OutDiagnosticCode = TEXT("HarvestRestoreEpochMismatch");
        }
        return false;
    }
    FSHA256Signature CandidateWorldFingerprint{};
    FSHA256Signature CandidateOutboxFingerprint{};
    if (!BuildHarvestWorldSnapshotFingerprint(
            WorldSnapshot, CandidateWorldFingerprint)
        || !UMythicHarvestRewardOutboxSubsystem::
            BuildSaveSnapshotFingerprint(
                OutboxSnapshot, CandidateOutboxFingerprint)) {
        OutDiagnosticCode = TEXT("HarvestRestoreFingerprintFailed");
        return false;
    }

    TMap<FMythicHarvestNodeId, uint32> HighWaterByNode;
    HighWaterByNode.Reserve(
        OutboxSnapshot.GenerationHighWatermarks.Num());
    for (const FMythicSavedHarvestGenerationHighWaterV1 &SavedHighWater :
         OutboxSnapshot.GenerationHighWatermarks) {
        HighWaterByNode.Add(
            FMythicHarvestNodeId(SavedHighWater.NodeGuid),
            SavedHighWater.HighestKnownGeneration);
    }

    TSet<FMythicHarvestNodeId> ExplicitNodeIds;
    ExplicitNodeIds.Reserve(WorldSnapshot.Nodes.Num());
    TMap<FIntPoint, float> RequiredReplicationCells;
    RequiredReplicationCells.Reserve(WorldSnapshot.Nodes.Num());
    for (const FMythicSavedHarvestNodeV1 &Saved :
         WorldSnapshot.Nodes) {
        const FMythicHarvestNodeId NodeId(Saved.NodeGuid);
        ExplicitNodeIds.Add(NodeId);
        const uint32 *HighWater = HighWaterByNode.Find(NodeId);
        if (Saved.State == EMythicHarvestNodeState::Available) {
            const uint32 ExpectedGeneration = HighWater
                ? (*HighWater == MAX_uint32 ? 0u : *HighWater + 1u)
                : 1u;
            if (ExpectedGeneration == 0
                || Saved.Generation != ExpectedGeneration) {
                OutDiagnosticCode =
                    TEXT("HarvestPartialGenerationMismatch");
                return false;
            }
        }
        else if (!HighWater || *HighWater != Saved.Generation) {
            OutDiagnosticCode = TEXT("HarvestMissingCompletionReceipt");
            return false;
        }

        if (const FRuntimeNode *Resident = Nodes.Find(NodeId);
            Resident && Resident->Provider.IsValid()
            && (!Resident->bHasReplicationCellCoordinate
                || Resident->ReplicationCellCoordinate
                    != Saved.ReplicationCellCoordinate)) {
            OutDiagnosticCode = TEXT("HarvestSpatialRoutingMismatch");
            return false;
        }
        RequiredReplicationCells.FindOrAdd(
            Saved.ReplicationCellCoordinate,
            Saved.ReplicationCellCenterZ);
    }

    for (const TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        if (!Pair.Value.Provider.IsValid()
            || ExplicitNodeIds.Contains(Pair.Key)) {
            continue;
        }
        if (const uint32 *HighWater = HighWaterByNode.Find(Pair.Key);
            HighWater && *HighWater == MAX_uint32) {
            OutDiagnosticCode = TEXT("HarvestGenerationResolutionFailed");
            return false;
        }
    }

    FMythicHarvestPresentationStreamToken NextStream;
    if (!FMythicHarvestPresentationStreamToken::TryAdvance(
            AuthorityPresentationStream, NextStream)
        || !CanResetAuthorityReplicationStream(NextStream)) {
        OutDiagnosticCode = TEXT("HarvestPresentationStreamRotationFailed");
        return false;
    }

    // Only after every payload, cross-domain high-water, resident route, and existing stream transition validates do
    // we reserve runtime actor capacity. Any partial reservation remains tracked and is removed on a recoverable
    // preflight failure/abort; live cells that existed before this transaction are never touched.
    TArray<FIntPoint> RequiredCoordinates;
    RequiredReplicationCells.GenerateKeyArray(RequiredCoordinates);
    RequiredCoordinates.Sort([](const FIntPoint &Left,
                                const FIntPoint &Right) {
        return Left.X == Right.X ? Left.Y < Right.Y : Left.X < Right.X;
    });
    for (const FIntPoint &Coordinate : RequiredCoordinates) {
        const TWeakObjectPtr<AMythicHarvestReplicationCell> *Existing =
            AuthorityCells.Find(Coordinate);
        const bool bHadLiveCell = Existing && Existing->IsValid();
        if (!FindOrCreateCell(
                Coordinate,
                FVector(0.0, 0.0,
                        static_cast<double>(
                            RequiredReplicationCells.FindChecked(
                                Coordinate))))) {
            DiscardPreflightCreatedReplicationCells();
            OutDiagnosticCode = TEXT("HarvestReplicationCellUnavailable");
            return false;
        }
        if (!bHadLiveCell) {
            PreflightCreatedAuthorityCellCoordinates.Add(Coordinate);
        }
    }
    if (!CanResetAuthorityReplicationStream(NextStream)) {
        DiscardPreflightCreatedReplicationCells();
        OutDiagnosticCode = TEXT("HarvestPresentationStreamRotationFailed");
        return false;
    }
    PreflightWorldSnapshotFingerprint = CandidateWorldFingerprint;
    PreflightOutboxSnapshotFingerprint = CandidateOutboxFingerprint;
    PreflightOutboxSnapshotSequence = OutboxSnapshot.SnapshotSequence;
    bHasPreflightSnapshotBinding = true;
    SaveRestorePhase = ESaveRestorePhase::Preflighted;
    OutDiagnosticCode = NAME_None;
    return true;
}

void UMythicHarvestWorldSubsystem::
DiscardPreflightCreatedReplicationCells() {
    for (const FIntPoint &Coordinate :
         PreflightCreatedAuthorityCellCoordinates) {
        TWeakObjectPtr<AMythicHarvestReplicationCell> *WeakCell =
            AuthorityCells.Find(Coordinate);
        AMythicHarvestReplicationCell *Cell =
            WeakCell ? WeakCell->Get() : nullptr;
        if (Cell && Cell->GetNodeDeltas().IsEmpty()) {
            AuthorityCells.Remove(Coordinate);
            Cell->Destroy();
        }
        else if (!Cell) {
            AuthorityCells.Remove(Coordinate);
        }
    }
    PreflightCreatedAuthorityCellCoordinates.Reset();
}

bool UMythicHarvestWorldSubsystem::CanResetAuthorityReplicationStream(
    const FMythicHarvestPresentationStreamToken &NextStream) const {
    const AMythicGameState *Coordinator = PresentationCoordinator.Get();
    if (!HasReadyAuthorityPresentationCoordinator() || !Coordinator
        || FMythicHarvestPresentationStreamToken::Compare(
               NextStream, AuthorityPresentationStream)
            != EMythicHarvestPresentationStreamOrder::Newer
        || !Coordinator->CanSetHarvestPresentationStreamToken(NextStream)) {
        return false;
    }

    for (const TPair<FIntPoint, TWeakObjectPtr<AMythicHarvestReplicationCell>> &Pair :
         AuthorityCells) {
        if (const AMythicHarvestReplicationCell *Cell = Pair.Value.Get();
            Cell && !Cell->CanResetForPresentationStream(NextStream)) {
            return false;
        }
    }
    return true;
}

bool UMythicHarvestWorldSubsystem::ResetAuthorityReplicationStream(
    const FMythicHarvestPresentationStreamToken &NextStream) {
    if (!CanResetAuthorityReplicationStream(NextStream)) {
        return false;
    }

    for (auto It = AuthorityCells.CreateIterator(); It; ++It) {
        AMythicHarvestReplicationCell *Cell = It.Value().Get();
        if (!Cell) {
            It.RemoveCurrent();
            continue;
        }
        if (!Cell->ResetForPresentationStream(NextStream)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest replication cell '%s' failed a preflighted presentation-stream reset."),
                   *Cell->GetPathName());
            return false;
        }
    }

    AMythicGameState *Coordinator = PresentationCoordinator.Get();
    AuthorityPresentationStream = NextStream;
    if (!Coordinator
        || !Coordinator->SetHarvestPresentationStreamToken(NextStream)) {
        UE_LOG(Myth, Error,
               TEXT("Harvest presentation coordinator failed a preflighted stream reset."));
        return false;
    }
    return true;
}

bool UMythicHarvestWorldSubsystem::RestoreSaveSnapshot(
    const FMythicHarvestWorldSaveV1 &Snapshot,
    FName &OutDiagnosticCode) {
    UWorld *World = GetWorld();
    if (bShuttingDown || !IsAuthorityWorld(World)) {
        OutDiagnosticCode = TEXT("HarvestAuthorityWorldUnavailable");
        return false;
    }
    if (bHarvestTransactionInProgress || HasActiveNodeMutation()) {
        OutDiagnosticCode = TEXT("HarvestTransactionInProgress");
        return false;
    }
    if (SaveRestorePhase != ESaveRestorePhase::Preflighted) {
        OutDiagnosticCode = TEXT("HarvestRestoreNotPreflighted");
        return false;
    }
    FSHA256Signature CandidateWorldFingerprint{};
    if (!bHasPreflightSnapshotBinding
        || !BuildHarvestWorldSnapshotFingerprint(
            Snapshot, CandidateWorldFingerprint)
        || FMemory::Memcmp(
               CandidateWorldFingerprint.Signature,
               PreflightWorldSnapshotFingerprint.Signature,
               UE_ARRAY_COUNT(
                   CandidateWorldFingerprint.Signature)) != 0) {
        OutDiagnosticCode = TEXT("HarvestRestorePayloadBindingMismatch");
        return false;
    }
    if (!ValidateHarvestWorldSnapshotForDeployment(
            Snapshot, OutDiagnosticCode)) {
        return false;
    }

    const UMythicHarvestRewardOutboxSubsystem *RewardOutbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    if (!RewardOutbox) {
        OutDiagnosticCode = TEXT("HarvestRewardOutboxUnavailable");
        return false;
    }
    if (PreflightOutboxSnapshotSequence == 0
        || RewardOutbox->GetLastIssuedWorldSnapshotSequence()
            != PreflightOutboxSnapshotSequence) {
        OutDiagnosticCode = TEXT("HarvestRestoreOutboxBindingMismatch");
        return false;
    }
    FSHA256Signature InstalledOutboxFingerprint{};
    if (!RewardOutbox->TryGetRestoredSaveSnapshotFingerprint(
            InstalledOutboxFingerprint)
        || FMemory::Memcmp(
               InstalledOutboxFingerprint.Signature,
               PreflightOutboxSnapshotFingerprint.Signature,
               UE_ARRAY_COUNT(
                   InstalledOutboxFingerprint.Signature)) != 0) {
        OutDiagnosticCode = TEXT("HarvestRestoreOutboxPayloadMismatch");
        return false;
    }
    for (const FMythicSavedHarvestNodeV1 &Saved : Snapshot.Nodes) {
        const FMythicHarvestNodeId NodeId(Saved.NodeGuid);
        if (const FRuntimeNode *Resident = Nodes.Find(NodeId);
            Resident && Resident->Provider.IsValid()
            && (!Resident->bHasReplicationCellCoordinate
                || Resident->ReplicationCellCoordinate
                    != Saved.ReplicationCellCoordinate)) {
            OutDiagnosticCode = TEXT("HarvestSpatialRoutingMismatch");
            return false;
        }
        if (Saved.State == EMythicHarvestNodeState::Available) {
            uint32 ExpectedGeneration = 0;
            if (!RewardOutbox->TryResolveNextGeneration(
                    Saved.WorldEpoch, NodeId, ExpectedGeneration)
                || ExpectedGeneration != Saved.Generation) {
                OutDiagnosticCode =
                    TEXT("HarvestPartialGenerationMismatch");
                UE_LOG(Myth, Error,
                       TEXT("Harvest restore rejected partial node %s generation %u because durable completion high-water expects generation %u."),
                       *Saved.NodeGuid.ToString(
                           EGuidFormats::DigitsWithHyphens),
                       Saved.Generation, ExpectedGeneration);
                return false;
            }
        }
        else {
            uint32 HighestKnownGeneration = 0;
            if (!RewardOutbox->TryGetHighestKnownGeneration(
                    Saved.WorldEpoch, NodeId,
                    HighestKnownGeneration)
                || HighestKnownGeneration != Saved.Generation) {
                OutDiagnosticCode =
                    TEXT("HarvestCompletionHighWaterMismatch");
                UE_LOG(Myth, Error,
                       TEXT("Harvest restore rejected unavailable node %s generation %u because installed completion high-water is %u."),
                       *Saved.NodeGuid.ToString(
                           EGuidFormats::DigitsWithHyphens),
                       Saved.Generation, HighestKnownGeneration);
                return false;
            }
        }
    }

    // Preflight already reserved every providerless spatial bucket. Verify that no engine-side teardown invalidated
    // that reservation before crossing the first live-state mutation boundary.
    for (const FMythicSavedHarvestNodeV1 &Saved : Snapshot.Nodes) {
        const TWeakObjectPtr<AMythicHarvestReplicationCell> *WeakCell =
            AuthorityCells.Find(Saved.ReplicationCellCoordinate);
        if (!WeakCell || !WeakCell->IsValid()) {
            OutDiagnosticCode = TEXT("HarvestReplicationCellUnavailable");
            return false;
        }
    }

    FMythicHarvestPresentationStreamToken RotatedPresentationStream;
    FMythicHarvestPresentationStreamToken::TryAdvance(
        AuthorityPresentationStream, RotatedPresentationStream);
    if (!RotatedPresentationStream.IsValid()
        || !CanResetAuthorityReplicationStream(
            RotatedPresentationStream)) {
        OutDiagnosticCode = TEXT("HarvestPresentationStreamRotationFailed");
        return false;
    }

    // Available nodes are intentionally absent from the world snapshot, but
    // their completed generations remain in the reward outbox. Resolve every
    // currently resident implicit node from that durable high-water before
    // touching live state; restarting any of them at generation one would
    // alias an earlier completion and permanently reject its next harvest.
    TSet<FMythicHarvestNodeId> SavedNodeIds;
    SavedNodeIds.Reserve(Snapshot.Nodes.Num());
    for (const FMythicSavedHarvestNodeV1 &Saved : Snapshot.Nodes) {
        SavedNodeIds.Add(FMythicHarvestNodeId(Saved.NodeGuid));
    }
    TMap<FMythicHarvestNodeId, uint32> ImplicitAvailableGenerations;
    ImplicitAvailableGenerations.Reserve(Nodes.Num());
    for (const TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        if (!Pair.Value.Provider.IsValid()
            || SavedNodeIds.Contains(Pair.Key)) {
            continue;
        }
        uint32 NextGeneration = 0;
        if (!RewardOutbox->TryResolveNextGeneration(
                Snapshot.WorldEpoch, Pair.Key, NextGeneration)) {
            OutDiagnosticCode = TEXT("HarvestGenerationResolutionFailed");
            UE_LOG(Myth, Error,
                   TEXT("Harvest restore could not resolve a never-issued generation for implicit available node %s."),
                   *Pair.Key.GetGuid().ToString());
            return false;
        }
        ImplicitAvailableGenerations.Add(Pair.Key, NextGeneration);
    }

    // Replace atomically on the game thread only after every row has passed pure validation. Providers already
    // resident are reset to their cooked available defaults; rows for streamed-out providers remain in Nodes as
    // identity-only staging records and are reconciled when RefreshResourceProvider supplies the new runtime id.
    bWorldReady = false;
    AttackCycles.Reset();
    RespawnQueue.Reset();
    if (!ResetAuthorityReplicationStream(RotatedPresentationStream)) {
        OutDiagnosticCode = TEXT("HarvestPresentationStreamRotationFailed");
        return false;
    }

    for (auto It = Nodes.CreateIterator(); It; ++It) {
        FRuntimeNode &Node = It.Value();
        if (!Node.Provider.IsValid()) {
            It.RemoveCurrent();
            continue;
        }
        Node.State = EMythicHarvestNodeState::Available;
        Node.RemainingWork = Node.MaximumWork;
        Node.Generation = SavedNodeIds.Contains(Node.NodeId)
            ? 1u
            : ImplicitAvailableGenerations.FindChecked(Node.NodeId);
        Node.Revision = 1;
        Node.RespawnServerDeadline = 0.0;
        Node.ClaimOwner = FMythicHarvestClaimIdentity();
        Node.ClaimExpiryServerTime = 0.0;
        Node.Contributors.Reset();
        Node.bMutationInProgress = false;
    }

    const double ServerNow = World->GetTimeSeconds();
    for (const FMythicSavedHarvestNodeV1 &Saved : Snapshot.Nodes) {
        const FMythicHarvestNodeId NodeId(Saved.NodeGuid);
        FRuntimeNode *Node = Nodes.Find(NodeId);
        if (!Node) {
            FRuntimeNode Staged;
            Staged.NodeId = NodeId;
            Staged.ReplicationCellCoordinate =
                Saved.ReplicationCellCoordinate;
            Staged.bHasReplicationCellCoordinate = true;
            const UMythicHarvestSettings *Settings =
                GetDefault<UMythicHarvestSettings>();
            const double GridSize = Settings
                ? FMath::Max(
                    100.0,
                    static_cast<double>(
                        Settings->ReplicationGridSizeCentimeters))
                : 20000.0;
            Staged.OriginalWorldLocation = FVector(
                (static_cast<double>(
                     Saved.ReplicationCellCoordinate.X) + 0.5)
                    * GridSize,
                (static_cast<double>(
                     Saved.ReplicationCellCoordinate.Y) + 0.5)
                    * GridSize,
                static_cast<double>(Saved.ReplicationCellCenterZ));
            Node = &Nodes.Add(NodeId, MoveTemp(Staged));
            UE_LOG(
                Myth, Warning,
                TEXT("Harvest.UnresolvedRestore NodeId=%s State=%u RemainingRespawnSeconds=%.3f Policy=RetainUntilDeadlineThenImplicitAvailable"),
                *Saved.NodeGuid.ToString(EGuidFormats::DigitsWithHyphens),
                static_cast<uint8>(Saved.State),
                Saved.RemainingRespawnSeconds);
        }
        else if (!Node->bHasReplicationCellCoordinate
                 || Node->ReplicationCellCoordinate
                     != Saved.ReplicationCellCoordinate) {
            UE_LOG(Myth, Fatal,
                   TEXT("Prevalidated harvest restore encountered a live spatial-routing mismatch for node %s."),
                   *Saved.NodeGuid.ToString(
                       EGuidFormats::DigitsWithHyphens));
        }
        Node->Generation = Saved.Generation;
        Node->Revision = Saved.Revision;
        Node->State = Saved.State;
        Node->ClaimOwner = FMythicHarvestClaimIdentity();
        Node->ClaimExpiryServerTime = 0.0;
        Node->Contributors.Reset();
        Node->bMutationInProgress = false;
        if (Saved.State == EMythicHarvestNodeState::Available) {
            const FMythicHarvestWork SavedMaximum =
                FMythicHarvestWork::FromQuanta(
                    Saved.CapturedMaximumWorkQuanta);
            const FMythicHarvestWork SavedRemaining =
                FMythicHarvestWork::FromQuanta(
                    Saved.RemainingWorkQuanta);
            Node->MaximumWork = SavedMaximum;
            Node->RemainingWork = SavedRemaining;
            Node->RespawnServerDeadline = 0.0;
            for (const FMythicSavedHarvestContributorV1 &SavedContributor :
                 Saved.Contributors) {
                FMythicHarvestParticipantSnapshot Participant;
                Participant.ContributorKey =
                    SavedContributor.ContributorKey;
                Participant.ContributionQuanta =
                    SavedContributor.ContributionQuanta;
                Participant.ItemLevel = SavedContributor.ItemLevel;
                Participant.QuantityMultiplierQuanta =
                    SavedContributor.QuantityMultiplierQuanta;
                Participant.ProficiencyLevel =
                    SavedContributor.ProficiencyLevel;
                Participant.WorkRewardContract =
                    SavedContributor.WorkRewardContract;
                Node->Contributors.Add(
                    Participant.ContributorKey, MoveTemp(Participant));
            }
        }
        else {
            Node->RemainingWork = FMythicHarvestWork();
            Node->RespawnServerDeadline =
                ServerNow + Saved.RemainingRespawnSeconds;
            ScheduleRespawn(*Node);
        }
    }

    // The world epoch is persistent reward identity, not a connection/session
    // nonce. Keeping it stable preserves exactly-once receipts across rollback;
    // the independent presentation stream above is what rotates for clients.
    WorldEpoch = Snapshot.WorldEpoch;
    NextRewardOutboxRetryServerTime = ServerNow;

    TMap<UMythicResourceISM *,
         TArray<FMythicHarvestNodePresentationUpdate>> RestoreUpdatesByProvider;
    for (TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        FRuntimeNode &Node = Pair.Value;
        if (UMythicResourceISM *Provider = Node.Provider.Get()) {
            RestoreUpdatesByProvider.FindOrAdd(Provider).Add(
                {Node.NodeId,
                 Node.State == EMythicHarvestNodeState::Available});
        }
    }
    for (TPair<UMythicResourceISM *,
               TArray<FMythicHarvestNodePresentationUpdate>> &Pair :
         RestoreUpdatesByProvider) {
        if (!Pair.Key->ApplyNodeAvailabilityBatch(Pair.Value)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest provider '%s' failed its restored-presentation batch and was quarantined for retry."),
                   *Pair.Key->GetPathName());
        }
    }

    for (TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        FRuntimeNode &Node = Pair.Value;
        if (Node.State != EMythicHarvestNodeState::Available
            || Node.RemainingWork != Node.MaximumWork) {
            if (!PublishNodeDelta(Node)) {
                UE_LOG(Myth, Fatal,
                       TEXT("Harvest restore could not publish prevalidated node %s."),
                       *Node.NodeId.GetGuid().ToString());
            }
        }
    }

    SaveRestorePhase = ESaveRestorePhase::Applied;
    PreflightCreatedAuthorityCellCoordinates.Reset();
    PreflightOutboxSnapshotSequence = 0;
    bHasPreflightSnapshotBinding = false;
    OutDiagnosticCode = NAME_None;
    return true;
}

bool UMythicHarvestWorldSubsystem::ResolveHarvestTarget(
    UMythicResourceISM &Provider, const int32 CurrentInstanceIndex,
    FPrimitiveInstanceId &OutRuntimeInstanceId,
    FMythicHarvestNodeId &OutNodeId, uint32 &OutGeneration) const {
    OutRuntimeInstanceId = FPrimitiveInstanceId();
    OutNodeId = FMythicHarvestNodeId();
    OutGeneration = 0;
    if (!bWorldReady || bShuttingDown || !IsAuthorityWorld(GetWorld())
        || Provider.GetWorld() != GetWorld()
        || !Provider.ResolveAuthoritativeHitInstance(
            CurrentInstanceIndex, OutRuntimeInstanceId, OutNodeId)) {
        return false;
    }
    const FRuntimeNode *Node = Nodes.Find(OutNodeId);
    if (!Node || Node->Provider.Get() != &Provider
        || Node->RuntimeInstanceId != OutRuntimeInstanceId) {
        OutRuntimeInstanceId = FPrimitiveInstanceId();
        OutNodeId = FMythicHarvestNodeId();
        return false;
    }
    OutGeneration = Node->Generation;
    return true;
}

bool UMythicHarvestWorldSubsystem::BeginAttackCycle(
    UMythicWeaponAttackAbility &Ability, const UAttackFragment &AttackFragment,
    const FGameplayAbilitySpecHandle AbilitySpecHandle,
    FMythicHarvestAttackCycleToken &OutToken) {
    OutToken = FMythicHarvestAttackCycleToken();
    UWorld *World = GetWorld();
    UMythicItemInstance *Item = AttackFragment.GetOwningItemInstance();
    UAbilitySystemComponent *ASC =
        AttackFragment.GetOwningAbilitySystemComponent();
    const FGameplayAbilitySpec *Spec =
        ASC ? ASC->FindAbilitySpecFromHandle(AbilitySpecHandle) : nullptr;
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const UAnimMontage *AttackMontage =
        AttackFragment.AttackConfig.AttackMontage;
    const double PlayRate = static_cast<double>(
        Ability.GetClampedAttackSpeedPlayRate());
    const double CadenceTolerance = Settings
        ? static_cast<double>(Settings->CadenceToleranceSeconds) : -1.0;
    if (!bWorldReady || bHarvestTransactionInProgress || bShuttingDown
        || !IsAuthorityWorld(World)
        || !Ability.IsActive() || !Item || !Item->GetItemInstanceGuid().IsValid()
        || !Spec || !Spec->IsActive() || Spec->SourceObject.Get() != &AttackFragment
        || Spec->GetPrimaryInstance() != &Ability || !AttackMontage
        || !FMath::IsFinite(PlayRate) || PlayRate <= 0.0
        || !FMath::IsFinite(CadenceTolerance) || CadenceTolerance < 0.0) {
        return false;
    }

    uint64 Serial = NextAttackCycleSerial++;
    if (Serial == 0) {
        Serial = NextAttackCycleSerial++;
    }
    OutToken.ServerSerial = Serial;
    OutToken.AbilitySpecHandle = AbilitySpecHandle;
    OutToken.SourceItemGuid = Item->GetItemInstanceGuid();

    FAttackCycleState State;
    State.Ability = &Ability;
    State.AttackFragment = const_cast<UAttackFragment *>(&AttackFragment);
    State.IssuedServerTime = World->GetTimeSeconds();
    if (!FMythicHarvestCadencePolicy::TryCalculateExpiry(
            State.IssuedServerTime,
            static_cast<double>(AttackMontage->GetPlayLength()), PlayRate,
            CadenceTolerance, State.ExpiresServerTime)) {
        OutToken = FMythicHarvestAttackCycleToken();
        return false;
    }
    AttackCycles.Add(OutToken, MoveTemp(State));
    return true;
}

void UMythicHarvestWorldSubsystem::EndAttackCycle(
    const FMythicHarvestAttackCycleToken &Token,
    const UMythicWeaponAttackAbility *ExpectedAbility) {
    if (FAttackCycleState *State = AttackCycles.Find(Token);
        State && (!ExpectedAbility || State->Ability.Get() == ExpectedAbility)) {
        AttackCycles.Remove(Token);
    }
}

FMythicHarvestResult UMythicHarvestWorldSubsystem::MakeRejectedResult(
    const EMythicHarvestRejectReason Reason, const FRuntimeNode *Node,
    const FMythicHarvestRequest & /*Request*/) {
    FMythicHarvestResult Result;
    Result.Outcome = EMythicHarvestOutcome::Rejected;
    Result.RejectReason = Reason;
    Result.ServerSequence = IssueServerSequence();
    if (Node) {
        Result.NodeId = Node->NodeId;
        Result.RemainingWork = Node->RemainingWork;
        Result.MaxWork = Node->MaximumWork;
        Result.Generation = Node->Generation;
        Result.Revision = Node->Revision;
    }
    return Result;
}

uint64 UMythicHarvestWorldSubsystem::IssueServerSequence() {
    uint64 Issued = NextServerSequence++;
    if (Issued == 0) {
        Issued = NextServerSequence++;
    }
    return Issued;
}

bool UMythicHarvestWorldSubsystem::ValidateAttackProvenance(
    const FMythicHarvestRequest &Request, const FAttackCycleState &Cycle,
    AMythicPlayerController *&OutController,
    EMythicHarvestRejectReason &OutReason) const {
    OutController = nullptr;
    OutReason = EMythicHarvestRejectReason::InvalidSource;

    APawn *Avatar = Request.AuthorityAvatar;
    AMythicPlayerController *Controller =
        Cast<AMythicPlayerController>(Request.AuthorityController);
    UAttackFragment *AttackFragment = Request.SourceAttackFragment;
    UMythicWeaponAttackAbility *Ability = Request.ActiveAttackAbility;
    if (!Avatar || !Controller || Controller->GetPawn() != Avatar
        || Avatar->GetController() != Controller || !AttackFragment || !Ability
        || Cycle.Ability.Get() != Ability
        || Cycle.AttackFragment.Get() != AttackFragment || !Ability->IsActive()) {
        return false;
    }

    UMythicItemInstance *Item = AttackFragment->GetOwningItemInstance();
    if (!Item || Item->GetInventoryOwner() != Controller
        || Request.AttackCycleToken.SourceItemGuid
            != Item->GetItemInstanceGuid()) {
        return false;
    }

    UAbilitySystemComponent *ASC = Controller->GetAbilitySystemComponent();
    const FGameplayAbilitySpec *Spec = ASC
        ? ASC->FindAbilitySpecFromHandle(
              Request.AttackCycleToken.AbilitySpecHandle)
        : nullptr;
    if (!Spec || !Spec->IsActive()
        || Spec->SourceObject.Get() != AttackFragment
        || Spec->GetPrimaryInstance() != Ability) {
        return false;
    }

    OutController = Controller;
    OutReason = EMythicHarvestRejectReason::None;
    return true;
}

bool UMythicHarvestWorldSubsystem::ResolveEquippedHarvestTool(
    AMythicPlayerController &Controller,
    const UMythicHarvestToolTypeDefinition *RequiredToolType,
    UMythicItemInstance *&OutTool, const UHarvestToolFragment *&OutHarvestTool,
    UDurabilityFragment *&OutDurability,
    EMythicHarvestRejectReason &OutReason) const {
    OutTool = nullptr;
    OutHarvestTool = nullptr;
    OutDurability = nullptr;
    OutReason = EMythicHarvestRejectReason::NoTool;
    if (!RequiredToolType) {
        OutReason = EMythicHarvestRejectReason::InvalidSource;
        return false;
    }

    for (UMythicInventoryComponent *Inventory :
         Controller.GetAllInventoryComponents()) {
        if (!Inventory) {
            continue;
        }
        for (const FMythicInventorySlotEntry &Slot : Inventory->GetAllSlots()) {
            if (!Slot.IsGearSlot()) {
                continue;
            }
            UMythicItemInstance *Item = Slot.SlottedItemInstance.Get();
            const UHarvestToolFragment *Fragment =
                UHarvestToolFragment::FindOnItem(Item);
            if (!Fragment || Fragment->ToolType.Get() != RequiredToolType
                || Item->GetInventoryOwner() != &Controller) {
                continue;
            }
            // Two gear slots holding the same tool family cannot name one wear
            // target, so authored duplication rejects instead of picking one.
            if (OutTool) {
                OutTool = nullptr;
                OutHarvestTool = nullptr;
                OutReason = EMythicHarvestRejectReason::InvalidSource;
                return false;
            }
            OutTool = Item;
            OutHarvestTool = Fragment;
        }
    }
    if (!OutTool) {
        return false;
    }

    OutDurability = const_cast<UDurabilityFragment *>(
        OutTool->GetFragment<UDurabilityFragment>());
    if (!OutDurability) {
        OutTool = nullptr;
        OutHarvestTool = nullptr;
        OutReason = EMythicHarvestRejectReason::InvalidSource;
        return false;
    }
    OutReason = EMythicHarvestRejectReason::None;
    return true;
}

bool UMythicHarvestWorldSubsystem::ValidateRangeAndLineOfSight(
    const FMythicHarvestRequest &Request, const FRuntimeNode &Node,
    EMythicHarvestRejectReason &OutReason) const {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const APawn *Avatar = Request.AuthorityAvatar;
    const UMythicResourceISM *Resource = Request.TargetResource;
    if (!Settings || !Avatar || !Resource
        || Request.AuthoritativeHit.GetComponent() != Resource
        || !Request.AuthoritativeHit.bBlockingHit
        || Request.AuthoritativeHit.Item == INDEX_NONE
        || !IsFiniteLocation(Request.AuthoritativeHit.ImpactPoint)) {
        OutReason = EMythicHarvestRejectReason::InvalidSource;
        return false;
    }
    FPrimitiveInstanceId ContactPrimitive;
    FMythicHarvestNodeId ContactNode;
    if (!Resource->ResolveAuthoritativeHitInstance(
            Request.AuthoritativeHit.Item, ContactPrimitive, ContactNode)
        || ContactPrimitive != Node.RuntimeInstanceId
        || ContactNode != Node.NodeId) {
        OutReason = EMythicHarvestRejectReason::InvalidInstance;
        return false;
    }
    const FVector Impact = Request.AuthoritativeHit.ImpactPoint;
    const double Range = Settings->AuthoritativeRangeCentimeters;
    if (!FMath::IsFinite(Range) || Range <= 0.0
        || FVector::DistSquared(Avatar->GetActorLocation(), Impact)
            > FMath::Square(Range)) {
        OutReason = EMythicHarvestRejectReason::OutOfRange;
        return false;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    Avatar->GetActorEyesViewPoint(ViewLocation, ViewRotation);
    FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(MythicHarvestLOS),
                                      Settings->bTraceComplexLineOfSight);
    QueryParams.AddIgnoredActor(Avatar);
    FHitResult SightHit;
    const FVector SightDirection = (Impact - ViewLocation).GetSafeNormal();
    const FVector TraceEnd = Impact + SightDirection * 5.0;
    if (SightDirection.IsNearlyZero()
        || !GetWorld()->LineTraceSingleByChannel(
            SightHit, ViewLocation, TraceEnd,
            Settings->LineOfSightTraceChannel, QueryParams)
        || SightHit.GetComponent() != Resource) {
        OutReason = EMythicHarvestRejectReason::NoLineOfSight;
        return false;
    }
    FPrimitiveInstanceId SightPrimitive;
    FMythicHarvestNodeId SightNode;
    if (SightHit.Item == INDEX_NONE
        || !Resource->ResolveAuthoritativeHitInstance(
            SightHit.Item, SightPrimitive, SightNode)
        || SightPrimitive != Node.RuntimeInstanceId
        || SightNode != Node.NodeId) {
        OutReason = EMythicHarvestRejectReason::NoLineOfSight;
        return false;
    }
    OutReason = EMythicHarvestRejectReason::None;
    return true;
}

FMythicHarvestClaimIdentity
UMythicHarvestWorldSubsystem::ResolveClaimIdentity(
    AMythicPlayerController &Controller) const {
    UWorld *World = GetWorld();
    AMythicPlayerState *PlayerState =
        Controller.GetPlayerState<AMythicPlayerState>();
    UMythicPlayerRegistrySubsystem *Registry = World
        ? World->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr;
    if (!World || !Controller.HasAuthority() || Controller.GetWorld() != World
        || !PlayerState || !Registry) {
        return FMythicHarvestClaimIdentity();
    }

    FString RegisteredControllerKey;
    FString PersistentPlayerKey;
    if (!Registry->GetKeyForObject(&Controller, RegisteredControllerKey)
        || !FMythicHarvestParticipantIdentityPolicy::
            TryResolveReadyContributorKey(
                PlayerState->GetPersistentCharacterId(),
                PlayerState->GetCanonicalPlayerKey(), RegisteredControllerKey,
                PersistentPlayerKey)
        || Registry->GetPlayerStateForKey(PersistentPlayerKey) != PlayerState
        || Registry->GetPlayerControllerForKey(PersistentPlayerKey)
            != &Controller) {
        return FMythicHarvestClaimIdentity();
    }

    const UMythicHarvestClaimMembershipSubsystem *Membership =
        World->GetSubsystem<UMythicHarvestClaimMembershipSubsystem>();
    FGuid PartyId;
    if (Membership
        && Membership->TryResolveParty(PersistentPlayerKey, PartyId)) {
        return FMythicHarvestClaimIdentity::MakeParty(PartyId);
    }
    return FMythicHarvestClaimIdentity::MakePlayer(PersistentPlayerKey);
}

FMythicHarvestResult UMythicHarvestWorldSubsystem::TryApplyHarvest(
    const FMythicHarvestRequest &Request) {
    UWorld *World = GetWorld();
    if (bHarvestTransactionInProgress) {
        return MakeRejectedResult(
            EMythicHarvestRejectReason::CadenceRejected, nullptr, Request);
    }
    if (!bWorldReady || bSaveCaptureInProgress || bShuttingDown
        || !IsAuthorityWorld(World)) {
        FMythicHarvestResult Result = MakeRejectedResult(
            EMythicHarvestRejectReason::WorldNotReady, nullptr, Request);
        SendFeedback(Cast<AMythicPlayerController>(
                         Request.AuthorityController),
                     Request, nullptr, Result);
        return Result;
    }

    UMythicResourceISM *Resource = Request.TargetResource;
    FMythicHarvestNodeId ResolvedNodeId;
    if (!Resource || Resource->GetWorld() != World
        || !Resource->ResolveStableNodeId(Request.RuntimeInstanceId,
                                          ResolvedNodeId)) {
        FMythicHarvestResult Result = MakeRejectedResult(
            EMythicHarvestRejectReason::InvalidInstance, nullptr, Request);
        SendFeedback(Cast<AMythicPlayerController>(
                         Request.AuthorityController),
                     Request, nullptr, Result);
        return Result;
    }

    FRuntimeNode *Node = Nodes.Find(ResolvedNodeId);
    auto Reject = [this, &Request, Node](
                      const EMythicHarvestRejectReason Reason,
                      AMythicPlayerController *Controller = nullptr) {
        FMythicHarvestResult Result = MakeRejectedResult(Reason, Node, Request);
        SendFeedback(Controller ? Controller
                                : Cast<AMythicPlayerController>(
                                      Request.AuthorityController),
                     Request, Node, Result);
        return Result;
    };

    if (!Node || Node->Provider.Get() != Resource
        || Node->RuntimeInstanceId != Request.RuntimeInstanceId) {
        return Reject(EMythicHarvestRejectReason::InvalidInstance);
    }
    if (Request.ExpectedGeneration != Node->Generation) {
        return Reject(EMythicHarvestRejectReason::GenerationMismatch);
    }
    if (Node->State != EMythicHarvestNodeState::Available
        || Node->RemainingWork.IsZero()) {
        return Reject(EMythicHarvestRejectReason::NodeDepleted);
    }
    if (Node->bMutationInProgress) {
        return Reject(EMythicHarvestRejectReason::CadenceRejected);
    }

    FAttackCycleState *Cycle = AttackCycles.Find(Request.AttackCycleToken);
    if (!Cycle) {
        return Reject(EMythicHarvestRejectReason::InvalidSource);
    }
    const double ServerNow = World->GetTimeSeconds();
    if (FMythicHarvestCadencePolicy::IsExpired(
            ServerNow, Cycle->ExpiresServerTime)) {
        AttackCycles.Remove(Request.AttackCycleToken);
        return Reject(EMythicHarvestRejectReason::CadenceRejected);
    }

    AMythicPlayerController *Controller = nullptr;
    EMythicHarvestRejectReason RejectReason =
        EMythicHarvestRejectReason::InvalidSource;
    if (!ValidateAttackProvenance(Request, *Cycle, Controller, RejectReason)) {
        return Reject(RejectReason, Controller);
    }

    UMythicHarvestableDefinition *Definition = Node->Definition.Get();
    if (!Definition) {
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }

    // The swing comes from the combat weapon; the tool that gates and pays for
    // this node is whatever sits in its own gear slot at the moment of impact.
    UMythicItemInstance *Tool = nullptr;
    const UHarvestToolFragment *HarvestTool = nullptr;
    UDurabilityFragment *Durability = nullptr;
    if (!ResolveEquippedHarvestTool(*Controller,
                                    Definition->RequiredToolType.Get(), Tool,
                                    HarvestTool, Durability, RejectReason)) {
        return Reject(RejectReason, Controller);
    }
    if (HarvestTool->ToolTier < Definition->MinimumToolTier) {
        return Reject(EMythicHarvestRejectReason::ToolTierTooLow, Controller);
    }
    if (Durability->IsBroken()) {
        return Reject(EMythicHarvestRejectReason::ToolBroken, Controller);
    }

    const FMythicHarvestClaimIdentity RequestClaim =
        ResolveClaimIdentity(*Controller);
    if (!RequestClaim.IsValid()) {
        return Reject(EMythicHarvestRejectReason::InvalidSource, Controller);
    }
    if (Node->ClaimOwner.IsValid() && !(Node->ClaimOwner == RequestClaim)
        && Node->ClaimExpiryServerTime > ServerNow) {
        return Reject(EMythicHarvestRejectReason::ClaimedByOther, Controller);
    }
    if (!ValidateRangeAndLineOfSight(Request, *Node, RejectReason)) {
        return Reject(RejectReason, Controller);
    }
    if (Cycle->ConsumedNodes.Contains(Node->NodeId)
        || Cycle->ConsumedNodes.Num()
            >= FMath::Max(1, HarvestTool->MaxNodesPerCycle)) {
        return Reject(EMythicHarvestRejectReason::CadenceRejected, Controller);
    }

    UAbilitySystemComponent *ASC = Controller->GetAbilitySystemComponent();
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!ASC || !Settings) {
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }
    const double RawMultiplier = ASC->GetNumericAttribute(
        UMythicAttributeSet_Utility::GetHarvestWorkMultiplierAttribute());
    double WorkMultiplier = 0.0;
    if (!Settings->TryClampHarvestWorkMultiplier(RawMultiplier,
                                                  WorkMultiplier)) {
        return Reject(EMythicHarvestRejectReason::InvalidSource, Controller);
    }
    FMythicHarvestWork RequestedWork;
    if (!FMythicHarvestWork::TryFromWorkUnits(
            static_cast<double>(HarvestTool->BaseWork) * WorkMultiplier,
            RequestedWork)
        || RequestedWork.IsZero()) {
        return Reject(EMythicHarvestRejectReason::InvalidSource, Controller);
    }

    const FMythicHarvestWork Applied =
        FMythicHarvestWork::Min(RequestedWork, Node->RemainingWork);
    const bool bWillComplete = Applied == Node->RemainingWork;

    // Freeze the participant only after the persistent character id and the
    // registry's reverse mapping agree. This is deliberately before every
    // fallible prepare step and every gameplay mutation: a player still in
    // asynchronous character load cannot create a transient contribution row.
    FMythicHarvestParticipantSnapshot AcceptedWorkSnapshot;
    if (!TryCaptureParticipantSnapshot(*Controller, *Definition,
                                       Applied.GetQuanta(),
                                       AcceptedWorkSnapshot)) {
        return Reject(EMythicHarvestRejectReason::InvalidSource, Controller);
    }
    UMythicHarvestRewardOutboxSubsystem *RewardOutbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    if (!RewardOutbox) {
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }
    if (HarvestTool->DurabilityWearPerAcceptedHit > 0
        && (Durability->GetMaxDurability() <= 0
            || !RewardOutbox->CanAccrueDurabilityCost(
                AcceptedWorkSnapshot.ContributorKey))) {
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }
    TMap<FString, FMythicHarvestParticipantSnapshot> StagedContributors =
        Node->Contributors;
    if (!FMythicHarvestParticipantLedger::TryAccumulate(
            StagedContributors, AcceptedWorkSnapshot)) {
        return Reject(EMythicHarvestRejectReason::InvalidSource, Controller);
    }

    // From this point through post-commit dispatch, saving and recursive harvest
    // entry must fail closed. The node flag also makes the snapshot gate robust
    // if a future native caller bypasses the subsystem-wide transaction gate.
    TGuardValue<bool> TransactionGuard(bHarvestTransactionInProgress, true);
    TGuardValue<bool> MutationGuard(Node->bMutationInProgress, true);

    // Spawning/configuring a spatial replicator is a fallible prepare step.
    // Complete it before durability, progression, outbox, or node-state
    // mutation so an exhausted actor channel leaves the transaction untouched.
    if (!Node->bHasReplicationCellCoordinate
        || !FindOrCreateCell(Node->ReplicationCellCoordinate,
                          Node->OriginalWorldLocation)) {
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }

    FMythicPreparedHarvestCompletion PreparedCompletion;
    FMythicPreparedHarvestWorkDelivery PreparedWorkDelivery;
    FMythicPreparedHarvestDurabilityCost PreparedDurabilityCost;
    int32 ReservedCompletionRows = 0;

    // Freeze every random decision as a pure prepare step. This deliberately
    // does not touch KnownCompletions, pending delivery arrays, inventories,
    // progression, quests, or callbacks while the node is still Available.
    if (bWillComplete) {
        const int64 MinimumContribution = FMath::CeilToInt64(
            static_cast<double>(Node->MaximumWork.GetQuanta())
            * static_cast<double>(Settings->MinimumContributionFraction));
        TArray<FMythicHarvestParticipantSnapshot> EligibleContributors;
        if (!FMythicHarvestParticipantLedger::BuildEligibleSnapshots(
                StagedContributors, MinimumContribution,
                EligibleContributors)
            || EligibleContributors.IsEmpty()) {
            return Reject(EMythicHarvestRejectReason::InvalidSource,
                          Controller);
        }
        if (!RewardOutbox->PrepareCompletion(
                    *Definition, WorldEpoch, Node->NodeId, Node->Generation,
                    EligibleContributors,
                    PreparedCompletion).WasPrepared()) {
            return Reject(EMythicHarvestRejectReason::WorldNotReady,
                          Controller);
        }
        ReservedCompletionRows = PreparedCompletion.Grants.Num()
            + PreparedCompletion.CompletionDeliveries.Num();
    }

    FName WorkDeliveryDiagnostic;
    const FMythicHarvestParticipantSnapshot &CumulativeParticipant =
        StagedContributors.FindChecked(
            AcceptedWorkSnapshot.ContributorKey);
    if (!RewardOutbox->PrepareAppliedWorkDelivery(
            *Definition, WorldEpoch, Node->NodeId, Node->Generation,
            CumulativeParticipant, Applied.ToWorkUnits(), PreparedWorkDelivery,
            WorkDeliveryDiagnostic, ReservedCompletionRows)) {
        UE_LOG(Myth, Warning,
               TEXT("Harvest work delivery prepare rejected node %s generation %u (%s)."),
               *Node->NodeId.GetGuid().ToString(), Node->Generation,
               *WorkDeliveryDiagnostic.ToString());
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }
    FMythicHarvestParticipantSnapshot &FrozenParticipant =
        StagedContributors.FindChecked(AcceptedWorkSnapshot.ContributorKey);
    const FMythicHarvestWorkRewardContract &PreparedWorkContract =
        PreparedWorkDelivery.Delivery.WorkRewardContract;
    if (!PreparedWorkContract.IsValid()
        || (!FrozenParticipant.WorkRewardContract.IsUnset()
            && !(FrozenParticipant.WorkRewardContract
                 == PreparedWorkContract))) {
        return Reject(EMythicHarvestRejectReason::WorldNotReady, Controller);
    }
    FrozenParticipant.WorkRewardContract = PreparedWorkContract;

    if (HarvestTool->DurabilityWearPerAcceptedHit > 0) {
        FName DurabilityCostDiagnostic;
        if (!RewardOutbox->PrepareDurabilityCost(
                WorldEpoch, Node->NodeId, Node->Generation,
                AcceptedWorkSnapshot.ContributorKey, *Tool,
                HarvestTool->DurabilityWearPerAcceptedHit, *Controller,
                PreparedDurabilityCost, DurabilityCostDiagnostic)) {
            UE_LOG(Myth, Warning,
                   TEXT("Harvest durability cost prepare rejected node %s generation %u (%s)."),
                   *Node->NodeId.GetGuid().ToString(), Node->Generation,
                   *DurabilityCostDiagnostic.ToString());
            return Reject(EMythicHarvestRejectReason::WorldNotReady,
                          Controller);
        }
    }

    // Internal authority commit. Nothing below this comment may reject: the
    // node, attack-cycle replay identity, and lifecycle become authoritative as
    // one game-thread state transition before any externally dispatching work.
    Node->RemainingWork = Node->RemainingWork.SubtractClamped(Applied);
    Node->Revision = AdvanceNonZeroRevision(Node->Revision);
    Node->ClaimOwner = RequestClaim;
    Node->ClaimExpiryServerTime =
        ServerNow + FMath::Max(0.001, static_cast<double>(
            Settings->SoftClaimDurationSeconds));
    Node->Contributors = MoveTemp(StagedContributors);
    Cycle->ConsumedNodes.Add(Node->NodeId);

    FMythicHarvestResult Result;
    Result.Outcome = EMythicHarvestOutcome::Accepted;
    Result.RejectReason = EMythicHarvestRejectReason::None;
    Result.AppliedWork = Applied;
    Result.RemainingWork = Node->RemainingWork;
    Result.MaxWork = Node->MaximumWork;
    Result.NodeId = Node->NodeId;
    Result.Generation = Node->Generation;
    Result.Revision = Node->Revision;
    Result.ServerSequence = IssueServerSequence();

    if (Node->RemainingWork.IsZero()) {
        Node->State = EMythicHarvestNodeState::Depleted;
        Node->RespawnServerDeadline =
            ServerNow + FMath::Max(0.0, static_cast<double>(
                Definition->RespawnDelaySeconds));
        Node->Revision = AdvanceNonZeroRevision(Node->Revision);
        Result.Outcome = EMythicHarvestOutcome::Completed;
        Result.Revision = Node->Revision;
        ScheduleRespawn(*Node);
    }

    // ISM state and spatial replication may invoke engine-facing callbacks,
    // but every authoritative field and replay latch is already committed and
    // the transaction/save gates remain held throughout. Durability is a
    // receipt-backed outbox cost below, never a direct world/character split.
    if (bWillComplete) {
        if (!Resource->ApplyNodeAvailability(Node->NodeId, false)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest provider '%s' failed to hide committed node %s; the provider was quarantined for retry."),
                   *Resource->GetPathName(),
                   *Node->NodeId.GetGuid().ToString());
        }
    }
    if (!PublishNodeDelta(*Node)) {
        UE_LOG(Myth, Fatal,
               TEXT("Committed harvest node %s generation %u revision %u could not publish its authoritative replication delta."),
               *Node->NodeId.GetGuid().ToString(), Node->Generation,
               Node->Revision);
    }

    // Publish the durable frozen plans only after node, cadence, ISM, and
    // replication commit. Combined capacity was reserved before mutation,
    // so failure here is an invariant breach and the server must fail-stop
    // rather than continue with a harvested node missing its receipts.
    if (HarvestTool->DurabilityWearPerAcceptedHit > 0
        && !RewardOutbox->CommitPreparedDurabilityCost(
            MoveTemp(PreparedDurabilityCost))) {
        UE_LOG(Myth, Fatal,
               TEXT("A prevalidated harvest durability cost failed admission after node commit for %s generation %u."),
               *Node->NodeId.GetGuid().ToString(), Node->Generation);
    }
    if (!RewardOutbox->CommitPreparedAppliedWorkDelivery(
            MoveTemp(PreparedWorkDelivery))) {
        UE_LOG(Myth, Fatal,
               TEXT("A prevalidated harvest work receipt failed admission after node commit for %s generation %u."),
               *Node->NodeId.GetGuid().ToString(), Node->Generation);
    }
    if (bWillComplete) {
        const EMythicHarvestCompletionAdmission Admission =
            RewardOutbox->CommitPreparedCompletion(
                MoveTemp(PreparedCompletion));
        const bool bOutboxCommitValid =
            Admission == EMythicHarvestCompletionAdmission::Committed;
        checkf(bOutboxCommitValid,
               TEXT("A validated harvest completion failed exact admission after node commit."));
        if (!bOutboxCommitValid) {
            UE_LOG(Myth, Fatal,
                   TEXT("A prevalidated harvest completion receipt failed admission after node commit for %s generation %u (admission %u)."),
                   *Node->NodeId.GetGuid().ToString(), Node->Generation,
                   static_cast<uint8>(Admission));
        }
    }

    FMythicHarvestPostCommitBarrier PostCommitBarrier;
    PostCommitBarrier.MarkStateCommitted();
    if (!ensureAlwaysMsgf(PostCommitBarrier.TryBeginSideEffects(),
                          TEXT("Harvest post-commit side effects attempted more than once."))) {
        return Result;
    }

    // Keep an immutable presentation copy so a progression/reward callback can
    // never invalidate the node storage subsequently read by feedback.
    const FRuntimeNode FeedbackNodeSnapshot = *Node;
    if (bWillComplete) {
        CommitCompletionChannels(*Node);
    }
    if (RewardOutbox->HasPendingWork()) {
        RewardOutbox->RetryPendingDeliveries();
    }
    SendFeedback(Controller, Request, &FeedbackNodeSnapshot, Result);
    return Result;
}

bool UMythicHarvestWorldSubsystem::TryCaptureParticipantSnapshot(
    AMythicPlayerController &Controller,
    const UMythicHarvestableDefinition &Definition,
    const int64 AppliedContributionQuanta,
    FMythicHarvestParticipantSnapshot &OutSnapshot) const {
    OutSnapshot = FMythicHarvestParticipantSnapshot();
    UWorld *World = GetWorld();
    AMythicPlayerState *PlayerState =
        Controller.GetPlayerState<AMythicPlayerState>();
    UMythicPlayerRegistrySubsystem *Registry = World
        ? World->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr;
    UAbilitySystemComponent *ASC = Controller.GetAbilitySystemComponent();
    UProficiencyComponent *Proficiencies =
        Controller.GetProficiencyComponent();
    if (!World || !Controller.HasAuthority() || Controller.GetWorld() != World
        || !PlayerState || !Registry || !ASC || !Proficiencies
        || !Definition.ProficiencyDefinition
        || AppliedContributionQuanta <= 0) {
        return false;
    }

    FString RegisteredControllerKey;
    if (!Registry->GetKeyForObject(&Controller, RegisteredControllerKey)) {
        return false;
    }

    FString ContributorKey;
    if (!FMythicHarvestParticipantIdentityPolicy::TryResolveReadyContributorKey(
            PlayerState->GetPersistentCharacterId(),
            PlayerState->GetCanonicalPlayerKey(),
            RegisteredControllerKey, ContributorKey)
        || Registry->GetPlayerStateForKey(ContributorKey) != PlayerState
        || Registry->GetPlayerControllerForKey(ContributorKey)
            != &Controller) {
        return false;
    }

    const FGameplayAttribute QuantityAttribute =
        UMythicAttributeSet_Utility::GetItemQuantityFindAttribute();
    int32 QuantityMultiplierQuanta = 0;
    int32 ProficiencyLevel = 0;
    if (!ASC->HasAttributeSetForAttribute(QuantityAttribute)
        || !FMythicHarvestRewardPlanner::TryQuantizeQuantityMultiplier(
            static_cast<double>(ASC->GetNumericAttribute(QuantityAttribute)),
            QuantityMultiplierQuanta)
        || !Proficiencies->TryGetLevelForDefinition(
            Definition.ProficiencyDefinition, ProficiencyLevel)) {
        return false;
    }

    OutSnapshot.ContributorKey = ContributorKey;
    OutSnapshot.ContributionQuanta = AppliedContributionQuanta;
    OutSnapshot.ItemLevel = FMath::Max(1, Controller.GetPlayerLevel());
    OutSnapshot.QuantityMultiplierQuanta = QuantityMultiplierQuanta;
    OutSnapshot.ProficiencyLevel = ProficiencyLevel;
    OutSnapshot.CurrentController = &Controller;
    return OutSnapshot.IsValid();
}

void UMythicHarvestWorldSubsystem::CommitCompletionChannels(
    FRuntimeNode &Node) const {
    UMythicHarvestableDefinition *Definition = Node.Definition.Get();
    if (!Definition) {
        return;
    }
    // Contributor-scoped material, completion-XP, and typed quest-credit
    // effects are frozen into the durable reward outbox before node commit.
    // Only the world-scoped pressure channel commits directly here.
    if (Definition->Pressure.bEmitPressure
        && FMath::IsFinite(Definition->Pressure.PressurePerCompletion)
        && Definition->Pressure.PressurePerCompletion > 0.0f) {
        if (UWorld *World = GetWorld()) {
            if (UMythicRegionalPressureSubsystem *Pressure =
                    World->GetSubsystem<UMythicRegionalPressureSubsystem>()) {
                Pressure->ServerRegisterHarvest(
                    Node.OriginalWorldLocation,
                    Definition->Pressure.PressurePerCompletion);
            }
        }
    }
}

uint16 UMythicHarvestWorldSubsystem::QuantizeRemainingWork(
    const FMythicHarvestWork Remaining, const FMythicHarvestWork Maximum) {
    if (Maximum.IsZero()) {
        return 0;
    }
    const double Fraction = FMath::Clamp(
        static_cast<double>(Remaining.GetQuanta())
            / static_cast<double>(Maximum.GetQuanta()),
        0.0, 1.0);
    return static_cast<uint16>(FMath::RoundToInt(Fraction * 65535.0));
}

FIntPoint UMythicHarvestWorldSubsystem::MakeCellCoordinate(
    const FVector &Location) const {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const double GridSize = Settings
        ? FMath::Max(100.0, static_cast<double>(
            Settings->ReplicationGridSizeCentimeters))
        : 20000.0;
    return FIntPoint(FMath::FloorToInt(Location.X / GridSize),
                     FMath::FloorToInt(Location.Y / GridSize));
}

AMythicHarvestReplicationCell *
UMythicHarvestWorldSubsystem::FindOrCreateCell(
    const FIntPoint &Coordinate, const FVector &NodeLocation) {
    if (TWeakObjectPtr<AMythicHarvestReplicationCell> *Existing =
            AuthorityCells.Find(Coordinate)) {
        if (AMythicHarvestReplicationCell *Cell = Existing->Get()) {
            return Cell;
        }
        AuthorityCells.Remove(Coordinate);
    }
    UWorld *World = GetWorld();
    if (!IsAuthorityWorld(World) || !AuthorityPresentationStream.IsValid()) {
        return nullptr;
    }
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const double GridSize = Settings
        ? FMath::Max(100.0, static_cast<double>(
            Settings->ReplicationGridSizeCentimeters))
        : 20000.0;
    const FVector Center((static_cast<double>(Coordinate.X) + 0.5) * GridSize,
                         (static_cast<double>(Coordinate.Y) + 0.5) * GridSize,
                         NodeLocation.Z);
    FActorSpawnParameters SpawnParameters;
    SpawnParameters.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    SpawnParameters.ObjectFlags |= RF_Transient;
    AMythicHarvestReplicationCell *Cell =
        World->SpawnActor<AMythicHarvestReplicationCell>(
            AMythicHarvestReplicationCell::StaticClass(), Center,
            FRotator::ZeroRotator, SpawnParameters);
    const double CullDistance = Settings
        ? Settings->ReplicationCullDistanceCentimeters : 50000.0;
    constexpr double HalfDiagonalScale = 0.70710678118654752440;
    const double RelevancyMargin = Settings
        ? static_cast<double>(
            Settings->ReplicationRelevancyMarginCentimeters)
        : 5000.0;
    const double MinimumCullDistance =
        GridSize * HalfDiagonalScale + RelevancyMargin;
    if (!Cell
        || !FMath::IsFinite(CullDistance)
        || !FMath::IsFinite(MinimumCullDistance)
        || CullDistance < MinimumCullDistance
        || !Cell->ConfigureSpatialCell(
            Coordinate, Center, CullDistance,
            AuthorityPresentationStream)) {
        if (Cell) {
            Cell->Destroy();
        }
        return nullptr;
    }
    AuthorityCells.Add(Coordinate, Cell);
    return Cell;
}

bool UMythicHarvestWorldSubsystem::PublishNodeDelta(FRuntimeNode &Node) {
    if (!IsAuthorityWorld(GetWorld())
        || !Node.bHasReplicationCellCoordinate) {
        return false;
    }
    AMythicHarvestReplicationCell *Cell = FindOrCreateCell(
        Node.ReplicationCellCoordinate,
        Node.OriginalWorldLocation);
    if (!Cell) {
        return false;
    }
    FMythicHarvestReplicatedNodeItem Delta;
    Delta.PresentationStream = AuthorityPresentationStream;
    Delta.NodeId = Node.NodeId;
    Delta.Generation = Node.Generation;
    Delta.Revision = Node.Revision;
    Delta.State = Node.State;
    Delta.QuantizedMaxWork = 65535;
    Delta.QuantizedRemainingWork =
        QuantizeRemainingWork(Node.RemainingWork, Node.MaximumWork);
    Delta.RespawnServerDeadline = Node.RespawnServerDeadline;
    return Cell->UpsertNodeDelta(Delta);
}

bool UMythicHarvestWorldSubsystem::RetireAuthorityCellIfOnlyAvailable(
    const FIntPoint &Coordinate,
    const FMythicHarvestNodeId &CurrentNodeId,
    TArray<FMythicHarvestNodeId> &OutAdditionalImplicitNodes) {
    const TWeakObjectPtr<AMythicHarvestReplicationCell> *WeakCell =
        AuthorityCells.Find(Coordinate);
    AMythicHarvestReplicationCell *Cell = WeakCell ? WeakCell->Get() : nullptr;
    if (!Cell) {
        return false;
    }

    TArray<FMythicHarvestNodeId> CellNodeIds;
    CellNodeIds.Reserve(Cell->GetNodeDeltas().Num());
    for (const FMythicHarvestReplicatedNodeItem &Item :
         Cell->GetNodeDeltas()) {
        if (Item.State != EMythicHarvestNodeState::Available) {
            return false;
        }
        CellNodeIds.Add(Item.NodeId);
    }
    if (CellNodeIds.IsEmpty() || !Cell->Destroy()) {
        return false;
    }
    AuthorityCells.Remove(Coordinate);

    for (const FMythicHarvestNodeId &NodeId : CellNodeIds) {
        if (NodeId == CurrentNodeId) {
            continue;
        }
        const FRuntimeNode *Candidate = Nodes.Find(NodeId);
        if (Candidate
            && Candidate->State == EMythicHarvestNodeState::Available
            && !Candidate->Provider.IsValid()
            && Candidate->bHasReplicationCellCoordinate
            && Candidate->ReplicationCellCoordinate == Coordinate) {
            OutAdditionalImplicitNodes.AddUnique(NodeId);
        }
    }
    return true;
}

bool UMythicHarvestWorldSubsystem::RestoreAvailableNode(
    FRuntimeNode &Node,
    TArray<FMythicHarvestNodeId> &OutAdditionalImplicitNodes) {
    UWorld *World = GetWorld();
    UMythicHarvestRewardOutboxSubsystem *RewardOutbox = World
        ? World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>() : nullptr;
    uint32 NextGeneration = 0;
    if (!RewardOutbox || !RewardOutbox->TryResolveNextGeneration(
            WorldEpoch, Node.NodeId, NextGeneration)) {
        const UMythicHarvestSettings *Settings =
            GetDefault<UMythicHarvestSettings>();
        const double Recheck = Settings
            ? FMath::Max(0.01, static_cast<double>(
                Settings->RespawnVisibilityRecheckSeconds))
            : 1.0;
        Node.State = EMythicHarvestNodeState::Regrowing;
        Node.RespawnServerDeadline =
            (World ? World->GetTimeSeconds() : 0.0) + Recheck;
        Node.Revision = AdvanceNonZeroRevision(Node.Revision);
        if (!PublishNodeDelta(Node)) {
            UE_LOG(Myth, Fatal,
                   TEXT("Harvest node %s could not publish its fail-closed regrow retry."),
                   *Node.NodeId.GetGuid().ToString());
        }
        ScheduleRespawn(Node);
        UE_LOG(Myth, Error,
               TEXT("Harvest node %s could not reserve its next never-issued lifecycle generation; respawn remains fail-closed."),
               *Node.NodeId.GetGuid().ToString());
        return false;
    }

    const uint32 PreviousGeneration = Node.Generation;
    const uint32 PreviousRevision = Node.Revision;
    Node.Generation = NextGeneration;
    Node.Revision = AdvanceNonZeroRevision(Node.Revision);
    Node.State = EMythicHarvestNodeState::Available;
    Node.RemainingWork = Node.MaximumWork;
    Node.RespawnServerDeadline = 0.0;
    if (!PublishNodeDelta(Node)) {
        const UMythicHarvestSettings *Settings =
            GetDefault<UMythicHarvestSettings>();
        const double Recheck = Settings
            ? FMath::Max(0.01, static_cast<double>(
                Settings->RespawnVisibilityRecheckSeconds))
            : 1.0;
        Node.Generation = PreviousGeneration;
        Node.Revision = AdvanceNonZeroRevision(PreviousRevision);
        Node.State = EMythicHarvestNodeState::Regrowing;
        Node.RemainingWork = FMythicHarvestWork();
        Node.RespawnServerDeadline =
            (World ? World->GetTimeSeconds() : 0.0) + Recheck;
        if (!PublishNodeDelta(Node)) {
            UE_LOG(Myth, Fatal,
                   TEXT("Harvest node %s could neither publish explicit availability nor supersede it with a fail-closed regrow retry."),
                   *Node.NodeId.GetGuid().ToString());
        }
        ScheduleRespawn(Node);
        UE_LOG(Myth, Error,
               TEXT("Harvest node %s deferred reveal because its explicit versioned Available row could not be published."),
               *Node.NodeId.GetGuid().ToString());
        return false;
    }

    Node.ClaimOwner = FMythicHarvestClaimIdentity();
    Node.ClaimExpiryServerTime = 0.0;
    Node.Contributors.Reset();
    if (UMythicResourceISM *Provider = Node.Provider.Get()) {
        if (!Provider->ApplyNodeAvailability(Node.NodeId, true)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest provider '%s' failed to reveal respawned node %s; the provider was quarantined for retry."),
                   *Provider->GetPathName(),
                   *Node.NodeId.GetGuid().ToString());
        }
    }
    // An explicit Available row remains in any mixed-state cell. Once every row
    // is Available, actor-channel retirement is the transport drain fence; only
    // then may providerless identity rows collapse back to cooked defaults.
    const bool bCellRetired = RetireAuthorityCellIfOnlyAvailable(
        Node.ReplicationCellCoordinate, Node.NodeId,
        OutAdditionalImplicitNodes);
    return !Node.Provider.IsValid() && bCellRetired;
}

bool UMythicHarvestWorldSubsystem::IsRespawnVisibleToLivingPlayer(
    const FVector &NodeLocation) const {
    const UWorld *World = GetWorld();
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!World || !Settings
        || Settings->RespawnVisibilityRadiusCentimeters <= 0.0f) {
        return false;
    }
    const double RadiusSquared = FMath::Square(
        static_cast<double>(Settings->RespawnVisibilityRadiusCentimeters));
    for (FConstPlayerControllerIterator It =
             World->GetPlayerControllerIterator(); It; ++It) {
        const APlayerController *Controller = It->Get();
        const APawn *Pawn = Controller ? Controller->GetPawn() : nullptr;
        const UAbilitySystemComponent *ASC = Pawn
            ? UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn)
            : nullptr;
        if (Pawn && (!ASC || !ASC->HasMatchingGameplayTag(GAS_STATE_DEAD))
            && FVector::DistSquared(Pawn->GetActorLocation(), NodeLocation)
                <= RadiusSquared) {
            return true;
        }
    }
    return false;
}

void UMythicHarvestWorldSubsystem::TickRespawns(
    const double ServerNowSeconds) {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const double Recheck = Settings
        ? FMath::Max(0.01, static_cast<double>(
            Settings->RespawnVisibilityRecheckSeconds))
        : 1.0;
    TArray<FMythicHarvestNodeId> ImplicitNodesToPrune;
    while (!RespawnQueue.IsEmpty()
           && RespawnQueue.HeapTop().RespawnServerDeadline
               <= ServerNowSeconds) {
        FRespawnQueueEntry Entry;
        RespawnQueue.HeapPop(Entry, EAllowShrinking::No);
        FRuntimeNode *Node = Nodes.Find(Entry.NodeId);
        if (!Node || Node->State == EMythicHarvestNodeState::Available
            || Node->bMutationInProgress
            || Node->Generation != Entry.Generation
            || Node->Revision != Entry.Revision
            || Node->RespawnServerDeadline
                != Entry.RespawnServerDeadline) {
            continue;
        }
        const UMythicHarvestableDefinition *Definition =
            Node->Definition.Get();
        if (Definition && Definition->bDeferRegrowWhileVisible
            && IsRespawnVisibleToLivingPlayer(Node->OriginalWorldLocation)) {
            Node->State = EMythicHarvestNodeState::Regrowing;
            Node->RespawnServerDeadline = ServerNowSeconds + Recheck;
            Node->Revision = AdvanceNonZeroRevision(Node->Revision);
            if (!PublishNodeDelta(*Node)) {
                UE_LOG(Myth, Fatal,
                       TEXT("Harvest node %s could not publish its visibility-deferred regrow state."),
                       *Node->NodeId.GetGuid().ToString());
            }
            ScheduleRespawn(*Node);
            continue;
        }
        if (!Node->Provider.IsValid()) {
            UE_LOG(
                Myth, Display,
                TEXT("Harvest.UnresolvedExpiry NodeId=%s Generation=%u Revision=%u Policy=CollapseToImplicitAvailable"),
                *Node->NodeId.GetGuid().ToString(
                    EGuidFormats::DigitsWithHyphens),
                Node->Generation, Node->Revision);
        }
        const FMythicHarvestNodeId RestoredNodeId = Node->NodeId;
        if (RestoreAvailableNode(*Node, ImplicitNodesToPrune)) {
            ImplicitNodesToPrune.AddUnique(RestoredNodeId);
        }
    }
    for (const FMythicHarvestNodeId &NodeId : ImplicitNodesToPrune) {
        const FRuntimeNode *Node = Nodes.Find(NodeId);
        if (!Node || Node->State != EMythicHarvestNodeState::Available
            || Node->Provider.IsValid()
            || (Node->bHasReplicationCellCoordinate
                && AuthorityCells.Contains(
                    Node->ReplicationCellCoordinate))) {
            continue;
        }
        Nodes.Remove(NodeId);
    }
}

void UMythicHarvestWorldSubsystem::ScheduleRespawn(
    const FRuntimeNode &Node) {
    if (Node.State == EMythicHarvestNodeState::Available
        || !Node.NodeId.IsValid() || Node.Generation == 0
        || Node.Revision == 0
        || !FMath::IsFinite(Node.RespawnServerDeadline)
        || Node.RespawnServerDeadline < 0.0) {
        return;
    }
    FRespawnQueueEntry Entry;
    Entry.NodeId = Node.NodeId;
    Entry.RespawnServerDeadline = Node.RespawnServerDeadline;
    Entry.Generation = Node.Generation;
    Entry.Revision = Node.Revision;
    RespawnQueue.HeapPush(MoveTemp(Entry));
}

void UMythicHarvestWorldSubsystem::PruneAttackCycles(
    const double ServerNowSeconds) {
    for (auto It = AttackCycles.CreateIterator(); It; ++It) {
        const FAttackCycleState &Cycle = It.Value();
        if (!Cycle.Ability.IsValid()
            || FMythicHarvestCadencePolicy::IsExpired(
                ServerNowSeconds, Cycle.ExpiresServerTime)) {
            It.RemoveCurrent();
        }
    }
}

bool UMythicHarvestWorldSubsystem::HasActiveNodeMutation() const {
    for (const TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        if (Pair.Value.bMutationInProgress) {
            return true;
        }
    }
    return false;
}

void UMythicHarvestWorldSubsystem::TickRewardOutbox(
    const double ServerNowSeconds) {
    if (bHarvestTransactionInProgress
        || ServerNowSeconds < NextRewardOutboxRetryServerTime) {
        return;
    }
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const double RetryInterval = Settings
        ? FMath::Max(0.05, static_cast<double>(
            Settings->RewardOutboxRetryIntervalSeconds))
        : 2.0;
    NextRewardOutboxRetryServerTime = ServerNowSeconds + RetryInterval;

    UMythicHarvestRewardOutboxSubsystem *Outbox = GetWorld()
        ? GetWorld()->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>()
        : nullptr;
    if (!Outbox || !Outbox->HasPendingWork()) {
        return;
    }
    const int32 GrantBudget = Settings
        ? FMath::Clamp(Settings->RewardOutboxGrantBudget, 1, 256) : 8;
    TGuardValue<bool> TransactionGuard(bHarvestTransactionInProgress, true);
    Outbox->RetryPendingDeliveries(GrantBudget);
}

void UMythicHarvestWorldSubsystem::SendFeedback(
    AMythicPlayerController *Controller,
    const FMythicHarvestRequest &Request, const FRuntimeNode *Node,
    const FMythicHarvestResult &Result) {
    if (!IsValid(Controller) || Controller->IsActorBeingDestroyed()) {
        return;
    }
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    const double Now = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    const double MinimumInterval = Settings
        ? FMath::Max(0.0, static_cast<double>(
            Settings->FeedbackRateLimitSeconds))
        : 0.0;
    FMythicHarvestClientFeedback Feedback;
    Feedback.Outcome = Result.Outcome;
    Feedback.RejectReason = Result.RejectReason;
    Feedback.HarvestableDefinition = Node ? Node->Definition.Get() : nullptr;
    Feedback.RequiredToolType = Feedback.HarvestableDefinition
        ? Feedback.HarvestableDefinition->RequiredToolType : nullptr;
    Feedback.RequiredToolTier = Feedback.HarvestableDefinition
        ? Feedback.HarvestableDefinition->MinimumToolTier : 0;

    // Only an identical rejection tuple is coalesced. Accepted work and
    // completion feedback are never hidden behind an earlier rejection, and a
    // changed reason/tool/node always reaches the owning player immediately.
    if (Result.Outcome == EMythicHarvestOutcome::Rejected
        && MinimumInterval > 0.0) {
        FFeedbackThrottleKey Key;
        Key.Controller = Controller;
        Key.NodeId = Result.NodeId;
        Key.RequiredToolType = Feedback.RequiredToolType.Get();
        Key.Outcome = Result.Outcome;
        Key.RejectReason = Result.RejectReason;
        Key.RequiredToolTier = Feedback.RequiredToolTier;
        if (const double *LastTime = LastRejectedFeedbackTime.Find(Key);
            LastTime && Now - *LastTime < MinimumInterval) {
            return;
        }
        LastRejectedFeedbackTime.Add(MoveTemp(Key), Now);

        // The number of live rejection signatures is naturally bounded by
        // players and focused nodes, but streamed worlds can accumulate old
        // keys. Compact only after a generous threshold and never remove a
        // signature while it could still be throttled.
        if (LastRejectedFeedbackTime.Num() > 256) {
            for (auto It = LastRejectedFeedbackTime.CreateIterator(); It;
                 ++It) {
                if (!It.Key().Controller.IsValid()
                    || Now - It.Value() >= MinimumInterval) {
                    It.RemoveCurrent();
                }
            }
        }
    }

    const FVector Location =
        IsFiniteLocation(Request.AuthoritativeHit.ImpactPoint)
        ? Request.AuthoritativeHit.ImpactPoint
        : (Node ? Node->OriginalWorldLocation : FVector::ZeroVector);
    Feedback.Location = Location;
    Feedback.QuantizedRemainingWork = Node
        ? QuantizeRemainingWork(Node->RemainingWork, Node->MaximumWork) : 0;
    Feedback.Generation = Result.Generation;
    Feedback.Revision = Result.Revision;
    Feedback.ServerSequence = static_cast<int64>(
        FMath::Min<uint64>(Result.ServerSequence,
                           static_cast<uint64>(MAX_int64)));
    Controller->ClientReceiveHarvestFeedback(Feedback);
}

void UMythicHarvestWorldSubsystem::RegisterReplicationCell(
    AMythicHarvestReplicationCell &Cell) {
    if (IsAuthorityWorld(GetWorld()) || ClientCells.Contains(&Cell)) {
        return;
    }
    ClientCells.Add(&Cell);
    Cell.OnReplicationBatchReceived.AddUObject(
        this, &ThisClass::HandleClientCellBatch);
    Cell.OnCellEndingPlay.AddUObject(
        this, &ThisClass::UnregisterReplicationCell);
    HandleClientCellBatch(Cell);
}

void UMythicHarvestWorldSubsystem::UnregisterReplicationCell(
    AMythicHarvestReplicationCell &Cell) {
    if (!ClientCells.Remove(&Cell)) {
        return;
    }

    // Actor-channel close is the transport drain fence for this source. Drop
    // it only now; overlapping replacement cells keep the high-water alive.
    // Once the final source closes, cooked Available presentation is restored
    // before the bounded high-water entry is retired.
    const TWeakObjectPtr<AMythicHarvestReplicationCell> CellKey(&Cell);
    ClientDeferredPresentationCells.Remove(CellKey);
    TSet<FMythicHarvestNodeId> OwnedNodeIds;
    ClientNodeIdsByCell.RemoveAndCopyValue(CellKey, OwnedNodeIds);
    for (const FMythicHarvestNodeId &NodeId : OwnedNodeIds) {
        if (TMap<TWeakObjectPtr<AMythicHarvestReplicationCell>,
                 FMythicHarvestReplicatedNodeItem> *Sources =
                ClientNodeSourcesByNode.Find(NodeId)) {
            Sources->Remove(CellKey);
            if (Sources->IsEmpty()) {
                ClientNodeSourcesByNode.Remove(NodeId);
            }
        }
        const bool bHasRemainingSource =
            ClientNodeSourcesByNode.Contains(NodeId);
        ReconcileClientNodePresentation(NodeId);
        if (!bHasRemainingSource) {
            ClientNodeState.Remove(NodeId);
            ClientNodeHighWater.Remove(NodeId);
        }
    }
    Cell.OnReplicationBatchReceived.RemoveAll(this);
    Cell.OnCellEndingPlay.RemoveAll(this);
}

bool UMythicHarvestWorldSubsystem::ApplyClientNodeState(
    AMythicHarvestReplicationCell &Cell,
    const FMythicHarvestReplicatedNodeItem &Item) {
    UWorld *World = GetWorld();
    if (!World || World->GetNetMode() != NM_Client
        || !IsValidReplicatedHarvestNodeItem(Item)
        || Item.PresentationStream != ClientPresentationStream
        || !ClientCells.Contains(&Cell)) {
        return false;
    }

    FClientNodeHighWater *HighWater = ClientNodeHighWater.Find(Item.NodeId);
    if (HighWater) {
        if (HighWater->PresentationStream != ClientPresentationStream) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest client found a high-water row outside its active presentation stream."));
            return false;
        }
        int32 Version = 0;
        if (!FMythicHarvestReplicatedNodeItem::TryCompareVersion(
                Item, HighWater->Snapshot, Version)) {
            return false;
        }
        if (Version < 0) {
            return false;
        }
        if (Version == 0
            && !Item.HasSameReplicatedPayload(HighWater->Snapshot)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest client rejected equal-version conflicting node delta %s from cell '%s'."),
                   *Item.NodeId.GetGuid().ToString(), *Cell.GetPathName());
            return false;
        }
        if (Version > 0) {
            HighWater->Snapshot.CopyReplicatedPayloadFrom(Item);
        }
    }
    else {
        FClientNodeHighWater NewHighWater;
        NewHighWater.PresentationStream = ClientPresentationStream;
        NewHighWater.Snapshot.CopyReplicatedPayloadFrom(Item);
        ClientNodeHighWater.Add(Item.NodeId, MoveTemp(NewHighWater));
    }

    FMythicHarvestReplicatedNodeItem StoredItem;
    StoredItem.CopyReplicatedPayloadFrom(Item);
    const TWeakObjectPtr<AMythicHarvestReplicationCell> CellKey(&Cell);
    ClientNodeSourcesByNode.FindOrAdd(Item.NodeId)
        .Add(CellKey, MoveTemp(StoredItem));
    ClientNodeIdsByCell.FindOrAdd(CellKey).Add(Item.NodeId);
    ReconcileClientNodePresentation(Item.NodeId);
    return true;
}

void UMythicHarvestWorldSubsystem::ReconcileClientNodePresentation(
    const FMythicHarvestNodeId &NodeId) {
    const FClientNodeHighWater *HighWater =
        ClientNodeHighWater.Find(NodeId);
    const FMythicHarvestReplicatedNodeItem *Selected = nullptr;
    if (HighWater
        && HighWater->PresentationStream == ClientPresentationStream) {
        if (const TMap<TWeakObjectPtr<AMythicHarvestReplicationCell>,
                       FMythicHarvestReplicatedNodeItem> *Sources =
                ClientNodeSourcesByNode.Find(NodeId)) {
            for (const TPair<TWeakObjectPtr<AMythicHarvestReplicationCell>,
                             FMythicHarvestReplicatedNodeItem> &Pair :
                 *Sources) {
                if (ClientCells.Contains(Pair.Key)
                    && Pair.Value.PresentationStream
                        == ClientPresentationStream
                    && Pair.Value.HasSameReplicatedPayload(
                        HighWater->Snapshot)) {
                    Selected = &Pair.Value;
                    break;
                }
            }
        }
    }

    if (Selected) {
        FMythicHarvestReplicatedNodeItem Effective;
        Effective.CopyReplicatedPayloadFrom(*Selected);
        ClientNodeState.Add(NodeId, MoveTemp(Effective));
    }
    else {
        ClientNodeState.Remove(NodeId);
    }
    if (const TWeakObjectPtr<UMythicResourceISM> *WeakProvider =
            ClientPresentationProviderByNode.Find(NodeId)) {
        if (UMythicResourceISM *Provider = WeakProvider->Get()) {
            if (!Provider->ApplyNodeAvailability(
                    NodeId,
                    !Selected
                        || Selected->State
                            == EMythicHarvestNodeState::Available)) {
                UE_LOG(Myth, Error,
                       TEXT("Client harvest provider '%s' failed replicated presentation reconciliation for node %s; the provider was quarantined for retry."),
                       *Provider->GetPathName(),
                       *NodeId.GetGuid().ToString());
            }
        }
    }
}

void UMythicHarvestWorldSubsystem::HandleClientCellBatch(
    AMythicHarvestReplicationCell &Cell) {
    ReconcileClientCellBatch(Cell);
}

void UMythicHarvestWorldSubsystem::ReconcileClientCellBatch(
    AMythicHarvestReplicationCell &Cell) {
    if (!ClientCells.Contains(&Cell)) {
        return;
    }
    const TWeakObjectPtr<AMythicHarvestReplicationCell> CellKey(&Cell);
    const TArray<FMythicHarvestReplicatedNodeItem> &Deltas =
        Cell.GetNodeDeltas();
    const FMythicHarvestPresentationStreamToken &BatchStream =
        Cell.GetPresentationStream();
    if (!BatchStream.IsValid()) {
        // A runtime cell can BeginPlay before its initial replicated properties arrive. Retain all existing sources
        // until the replicated cell-level barrier invokes this method again; an empty/incomplete bunch is not a
        // semantic row removal.
        return;
    }

    switch (ClassifyClientPresentationStream(BatchStream)) {
        case EClientPresentationStreamDisposition::Future:
            // The cell-level token remains available even when restore cleared every Fast Array row. The
            // always-relevant GameState token is the only activation source and replays this final snapshot after
            // clearing the old stream atomically.
            ClientDeferredPresentationCells.Add(CellKey);
            return;
        case EClientPresentationStreamDisposition::Rejected:
            ClientDeferredPresentationCells.Remove(CellKey);
            return;
        case EClientPresentationStreamDisposition::Current:
            ClientDeferredPresentationCells.Remove(CellKey);
            break;
    }

    TSet<FMythicHarvestNodeId> PresentIds;
    PresentIds.Reserve(Deltas.Num());
    TOptional<FMythicHarvestPresentationStreamToken> RowStream;
    for (const FMythicHarvestReplicatedNodeItem &Item : Deltas) {
        if (!IsValidReplicatedHarvestNodeItem(Item)
            || PresentIds.Contains(Item.NodeId)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest client rejected malformed or duplicate node rows from cell '%s'."),
                   *Cell.GetPathName());
            return;
        }
        if (RowStream.IsSet()
            && RowStream.GetValue() != Item.PresentationStream) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest client rejected mixed row streams from cell '%s'."),
                   *Cell.GetPathName());
            return;
        }
        RowStream = Item.PresentationStream;
        PresentIds.Add(Item.NodeId);
    }
    if (RowStream.IsSet() && RowStream.GetValue() != BatchStream) {
        if (ClassifyClientPresentationStream(RowStream.GetValue())
            == EClientPresentationStreamDisposition::Future) {
            // Fast Array data can arrive before the actor's cell-level token. Treat a uniform valid future row set as
            // deferred transport state; the RepNotify replays it after the matching scalar arrives.
            ClientDeferredPresentationCells.Add(CellKey);
            return;
        }
        UE_LOG(Myth, Error,
               TEXT("Harvest client rejected a row/cell presentation-stream conflict from cell '%s'."),
               *Cell.GetPathName());
        return;
    }

    TSet<FMythicHarvestNodeId> PreviousIds =
        ClientNodeIdsByCell.FindRef(CellKey);
    for (const FMythicHarvestReplicatedNodeItem &Item : Deltas) {
        ApplyClientNodeState(Cell, Item);
    }
    bool bHasMissingRows = false;
    for (const FMythicHarvestNodeId &PreviousId : PreviousIds) {
        if (!PresentIds.Contains(PreviousId)) {
            bHasMissingRows = true;
            break;
        }
    }
    if (bHasMissingRows) {
        // Fast Array absence is transport state, never gameplay semantics. A
        // respawn arrives as a newer explicit Available row; a stream rotation
        // is fenced by the coordinator token; source retirement waits for this
        // actor channel to close. Retaining the last accepted source here also
        // covers arbitrary property/FastArray callback reordering.
        UE_LOG(Myth, VeryVerbose,
               TEXT("Harvest cell '%s' omitted previously accepted rows; retaining them until explicit versioned replacement or channel retirement."),
               *Cell.GetPathName());
    }
}

void UMythicHarvestWorldSubsystem::GetNodeCounts(
    int32 &OutRegisteredProviders, int32 &OutAvailable,
    int32 &OutUnavailable) const {
    OutRegisteredProviders = NodesByProvider.Num();
    OutAvailable = 0;
    OutUnavailable = 0;
    for (const TPair<FMythicHarvestNodeId, FRuntimeNode> &Pair : Nodes) {
        if (Pair.Value.State == EMythicHarvestNodeState::Available) {
            ++OutAvailable;
        }
        else {
            ++OutUnavailable;
        }
    }
}
