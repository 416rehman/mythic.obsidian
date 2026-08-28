#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR

#include "CoreGlobals.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Resources/MythicResourceISM.h"
#include "MythicHarvestReplicationTestTypes.h"
#include "TimerManager.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"
#include "World/Harvesting/MythicHarvestReplicationCell.h"
#include "World/Harvesting/MythicHarvestRewardOutboxSubsystem.h"
#include "World/Harvesting/MythicHarvestWorldSubsystem.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"

namespace {

class FScopedHarvestPIEWorld final {
public:
    explicit FScopedHarvestPIEWorld(const ENetMode NetMode) {
        InitializationValues = UWorld::InitializationValues()
            .CreatePhysicsScene(false)
            .ShouldSimulatePhysics(false)
            .EnableTraceCollision(false)
            .CreateNavigation(false)
            .CreateAISystem(false);
        World = UWorld::CreateWorld(
            EWorldType::PIE, false,
            MakeUniqueObjectName(nullptr, UWorld::StaticClass(),
                                 TEXT("HarvestRegistryTest")),
            nullptr, true, ERHIFeatureLevel::Num, &InitializationValues,
            true);
        if (World) {
            World->SetPlayInEditorInitialNetMode(NetMode);
            World->InitWorld(InitializationValues);
        }
    }

    ~FScopedHarvestPIEWorld() {
        if (World) {
            World->DestroyWorld(false);
        }
    }

    UWorld *Get() const { return World; }

private:
    UWorld::InitializationValues InitializationValues;
    UWorld *World = nullptr;
};

// A whole automation test body runs inside one engine frame, where FTimerManager delivers nothing: a next-tick timer
// is queued with ExpireTime == InternalTime and expiry is strictly greater (TimerManager.cpp:1212), so a zero-delta
// Tick fires it only once time has advanced, and every later Tick in the same frame is dropped by
// HasBeenTickedThisFrame (:1136). Advancing the frame counter and passing positive deltas restores both.
void AdvanceHarvestWorldTimers(UWorld &World, const float Seconds) {
    constexpr int32 FrameCount = 3;
    const float FrameSeconds =
        FMath::Max(Seconds, 1.0f / 60.0f) / static_cast<float>(FrameCount);
    for (int32 Frame = 0; Frame < FrameCount; ++Frame) {
        ++GFrameCounter;
        World.GetTimerManager().Tick(FrameSeconds);
    }
}

UMythicResourceISM *CreatePackedProvider(
    AActor &Owner, UMythicHarvestableDefinition &Definition,
    const FName Name, const TArray<float> &PackedNodeId) {
    UMythicResourceISM *Provider =
        NewObject<UMythicResourceISM>(&Owner, Name);
    Owner.AddInstanceComponent(Provider);
    Provider->HarvestableDefinition = &Definition;
    Provider->IdentitySource = EMythicHarvestIdentitySource::PCGPacked;
    Provider->IdentityCustomDataStartIndex = 0;
    Provider->SetNumCustomDataFloats(
        MythicHarvestPCGIdentity::PackedFloatCount);
    Provider->AddInstance(FTransform::Identity);
    Provider->SetCustomData(0, MakeArrayView(PackedNodeId));
    return Provider;
}

} // namespace

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestReplicationCellHorizontalRelevancyTest,
    "Mythic.Harvesting.Replication.HorizontalCellRelevancy",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestReplicationCellHorizontalRelevancyTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedHarvestPIEWorld ScopedWorld(NM_DedicatedServer);
    UWorld *World = ScopedWorld.Get();
    TestNotNull(TEXT("dedicated authority fixture exists"), World);
    if (!World) {
        return false;
    }

    AMythicHarvestReplicationCell *Cell =
        World->SpawnActor<AMythicHarvestReplicationCell>();
    TestNotNull(TEXT("spatial replication cell spawns"), Cell);
    if (!Cell) {
        return false;
    }
    const FMythicHarvestPresentationStreamToken Stream(
        FGuid(0x11000001, 0x22000002, 0x33000003, 0x44000004), 1);
    TestTrue(TEXT("authority configures a horizontal replication bucket"),
             Cell->ConfigureSpatialCell(
                 FIntPoint::ZeroValue,
                 FVector(0.0, 0.0, 500000.0), 1000.0, Stream));
    TestTrue(TEXT("a far-vertical viewer at the XY cell corner remains relevant"),
             Cell->IsNetRelevantFor(
                 nullptr, nullptr,
                 FVector(700.0, 700.0, -500000.0)));
    TestFalse(TEXT("a viewer outside the horizontal cull radius is irrelevant"),
              Cell->IsNetRelevantFor(
                  nullptr, nullptr,
                  FVector(800.0, 800.0, 500000.0)));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestProviderTransientRecoveryTest,
    "Mythic.Harvesting.Provider.TransientRegistrationRecovery",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestProviderTransientRecoveryTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedHarvestPIEWorld ScopedWorld(NM_ListenServer);
    UWorld *World = ScopedWorld.Get();
    TestNotNull(TEXT("authority recovery fixture initializes"), World);
    if (!World) {
        return false;
    }

    UMythicHarvestWorldSubsystem *Subsystem =
        World->GetSubsystem<UMythicHarvestWorldSubsystem>();
    TestNotNull(TEXT("authority recovery subsystem initializes"), Subsystem);
    AActor *Owner = World->SpawnActor<AActor>();
    TestNotNull(TEXT("authority recovery owner spawns"), Owner);
    if (!Subsystem || !Owner) {
        return false;
    }

    UMythicHarvestableDefinition *Definition =
        NewObject<UMythicHarvestableDefinition>(Owner);
    Definition->MaxWork = 10.0f;
    const auto PackNode = [this](const FGuid &Guid) {
        TArray<float> Packed;
        TestTrue(TEXT("recovery fixture identity packs"),
                 MythicHarvestPCGIdentity::AppendPackedNodeId(
                     FMythicHarvestNodeId(Guid), Packed));
        return Packed;
    };

    UMythicResourceISM *PrimitiveMapProvider = CreatePackedProvider(
        *Owner, *Definition, TEXT("PrimitiveMapRecoveryProvider"),
        PackNode(FGuid(0x71000001, 0x71000002, 0x71000003, 0x71000004)));
    PrimitiveMapProvider->SetIdentityRefreshRuntimeFailureInjectionForTests(
        1, 0);
    PrimitiveMapProvider->RegisterComponent();
    AdvanceHarvestWorldTimers(*World, 0.0f);
    TestTrue(TEXT("transient primitive-map failure suppresses collision"),
             PrimitiveMapProvider
                 ->IsHarvestQueryCollisionSuppressedForTests());
    TestEqual(TEXT("transient primitive-map failure stays unregistered"),
              Subsystem->NodesByProvider.Num(), 0);
    AdvanceHarvestWorldTimers(*World, 0.11f);
    TestFalse(TEXT("primitive-map recovery restores authored collision"),
              PrimitiveMapProvider
                  ->IsHarvestQueryCollisionSuppressedForTests());
    TestTrue(TEXT("primitive-map recovery restores query capability"),
             CollisionEnabledHasQuery(
                 PrimitiveMapProvider->GetCollisionEnabled()));
    TestEqual(TEXT("primitive-map retry registers exactly one provider"),
              Subsystem->NodesByProvider.Num(), 1);

    const int32 RegistrationWrites =
        PrimitiveMapProvider->GetAvailabilityTransformWriteCountForTests();
    const FMythicHarvestNodeId PrimitiveMapNodeId(
        FGuid(0x71000001, 0x71000002, 0x71000003, 0x71000004));
    TestTrue(TEXT("an already-available presentation is a verified no-op"),
             PrimitiveMapProvider->ApplyNodeAvailability(
                 PrimitiveMapNodeId, true));
    TestEqual(TEXT("availability no-op performs no native transform write"),
              PrimitiveMapProvider
                  ->GetAvailabilityTransformWriteCountForTests(),
              RegistrationWrites);
    TestTrue(TEXT("a state change writes one instance transform"),
             PrimitiveMapProvider->ApplyNodeAvailability(
                 PrimitiveMapNodeId, false));
    const int32 HiddenWrites =
        PrimitiveMapProvider->GetAvailabilityTransformWriteCountForTests();
    TestEqual(TEXT("the first hide submits exactly one transform"),
              HiddenWrites, RegistrationWrites + 1);
    TestTrue(TEXT("reapplying the same hidden state is a no-op"),
             PrimitiveMapProvider->ApplyNodeAvailability(
                 PrimitiveMapNodeId, false));
    TestEqual(TEXT("a repeated hide performs no native transform write"),
              PrimitiveMapProvider
                  ->GetAvailabilityTransformWriteCountForTests(),
              HiddenWrites);

    UMythicResourceISM *TransformProvider = CreatePackedProvider(
        *Owner, *Definition, TEXT("TransformReadRecoveryProvider"),
        PackNode(FGuid(0x72000001, 0x72000002, 0x72000003, 0x72000004)));
    TransformProvider->SetIdentityRefreshRuntimeFailureInjectionForTests(0, 1);
    TransformProvider->RegisterComponent();
    AdvanceHarvestWorldTimers(*World, 0.0f);
    TestTrue(TEXT("transient transform-read failure suppresses collision"),
             TransformProvider->IsHarvestQueryCollisionSuppressedForTests());
    TestEqual(TEXT("transform-read failure does not publish a partial provider"),
              Subsystem->NodesByProvider.Num(), 1);
    AdvanceHarvestWorldTimers(*World, 0.11f);
    TestFalse(TEXT("transform-read recovery restores authored collision"),
              TransformProvider->IsHarvestQueryCollisionSuppressedForTests());
    TestEqual(TEXT("transform-read retry registers the second provider"),
              Subsystem->NodesByProvider.Num(), 2);

    const FMythicHarvestNodeId BatchNodeA(
        FGuid(0x73000001, 0x73000002, 0x73000003, 0x73000004));
    const FMythicHarvestNodeId BatchNodeB(
        FGuid(0x74000001, 0x74000002, 0x74000003, 0x74000004));
    UMythicResourceISM *BatchProvider = NewObject<UMythicResourceISM>(
        Owner, TEXT("ContiguousBatchProvider"));
    Owner->AddInstanceComponent(BatchProvider);
    BatchProvider->HarvestableDefinition = Definition;
    BatchProvider->IdentitySource = EMythicHarvestIdentitySource::PCGPacked;
    BatchProvider->SetNumCustomDataFloats(
        MythicHarvestPCGIdentity::PackedFloatCount);
    BatchProvider->AddInstance(FTransform::Identity);
    BatchProvider->AddInstance(FTransform(FVector(100.0, 0.0, 0.0)));
    const TArray<float> PackedBatchNodeA = PackNode(BatchNodeA.GetGuid());
    const TArray<float> PackedBatchNodeB = PackNode(BatchNodeB.GetGuid());
    BatchProvider->SetCustomData(0, MakeArrayView(PackedBatchNodeA));
    BatchProvider->SetCustomData(1, MakeArrayView(PackedBatchNodeB));
    BatchProvider->RegisterComponent();
    TestTrue(TEXT("two-node provider registers through one prevalidated batch"),
             BatchProvider->RefreshHarvestIdentityRegistration());
    const int32 BatchWritesBefore =
        BatchProvider->GetAvailabilityTransformWriteCountForTests();
    const int32 BatchCallsBefore =
        BatchProvider->GetAvailabilityNativeBatchCallCountForTests();
    const TArray<FMythicHarvestNodePresentationUpdate> HideBatch = {
        {BatchNodeA, false}, {BatchNodeB, false}};
    TestTrue(TEXT("contiguous node updates apply as one provider batch"),
             BatchProvider->ApplyNodeAvailabilityBatch(HideBatch));
    TestEqual(TEXT("one contiguous batch submits two transform writes"),
              BatchProvider->GetAvailabilityTransformWriteCountForTests(),
              BatchWritesBefore + 2);
    TestEqual(TEXT("one contiguous batch makes one native submission"),
              BatchProvider->GetAvailabilityNativeBatchCallCountForTests(),
              BatchCallsBefore + 1);
    TestTrue(TEXT("repeating a complete provider batch is a no-op"),
             BatchProvider->ApplyNodeAvailabilityBatch(HideBatch));
    TestEqual(TEXT("repeated batch performs no additional transform writes"),
              BatchProvider->GetAvailabilityTransformWriteCountForTests(),
              BatchWritesBefore + 2);
    TestEqual(TEXT("repeated batch performs no native submission"),
              BatchProvider->GetAvailabilityNativeBatchCallCountForTests(),
              BatchCallsBefore + 1);

    TArray<float> InvalidPackedNodeId;
    InvalidPackedNodeId.SetNumZeroed(
        MythicHarvestPCGIdentity::PackedFloatCount);
    UMythicResourceISM *MalformedProvider = CreatePackedProvider(
        *Owner, *Definition, TEXT("TerminalMalformedProvider"),
        InvalidPackedNodeId);
    MalformedProvider->RegisterComponent();
    AddExpectedError(
        TEXT("invalid or duplicate identity at current index"),
        EAutomationExpectedErrorFlags::Contains, 1);
    AdvanceHarvestWorldTimers(*World, 0.0f);
    TestTrue(TEXT("terminal authored corruption remains quarantined"),
             MalformedProvider->IsHarvestQueryCollisionSuppressedForTests());
    TestEqual(TEXT("terminal corruption publishes no provider lifetime"),
              Subsystem->NodesByProvider.Num(), 3);
    AdvanceHarvestWorldTimers(*World, 1.0f);
    TestTrue(TEXT("terminal authored corruption never reopens collision"),
             MalformedProvider->IsHarvestQueryCollisionSuppressedForTests());
    TestEqual(TEXT("terminal authored corruption is not retried into authority"),
              Subsystem->NodesByProvider.Num(), 3);

    World->DestroyActor(Owner);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestExplicitAvailableReplicationTest,
    "Mythic.Harvesting.Replication.ExplicitAvailableAndCellRetirement",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestExplicitAvailableReplicationTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedHarvestPIEWorld ScopedWorld(NM_ListenServer);
    UWorld *World = ScopedWorld.Get();
    TestNotNull(TEXT("explicit-availability authority world initializes"),
                World);
    if (!World) {
        return false;
    }

    AMythicHarvestReplicationTestGameState *GameState =
        World->SpawnActor<AMythicHarvestReplicationTestGameState>();
    TestNotNull(TEXT("explicit-availability coordinator spawns"), GameState);
    if (!GameState) {
        return false;
    }
    World->SetGameState(GameState);

    UMythicHarvestWorldSubsystem *Subsystem =
        World->GetSubsystem<UMythicHarvestWorldSubsystem>();
    UMythicHarvestRewardOutboxSubsystem *Outbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    TestNotNull(TEXT("explicit-availability harvest subsystem exists"),
                Subsystem);
    TestNotNull(TEXT("explicit-availability outbox exists"), Outbox);
    if (!Subsystem || !Outbox) {
        return false;
    }
    Subsystem->RegisterPresentationCoordinator(*GameState);

    const FMythicHarvestNodeId NodeA(
        FGuid(0x81000001, 0x81000002, 0x81000003, 0x81000004));
    const FMythicHarvestNodeId NodeB(
        FGuid(0x82000001, 0x82000002, 0x82000003, 0x82000004));
    const auto BuildOutboxSnapshot =
        [Subsystem](const uint64 Sequence,
                    const TArray<TPair<FMythicHarvestNodeId, uint32>>
                        &CompletedGenerations) {
            FMythicHarvestRewardOutboxSaveV1 Snapshot;
            Snapshot.WorldEpoch = Subsystem->GetWorldEpoch();
            Snapshot.SnapshotSequence = Sequence;
            for (const TPair<FMythicHarvestNodeId, uint32> &Pair :
                 CompletedGenerations) {
                FMythicSavedHarvestRewardCompletionV1 &Completion =
                    Snapshot.KnownCompletions.AddDefaulted_GetRef();
                Completion.WorldEpoch = Snapshot.WorldEpoch;
                Completion.NodeGuid = Pair.Key.GetGuid();
                Completion.Generation = Pair.Value;
                FMythicSavedHarvestGenerationHighWaterV1 &HighWater =
                    Snapshot.GenerationHighWatermarks.AddDefaulted_GetRef();
                HighWater.WorldEpoch = Snapshot.WorldEpoch;
                HighWater.NodeGuid = Pair.Key.GetGuid();
                HighWater.HighestKnownGeneration = Pair.Value;
            }
            return Snapshot;
        };

    FName Diagnostic;
    TestTrue(TEXT("node A generation-one completion installs"),
             Outbox->RestoreSaveSnapshot(
                 BuildOutboxSnapshot(1, {{NodeA, 1}}), Diagnostic));

    AActor *Owner = World->SpawnActor<AActor>();
    UMythicHarvestableDefinition *Definition =
        Owner ? NewObject<UMythicHarvestableDefinition>(Owner) : nullptr;
    TestNotNull(TEXT("explicit-availability provider owner spawns"), Owner);
    TestNotNull(TEXT("explicit-availability definition allocates"),
                Definition);
    if (!Owner || !Definition) {
        return false;
    }
    Definition->MaxWork = 10.0f;

    TArray<float> PackedA;
    TArray<float> PackedB;
    TestTrue(TEXT("node A identity packs"),
             MythicHarvestPCGIdentity::AppendPackedNodeId(NodeA, PackedA));
    TestTrue(TEXT("node B identity packs"),
             MythicHarvestPCGIdentity::AppendPackedNodeId(NodeB, PackedB));
    UMythicResourceISM *ProviderA = CreatePackedProvider(
        *Owner, *Definition, TEXT("ExplicitAvailableProviderA"), PackedA);
    UMythicResourceISM *ProviderB = CreatePackedProvider(
        *Owner, *Definition, TEXT("ExplicitAvailableProviderB"), PackedB);
    ProviderA->RegisterComponent();
    ProviderB->RegisterComponent();
    TestTrue(TEXT("provider A registers"),
             ProviderA->RefreshHarvestIdentityRegistration());
    TestTrue(TEXT("provider B registers"),
             ProviderB->RefreshHarvestIdentityRegistration());

    UMythicHarvestWorldSubsystem::FRuntimeNode *RuntimeA =
        Subsystem->Nodes.Find(NodeA);
    UMythicHarvestWorldSubsystem::FRuntimeNode *RuntimeB =
        Subsystem->Nodes.Find(NodeB);
    TestNotNull(TEXT("node A runtime row exists"), RuntimeA);
    TestNotNull(TEXT("node B runtime row exists"), RuntimeB);
    if (!RuntimeA || !RuntimeB) {
        return false;
    }
    TestEqual(TEXT("node A begins after installed completion high-water"),
              RuntimeA->Generation, 2u);
    TestEqual(TEXT("node B begins at its first generation"),
              RuntimeB->Generation, 1u);

    RuntimeA->State = EMythicHarvestNodeState::Regrowing;
    RuntimeA->RemainingWork = FMythicHarvestWork();
    RuntimeA->Revision = 2;
    RuntimeA->RespawnServerDeadline = 100.0;
    RuntimeB->State = EMythicHarvestNodeState::Regrowing;
    RuntimeB->RemainingWork = FMythicHarvestWork();
    RuntimeB->Revision = 2;
    RuntimeB->RespawnServerDeadline = 100.0;
    TestTrue(TEXT("node A regrow row publishes"),
             Subsystem->PublishNodeDelta(*RuntimeA));
    TestTrue(TEXT("node B regrow row publishes in the same cell"),
             Subsystem->PublishNodeDelta(*RuntimeB));
    TestEqual(TEXT("both nearby rows share one replication cell"),
              Subsystem->AuthorityCells.Num(), 1);

    TestTrue(TEXT("node A generation-two completion installs"),
             Outbox->RestoreSaveSnapshot(
                 BuildOutboxSnapshot(2, {{NodeA, 2}}), Diagnostic));
    TArray<FMythicHarvestNodeId> AdditionalImplicitNodes;
    TestFalse(TEXT("a provider-backed Available row remains resident"),
              Subsystem->RestoreAvailableNode(
                  *RuntimeA, AdditionalImplicitNodes));
    TestEqual(TEXT("respawn advances node A to a never-issued generation"),
              RuntimeA->Generation, 3u);
    const AMythicHarvestReplicationCell *Cell =
        Subsystem->AuthorityCells.CreateConstIterator().Value().Get();
    const FMythicHarvestReplicatedNodeItem *AvailableRow =
        Cell ? Cell->FindNodeDelta(NodeA) : nullptr;
    TestNotNull(TEXT("mixed-state cell retains node A's explicit row"),
                AvailableRow);
    if (AvailableRow) {
        TestEqual(TEXT("retained row has explicit Available semantics"),
                  AvailableRow->State,
                  EMythicHarvestNodeState::Available);
        TestEqual(TEXT("retained row carries the advanced version"),
                  AvailableRow->Generation, 3u);
    }

    ProviderA->UnregisterComponent();
    RuntimeA = Subsystem->Nodes.Find(NodeA);
    TestNotNull(TEXT("providerless explicit row retains runtime version"),
                RuntimeA);
    if (RuntimeA) {
        TestFalse(TEXT("providerless explicit row detaches its provider"),
                  RuntimeA->Provider.IsValid());
    }

    TestTrue(TEXT("both completion high-waters install before final respawn"),
             Outbox->RestoreSaveSnapshot(
                 BuildOutboxSnapshot(3, {{NodeA, 2}, {NodeB, 1}}),
                 Diagnostic));
    RuntimeB = Subsystem->Nodes.Find(NodeB);
    if (!RuntimeB) {
        return false;
    }
    RuntimeB->Revision = 3;
    RuntimeB->RespawnServerDeadline = World->GetTimeSeconds();
    TestTrue(TEXT("node B deadline update publishes"),
             Subsystem->PublishNodeDelta(*RuntimeB));
    Subsystem->ScheduleRespawn(*RuntimeB);
    Subsystem->TickRespawns(World->GetTimeSeconds());
    TestTrue(TEXT("all-Available cell retires through actor-channel close"),
             Subsystem->AuthorityCells.IsEmpty());
    TestFalse(TEXT("cell retirement prunes providerless explicit runtime rows"),
              Subsystem->Nodes.Contains(NodeA));
    RuntimeB = Subsystem->Nodes.Find(NodeB);
    TestNotNull(TEXT("provider-backed node B remains resident"), RuntimeB);
    if (RuntimeB) {
        TestEqual(TEXT("node B publishes explicit availability before retirement"),
                  RuntimeB->State, EMythicHarvestNodeState::Available);
    }

    World->DestroyActor(Owner);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestClientPresentationRegistryTest,
    "Mythic.Harvesting.Replication.ClientPresentationNeverAuthorsNodes",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestClientPresentationRegistryTest::RunTest(
    const FString & /*Parameters*/) {
    const FMythicHarvestNodeId NodeId(
        FGuid(0x13572468, 0x24681357, 0xaabbccdd, 0x10203040));
    TArray<float> PackedNodeId;
    TestTrue(TEXT("test node identity packs"),
             MythicHarvestPCGIdentity::AppendPackedNodeId(NodeId,
                                                          PackedNodeId));

    {
        FScopedHarvestPIEWorld ScopedClientWorld(NM_Client);
        UWorld *World = ScopedClientWorld.Get();
        TestNotNull(TEXT("client PIE world initializes"), World);
        if (!World) {
            return false;
        }
        TestEqual(TEXT("test world has client net mode"), World->GetNetMode(),
                  NM_Client);

        UMythicHarvestWorldSubsystem *Subsystem =
            World->GetSubsystem<UMythicHarvestWorldSubsystem>();
        TestNotNull(TEXT("harvest subsystem exists on presentation client"),
                    Subsystem);
        if (!Subsystem) {
            return false;
        }
        TestFalse(TEXT("client does not author an authority world epoch"),
                  Subsystem->GetWorldEpoch().IsValid());

        // A reconnect may first observe any current live serial. Only the always-relevant coordinator activates it;
        // spatial rows arriving before this call remain presentation-inert.
        const FMythicHarvestPresentationStreamToken StreamA(
            FGuid(0x10000001, 0x10000002, 0x10000003, 0x10000004),
            41);
        TestTrue(TEXT("a fresh client accepts the current coordinator stream"),
                 Subsystem->ActivateClientPresentationStream(StreamA));

        AActor *Owner = World->SpawnActor<AActor>();
        TestNotNull(TEXT("client provider owner spawns"), Owner);
        if (!Owner) {
            return false;
        }

        UMythicHarvestableDefinition *Definition =
            NewObject<UMythicHarvestableDefinition>(Owner);
        Definition->MaxWork = 10.0f;
        UMythicResourceISM *Provider = CreatePackedProvider(
            *Owner, *Definition, TEXT("ClientProvider"), PackedNodeId);
        Provider->RegisterComponent();

        TestTrue(TEXT("client provider builds presentation identity index"),
                 Provider->RefreshHarvestIdentityRegistration());
        TestFalse(TEXT("authority registration API rejects NM_Client"),
                  Subsystem->RefreshResourceProvider(*Provider));
        TestEqual(TEXT("client creates no authoritative node rows"),
                  Subsystem->Nodes.Num(), 0);
        TestEqual(TEXT("client creates no authoritative provider rows"),
                  Subsystem->NodesByProvider.Num(), 0);
        TestEqual(TEXT("client owns one presentation provider row"),
                  Subsystem->ClientPresentationNodesByProvider.Num(), 1);
        TestEqual(TEXT("client indexes one presentation node"),
                  Subsystem->ClientPresentationProviderByNode.Num(), 1);

        UMythicResourceISM *DuplicateProvider = CreatePackedProvider(
            *Owner, *Definition, TEXT("DuplicateClientProvider"),
            PackedNodeId);
        DuplicateProvider->RegisterComponent();
        AddExpectedError(TEXT("Client harvest presentation provider"),
                         EAutomationExpectedErrorFlags::Contains, 1);
        TestFalse(
            TEXT("duplicate client presentation identity fails atomically"),
            DuplicateProvider->RefreshHarvestIdentityRegistration());
        TestEqual(TEXT("duplicate rejection preserves original provider row"),
                  Subsystem->ClientPresentationNodesByProvider.Num(), 1);
        TestEqual(TEXT("duplicate rejection preserves original node row"),
                  Subsystem->ClientPresentationProviderByNode.Num(), 1);
        DuplicateProvider->UnregisterComponent();

        AMythicHarvestReplicationCell *CellA =
            World->SpawnActor<AMythicHarvestReplicationCell>();
        AMythicHarvestReplicationCell *CellB =
            World->SpawnActor<AMythicHarvestReplicationCell>();
        TestNotNull(TEXT("first replacement cell spawns"), CellA);
        TestNotNull(TEXT("second replacement cell spawns"), CellB);
        if (!CellA || !CellB) {
            return false;
        }
        Subsystem->RegisterReplicationCell(*CellA);
        Subsystem->RegisterReplicationCell(*CellB);

        const auto SetCellRows =
            [Subsystem](AMythicHarvestReplicationCell &Cell,
                        const TArray<FMythicHarvestReplicatedNodeItem> &Rows) {
                if (!Rows.IsEmpty()) {
                    Cell.ReplicatedPresentationStream =
                        Rows[0].PresentationStream;
                }
                Cell.ReplicatedNodes.Items = Rows;
                Subsystem->HandleClientCellBatch(Cell);
            };

        FMythicHarvestReplicatedNodeItem Depleted;
        Depleted.PresentationStream = StreamA;
        Depleted.NodeId = NodeId;
        Depleted.Generation = 1;
        Depleted.Revision = 1;
        Depleted.State = EMythicHarvestNodeState::Depleted;
        SetCellRows(*CellA, {Depleted});
        SetCellRows(*CellB, {Depleted});

        FTransform OriginalTransform = FTransform::Identity;
        FTransform HiddenTransform;
        TestTrue(TEXT("replicated state resolves the streamed provider"),
                 Provider->GetInstanceTransform(0, HiddenTransform, true));
        TestTrue(TEXT("replicated depleted state hides only presentation"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        TestEqual(TEXT("replicated presentation authors no runtime nodes"),
                  Subsystem->Nodes.Num(), 0);

        FMythicHarvestReplicatedNodeItem ConflictingEqualVersion = Depleted;
        ConflictingEqualVersion.State = EMythicHarvestNodeState::Available;
        AddExpectedError(TEXT("equal-version conflicting node delta"),
                         EAutomationExpectedErrorFlags::Contains, 1);
        SetCellRows(*CellB, {ConflictingEqualVersion});
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("equal-version conflict cannot restore presentation"),
                 HiddenTransform.GetLocation().Z < -999000.0);

        Subsystem->UnregisterReplicationCell(*CellA);
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("old cell teardown preserves replacement snapshot"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        TestEqual(TEXT("replacement remains the sole active source"),
                  Subsystem->ClientNodeSourcesByNode.FindChecked(NodeId).Num(),
                  1);

        SetCellRows(*CellB, {});
        FTransform RestoredTransform;
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("Fast Array absence cannot semantically reveal a node"),
                 RestoredTransform.GetLocation().Z < -999000.0);
        AdvanceHarvestWorldTimers(*World, 5.0f);
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("arbitrary callback delay cannot turn absence into availability"),
                 RestoredTransform.GetLocation().Z < -999000.0);
        TestTrue(TEXT("missing transport rows retain their accepted source"),
                 Subsystem->ClientNodeSourcesByNode.Contains(NodeId)
                     && Subsystem->ClientNodeHighWater.Contains(NodeId));

        FMythicHarvestReplicatedNodeItem Available = Depleted;
        Available.Generation = 2;
        Available.Revision = 1;
        Available.State = EMythicHarvestNodeState::Available;
        Available.QuantizedMaxWork = 65535;
        Available.QuantizedRemainingWork = 65535;
        SetCellRows(*CellB, {Available});
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("a newer explicit Available row restores cooked presentation"),
                 RestoredTransform.Equals(OriginalTransform));
        TestEqual(TEXT("explicit availability remains versioned while the channel lives"),
                  Subsystem->ClientNodeState.FindChecked(NodeId).State,
                  EMythicHarvestNodeState::Available);

        Subsystem->UnregisterReplicationCell(*CellB);
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("final actor-channel close restores cooked presentation"),
                 RestoredTransform.Equals(OriginalTransform));
        TestTrue(TEXT("final source retirement bounds traversal high-water memory"),
                 Subsystem->ClientNodeState.IsEmpty()
                     && Subsystem->ClientNodeHighWater.IsEmpty()
                     && Subsystem->ClientNodeSourcesByNode.IsEmpty());

        AMythicHarvestReplicationCell *CellC =
            World->SpawnActor<AMythicHarvestReplicationCell>();
        TestNotNull(TEXT("new relevance cell spawns"), CellC);
        if (!CellC) {
            return false;
        }
        Subsystem->RegisterReplicationCell(*CellC);
        SetCellRows(*CellC, {Depleted});
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("a newly relevant channel may install its current snapshot"),
                 HiddenTransform.GetLocation().Z < -999000.0);

        FMythicHarvestReplicatedNodeItem NewerDepleted = Depleted;
        NewerDepleted.Revision = 2;
        SetCellRows(*CellC, {NewerDepleted});
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("newer authority version advances live high-water"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        Subsystem->UnregisterReplicationCell(*CellC);
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("cell relevance loss restores without version regress"),
                 RestoredTransform.Equals(OriginalTransform));

        AMythicHarvestReplicationCell *CellD =
            World->SpawnActor<AMythicHarvestReplicationCell>();
        TestNotNull(TEXT("re-entry cell spawns"), CellD);
        if (!CellD) {
            return false;
        }
        Subsystem->RegisterReplicationCell(*CellD);
        SetCellRows(*CellD, {NewerDepleted});
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("identical high-water snapshot is valid on re-entry"),
                 HiddenTransform.GetLocation().Z < -999000.0);

        FMythicHarvestReplicatedNodeItem AvailableOnD = NewerDepleted;
        AvailableOnD.Generation = 2;
        AvailableOnD.Revision = 1;
        AvailableOnD.State = EMythicHarvestNodeState::Available;
        AvailableOnD.QuantizedMaxWork = 65535;
        AvailableOnD.QuantizedRemainingWork = 65535;
        SetCellRows(*CellD, {AvailableOnD});
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("explicit availability preserves the exact original transform"),
                 RestoredTransform.Equals(OriginalTransform));

        FMythicHarvestReplicatedNodeItem LatestDepleted = AvailableOnD;
        LatestDepleted.Revision = 2;
        LatestDepleted.State = EMythicHarvestNodeState::Depleted;
        LatestDepleted.QuantizedRemainingWork = 0;
        SetCellRows(*CellD, {LatestDepleted});
        Provider->UnregisterComponent();
        TestTrue(TEXT("component unregister detaches presentation lifetime"),
                  Subsystem->ClientPresentationProviderByNode.IsEmpty()
                      && Subsystem->ClientPresentationNodesByProvider.IsEmpty());
        Provider->RegisterComponent();
        AdvanceHarvestWorldTimers(*World, 0.0f);
        TestEqual(TEXT("OnRegister retries identity indexing"),
                  Subsystem->ClientPresentationProviderByNode.Num(), 1);
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("OnRegister reapplies current depleted presentation"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        FMythicHarvestReplicatedNodeItem SecondAvailable = LatestDepleted;
        SecondAvailable.Revision = 3;
        SecondAvailable.State = EMythicHarvestNodeState::Available;
        SecondAvailable.QuantizedRemainingWork = 65535;
        SetCellRows(*CellD, {SecondAvailable});
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("register cycling does not lose original transform"),
                 RestoredTransform.Equals(OriginalTransform));

        FMythicHarvestReplicatedNodeItem OldStreamHighWater =
            SecondAvailable;
        OldStreamHighWater.Revision = 4;
        OldStreamHighWater.State = EMythicHarvestNodeState::Depleted;
        OldStreamHighWater.QuantizedRemainingWork = 0;
        SetCellRows(*CellD, {OldStreamHighWater});
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("old stream owns a high presentation watermark"),
                 HiddenTransform.GetLocation().Z < -999000.0);

        FMythicHarvestPresentationStreamToken StreamB;
        TestTrue(TEXT("authority presentation stream can rotate"),
                  FMythicHarvestPresentationStreamToken::TryAdvance(
                      StreamA, StreamB));
        AMythicHarvestReplicationCell *CellE =
            World->SpawnActor<AMythicHarvestReplicationCell>();
        TestNotNull(TEXT("future-stream cell spawns"), CellE);
        if (!CellE) {
            return false;
        }
        Subsystem->RegisterReplicationCell(*CellE);

        FMythicHarvestReplicatedNodeItem RestoredLowerVersion = Depleted;
        RestoredLowerVersion.PresentationStream = StreamB;
        RestoredLowerVersion.Generation = 1;
        RestoredLowerVersion.Revision = 1;
        SetCellRows(*CellE, {RestoredLowerVersion});
        TestTrue(TEXT("future rows wait for the GameState coordinator"),
                 Subsystem->ClientDeferredPresentationCells.Contains(CellE));
        TestTrue(TEXT("future rows cannot rotate client state themselves"),
                 Subsystem->ClientPresentationStream == StreamA);
        TestEqual(TEXT("future rows cannot lower the active high-water"),
                  Subsystem->ClientNodeHighWater.FindChecked(NodeId)
                      .Snapshot.Revision,
                  4u);

        TestTrue(TEXT("the coordinator activates the restored stream"),
                 Subsystem->ActivateClientPresentationStream(StreamB));
        TestTrue(TEXT("the client installs the coordinator stream"),
                 Subsystem->ClientPresentationStream == StreamB);
        TestEqual(TEXT("new-stream lower generation is accepted"),
                  Subsystem->ClientNodeHighWater.FindChecked(NodeId)
                      .Snapshot.Generation,
                  1u);
        TestEqual(TEXT("new-stream lower revision is accepted"),
                  Subsystem->ClientNodeHighWater.FindChecked(NodeId)
                      .Snapshot.Revision,
                  1u);
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("deferred restored state replays after activation"),
                 HiddenTransform.GetLocation().Z < -999000.0);

        Subsystem->UnregisterReplicationCell(*CellD);
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("old-cell teardown cannot remove the new stream"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        Subsystem->RegisterReplicationCell(*CellD);
        SetCellRows(*CellD, {OldStreamHighWater});
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("old-stream replay cannot overwrite the new stream"),
                 HiddenTransform.GetLocation().Z < -999000.0);

        FMythicHarvestPresentationStreamToken StreamC;
        TestTrue(TEXT("an empty replacement stream can be created"),
                  FMythicHarvestPresentationStreamToken::TryAdvance(
                      StreamB, StreamC));
        CellE->ReplicatedNodes.Items.Reset();
        Subsystem->HandleClientCellBatch(*CellE);
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("an empty Fast Array callback cannot reveal before its cell token arrives"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        CellE->ReplicatedPresentationStream = StreamC;
        Subsystem->HandleClientCellBatch(*CellE);
        Provider->GetInstanceTransform(0, HiddenTransform, true);
        TestTrue(TEXT("a future empty cell remains hidden until the coordinator barrier"),
                 HiddenTransform.GetLocation().Z < -999000.0);
        TestTrue(TEXT("the GameState coordinator activates an empty stream"),
                 Subsystem->ActivateClientPresentationStream(StreamC));
        Provider->GetInstanceTransform(0, RestoredTransform, true);
        TestTrue(TEXT("an empty stream restores cooked presentation"),
                 RestoredTransform.Equals(OriginalTransform));
        TestTrue(TEXT("an empty stream clears effective rows and high-water"),
                  Subsystem->ClientNodeState.IsEmpty()
                     && Subsystem->ClientNodeHighWater.IsEmpty()
                     && Subsystem->ClientNodeSourcesByNode.IsEmpty());

        TestFalse(TEXT("an invalid coordinator token fails closed"),
                  Subsystem->ActivateClientPresentationStream(
                      FMythicHarvestPresentationStreamToken()));
        AddExpectedError(
            TEXT("invalid or conflicting presentation-stream coordinator"),
            EAutomationExpectedErrorFlags::Contains, 2);
        TestFalse(TEXT("same-serial different-nonce coordinator conflicts"),
                   Subsystem->ActivateClientPresentationStream(
                       FMythicHarvestPresentationStreamToken(
                          FGuid(0x40000001, 0x40000002, 0x40000003,
                                0x40000004),
                           StreamC.GetSerial())));
        TestTrue(TEXT("a conflict cannot rotate the active stream"),
                  Subsystem->ClientPresentationStream == StreamC);
        TestFalse(TEXT("an exact half-range coordinator fails closed"),
                  Subsystem->ActivateClientPresentationStream(
                      FMythicHarvestPresentationStreamToken(
                          StreamC.GetNonce(),
                          StreamC.GetSerial() + 0x80000000u)));
        TestFalse(TEXT("an older coordinator cannot rotate backwards"),
                  Subsystem->ActivateClientPresentationStream(StreamB));
        TestFalse(TEXT("presentation streams never populate client WorldEpoch"),
                  Subsystem->GetWorldEpoch().IsValid());

        Subsystem->UnregisterReplicationCell(*CellD);
        Subsystem->UnregisterReplicationCell(*CellE);
        Provider->UnregisterHarvestIdentityProvider();
        Provider->UnregisterComponent();
        World->DestroyActor(Owner);
    }

    {
        FScopedHarvestPIEWorld ScopedAuthorityWorld(NM_ListenServer);
        UWorld *World = ScopedAuthorityWorld.Get();
        TestNotNull(TEXT("authority PIE world initializes"), World);
        if (!World) {
            return false;
        }
        UMythicHarvestWorldSubsystem *Subsystem =
            World->GetSubsystem<UMythicHarvestWorldSubsystem>();
        TestNotNull(TEXT("authority harvest subsystem initializes"),
                    Subsystem);
        if (!Subsystem) {
            return false;
        }
        TestTrue(TEXT("authority owns a world epoch"),
                 Subsystem->GetWorldEpoch().IsValid());

        AActor *Owner = World->SpawnActor<AActor>();
        UMythicHarvestableDefinition *Definition =
            NewObject<UMythicHarvestableDefinition>(Owner);
        Definition->MaxWork = 10.0f;
        UMythicResourceISM *Provider = CreatePackedProvider(
            *Owner, *Definition, TEXT("AuthorityProvider"), PackedNodeId);
        Provider->RegisterComponent();
        TestTrue(TEXT("authority provider registers canonical identity"),
                 Provider->RefreshHarvestIdentityRegistration());
        TestEqual(TEXT("authority owns one runtime node"),
                  Subsystem->Nodes.Num(), 1);
        TestEqual(TEXT("authority owns one provider lifetime"),
                  Subsystem->NodesByProvider.Num(), 1);

        FPrimitiveInstanceId PrimitiveId;
        FMythicHarvestNodeId ResolvedNodeId;
        TestTrue(TEXT("authority resolves valid provider identity"),
                 Provider->ResolveAuthoritativeHitInstance(
                     0, PrimitiveId, ResolvedNodeId));
        Provider->UnregisterComponent();
        TestTrue(TEXT("authority WP teardown detaches provider"),
                 Subsystem->NodesByProvider.IsEmpty());
        TestTrue(TEXT("untouched Available authority node returns to implicit state"),
                 Subsystem->Nodes.IsEmpty());
        TestTrue(TEXT("authority teardown suppresses query collision"),
                 Provider->IsHarvestQueryCollisionSuppressedForTests());
        Provider->RegisterComponent();
        AdvanceHarvestWorldTimers(*World, 0.0f);
        TestFalse(TEXT("authority WP recovery restores query collision"),
                  Provider->IsHarvestQueryCollisionSuppressedForTests());
        TestEqual(TEXT("WP recovery recreates exactly one authority node"),
                  Subsystem->Nodes.Num(), 1);

        Provider->UnregisterHarvestIdentityProvider();
        Provider->UnregisterComponent();
        World->DestroyActor(Owner);
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicHarvestPresentationStreamRestoreTest,
    "Mythic.Harvesting.Replication.SaveRestoreRotatesPresentationStream",
    EAutomationTestFlags_ApplicationContextMask
        | EAutomationTestFlags::ProductFilter)

bool FMythicHarvestPresentationStreamRestoreTest::RunTest(
    const FString & /*Parameters*/) {
    FScopedHarvestPIEWorld ScopedAuthorityWorld(NM_ListenServer);
    UWorld *World = ScopedAuthorityWorld.Get();
    TestNotNull(TEXT("authority restore world initializes"), World);
    if (!World) {
        return false;
    }

    AMythicHarvestReplicationTestGameState *GameState =
        World->SpawnActor<AMythicHarvestReplicationTestGameState>();
    TestNotNull(TEXT("concrete harvest coordinator GameState spawns"),
                GameState);
    if (!GameState) {
        return false;
    }
    World->SetGameState(GameState);

    UMythicHarvestWorldSubsystem *Subsystem =
        World->GetSubsystem<UMythicHarvestWorldSubsystem>();
    TestNotNull(TEXT("authority harvest subsystem initializes"), Subsystem);
    if (!Subsystem) {
        return false;
    }
    Subsystem->RegisterPresentationCoordinator(*GameState);
    TestTrue(TEXT("authority coordinator is synchronized"),
             Subsystem->HasReadyAuthorityPresentationCoordinator());

    const FMythicHarvestNodeId NodeId(
        FGuid(0x55555555, 0x66666666, 0x77777777, 0x88888888));
    TArray<float> PackedNodeId;
    TestTrue(TEXT("restore fixture identity packs"),
             MythicHarvestPCGIdentity::AppendPackedNodeId(NodeId,
                                                          PackedNodeId));

    AActor *Owner = World->SpawnActor<AActor>();
    UMythicHarvestableDefinition *Definition =
        Owner ? NewObject<UMythicHarvestableDefinition>(Owner) : nullptr;
    TestNotNull(TEXT("restore fixture owner spawns"), Owner);
    TestNotNull(TEXT("restore fixture definition allocates"), Definition);
    if (!Owner || !Definition) {
        return false;
    }
    Definition->MaxWork = 10.0f;
    UMythicResourceISM *Provider = CreatePackedProvider(
        *Owner, *Definition, TEXT("RestoreAuthorityProvider"),
        PackedNodeId);
    Provider->RegisterComponent();
    TestTrue(TEXT("restore fixture authority provider registers"),
             Provider->RefreshHarvestIdentityRegistration());

    UMythicHarvestWorldSubsystem::FRuntimeNode *Node =
        Subsystem->Nodes.Find(NodeId);
    TestNotNull(TEXT("restore fixture runtime node exists"), Node);
    if (!Node) {
        return false;
    }
    Node->State = EMythicHarvestNodeState::Depleted;
    Node->RemainingWork = FMythicHarvestWork();
    Node->Generation = 99;
    Node->Revision = 101;
    Node->RespawnServerDeadline = 120.0;
    TestTrue(TEXT("old stream node delta publishes"),
             Subsystem->PublishNodeDelta(*Node));

    TestEqual(TEXT("one authority spatial cell carries the old stream"),
              Subsystem->AuthorityCells.Num(), 1);
    if (Subsystem->AuthorityCells.Num() != 1) {
        return false;
    }
    AMythicHarvestReplicationCell *Cell =
        Subsystem->AuthorityCells.CreateConstIterator().Value().Get();
    TestNotNull(TEXT("old-stream authority cell exists"), Cell);
    if (!Cell) {
        return false;
    }
    TestEqual(TEXT("old stream publishes one row"),
              Cell->GetNodeDeltas().Num(), 1);
    if (Cell->GetNodeDeltas().Num() != 1) {
        return false;
    }
    TestEqual(TEXT("old stream publishes its high generation"),
              Cell->GetNodeDeltas()[0].Generation, 99u);

    const FGuid OldWorldEpoch = Subsystem->GetWorldEpoch();
    const FMythicHarvestPresentationStreamToken OldPresentationStream =
        Subsystem->AuthorityPresentationStream;
    FMythicHarvestWorldSaveV1 Snapshot;
    Snapshot.WorldEpoch = FGuid(
        0x90000001, 0x90000002, 0x90000003, 0x90000004);
    FMythicSavedHarvestNodeV1 &Saved = Snapshot.Nodes.AddDefaulted_GetRef();
    Saved.NodeGuid = NodeId.GetGuid();
    Saved.WorldEpoch = Snapshot.WorldEpoch;
    Saved.Generation = 1;
    Saved.Revision = 1;
    Saved.ReplicationCellCoordinate = Node->ReplicationCellCoordinate;
    Saved.ReplicationCellCenterZ =
        static_cast<float>(Node->OriginalWorldLocation.Z);
    Saved.State = EMythicHarvestNodeState::Depleted;
    Saved.RemainingRespawnSeconds = 30.0;

    FName Diagnostic;
    UMythicHarvestRewardOutboxSubsystem *RewardOutbox =
        World->GetSubsystem<UMythicHarvestRewardOutboxSubsystem>();
    TestNotNull(TEXT("restore fixture reward outbox initializes"),
                RewardOutbox);
    if (!RewardOutbox) {
        return false;
    }
    FMythicHarvestRewardOutboxSaveV1 MissingReceiptSnapshot;
    MissingReceiptSnapshot.WorldEpoch = Snapshot.WorldEpoch;
    MissingReceiptSnapshot.SnapshotSequence = 1;
    TestTrue(TEXT("receipt rejection fixture acquires its transaction gate"),
             Subsystem->BeginSaveRestore(Diagnostic));
    TestFalse(TEXT("restore cannot complete before preflight and apply"),
              Subsystem->CompleteSaveRestore(Diagnostic));
    TestFalse(TEXT("world apply cannot run before cross-domain preflight"),
              Subsystem->RestoreSaveSnapshot(Snapshot, Diagnostic));
    TestEqual(TEXT("preflight phase failure uses a stable diagnostic"),
              Diagnostic,
              FName(TEXT("HarvestRestoreNotPreflighted")));
    TestFalse(TEXT("unavailable preflight fails without exact completion history"),
              Subsystem->PreflightSaveRestore(
                  Snapshot, MissingReceiptSnapshot, Diagnostic));
    TestEqual(TEXT("missing completion uses a stable diagnostic"),
              Diagnostic,
              FName(TEXT("HarvestMissingCompletionReceipt")));
    Subsystem->AbortSaveRestore();

    FMythicHarvestRewardOutboxSaveV1 RewardSnapshot;
    RewardSnapshot.WorldEpoch = Snapshot.WorldEpoch;
    RewardSnapshot.SnapshotSequence = 1;
    FMythicSavedHarvestRewardCompletionV1 &KnownCompletion =
        RewardSnapshot.KnownCompletions.AddDefaulted_GetRef();
    KnownCompletion.WorldEpoch = Snapshot.WorldEpoch;
    KnownCompletion.NodeGuid = NodeId.GetGuid();
    KnownCompletion.Generation = Saved.Generation;
    FMythicSavedHarvestGenerationHighWaterV1 &GenerationHighWater =
        RewardSnapshot.GenerationHighWatermarks.AddDefaulted_GetRef();
    GenerationHighWater.WorldEpoch = Snapshot.WorldEpoch;
    GenerationHighWater.NodeGuid = NodeId.GetGuid();
    GenerationHighWater.HighestKnownGeneration = Saved.Generation;
    TestTrue(TEXT("first restore acquires its transaction gate"),
             Subsystem->BeginSaveRestore(Diagnostic));
    TestTrue(TEXT("world/outbox restore pair preflights before mutation"),
             Subsystem->PreflightSaveRestore(
                 Snapshot, RewardSnapshot, Diagnostic));
    FMythicHarvestWorldSaveV1 UnboundSnapshot = Snapshot;
    UnboundSnapshot.Nodes[0].Revision += 1;
    TestFalse(TEXT("a preflight for one payload cannot apply another payload"),
              Subsystem->RestoreSaveSnapshot(
                  UnboundSnapshot, Diagnostic));
    TestEqual(TEXT("payload substitution fails the canonical transaction binding"),
              Diagnostic,
              FName(TEXT("HarvestRestorePayloadBindingMismatch")));
    TestTrue(TEXT("matching reward completion history restores after preflight"),
             RewardOutbox->RestoreSaveSnapshot(RewardSnapshot,
                                               Diagnostic));
    TestTrue(TEXT("in-place restore succeeds"),
             Subsystem->RestoreSaveSnapshot(Snapshot, Diagnostic));
    TestTrue(TEXT("restore clears its diagnostic"), Diagnostic.IsNone());
    TestFalse(TEXT("one preflight cannot apply the same world snapshot twice"),
              Subsystem->RestoreSaveSnapshot(Snapshot, Diagnostic));
    TestEqual(TEXT("replayed apply is rejected by the transaction phase"),
              Diagnostic, FName(TEXT("HarvestRestoreNotPreflighted")));
    TestTrue(TEXT("full-world restore barrier releases explicitly"),
             Subsystem->CompleteSaveRestore(Diagnostic));
    TestNotEqual(TEXT("fixture loads a different persisted world epoch"),
                 Snapshot.WorldEpoch, OldWorldEpoch);
    TestEqual(TEXT("restore adopts the persisted durable authority epoch"),
              Subsystem->GetWorldEpoch(), Snapshot.WorldEpoch);
    TestEqual(TEXT("presentation stream advances independently"),
              FMythicHarvestPresentationStreamToken::Compare(
                  Subsystem->AuthorityPresentationStream,
                  OldPresentationStream),
              EMythicHarvestPresentationStreamOrder::Newer);
    TestTrue(TEXT("GameState publishes the new opaque stream"),
             GameState->HarvestPresentationStreamToken
                 == Subsystem->AuthorityPresentationStream);

    TestEqual(TEXT("the existing cell is repopulated once"),
              Cell->GetNodeDeltas().Num(), 1);
    if (Cell->GetNodeDeltas().Num() == 1) {
        const FMythicHarvestReplicatedNodeItem &Restored =
            Cell->GetNodeDeltas()[0];
        TestTrue(TEXT("restored row carries the new stream"),
                 Restored.PresentationStream
                     == Subsystem->AuthorityPresentationStream);
        TestEqual(TEXT("new stream accepts restored lower generation"),
                  Restored.Generation, 1u);
        TestEqual(TEXT("new stream accepts restored lower revision"),
                  Restored.Revision, 1u);
    }

    const FMythicHarvestPresentationStreamToken RestoredPresentationStream =
        Subsystem->AuthorityPresentationStream;
    FMythicHarvestWorldSaveV1 EmptySnapshot;
    EmptySnapshot.WorldEpoch = Snapshot.WorldEpoch;
    TestTrue(TEXT("empty restore acquires its transaction gate"),
             Subsystem->BeginSaveRestore(Diagnostic));
    TestTrue(TEXT("empty replacement pair preflights"),
             Subsystem->PreflightSaveRestore(
                 EmptySnapshot, RewardSnapshot, Diagnostic));
    TestTrue(TEXT("an empty in-place restore succeeds"),
             Subsystem->RestoreSaveSnapshot(EmptySnapshot, Diagnostic));
    TestTrue(TEXT("empty full-world restore barrier releases"),
             Subsystem->CompleteSaveRestore(Diagnostic));
    TestEqual(TEXT("an empty restore advances the coordinator stream"),
              FMythicHarvestPresentationStreamToken::Compare(
                  Subsystem->AuthorityPresentationStream,
                  RestoredPresentationStream),
              EMythicHarvestPresentationStreamOrder::Newer);
    TestTrue(TEXT("GameState publishes the empty replacement stream"),
             GameState->HarvestPresentationStreamToken
                 == Subsystem->AuthorityPresentationStream);
    TestTrue(TEXT("an empty replacement stream clears the existing cell"),
             Cell->GetNodeDeltas().IsEmpty());
    Node = Subsystem->Nodes.Find(NodeId);
    TestNotNull(TEXT("implicit available node remains resident"), Node);
    if (Node) {
        TestEqual(TEXT("implicit available node advances from durable high-water"),
                  Node->Generation, 2u);
    }

    FMythicHarvestReplicatedNodeItem StaleOldStreamRow;
    StaleOldStreamRow.PresentationStream = OldPresentationStream;
    StaleOldStreamRow.NodeId = NodeId;
    StaleOldStreamRow.Generation = MAX_uint32;
    StaleOldStreamRow.Revision = MAX_uint32;
    StaleOldStreamRow.State = EMythicHarvestNodeState::Depleted;
    StaleOldStreamRow.QuantizedMaxWork = 65535;
    TestFalse(TEXT("a stale stream cannot upsert despite higher node counters"),
              Cell->UpsertNodeDelta(StaleOldStreamRow));
    TestFalse(TEXT("a cell cannot rotate its stream backwards"),
              Cell->ResetForPresentationStream(OldPresentationStream));

    Provider->UnregisterHarvestIdentityProvider();
    Provider->UnregisterComponent();
    World->DestroyActor(Owner);
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS && WITH_EDITOR
