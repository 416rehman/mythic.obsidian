#include "World/Harvesting/MythicHarvestReplicationCell.h"

#include "Components/SceneComponent.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "World/Harvesting/MythicHarvestWorldSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestReplicationCell)

namespace {
constexpr double DefaultHarvestCellCullDistanceCentimeters = 50000.0;

bool IsFiniteHarvestReplicationVector(const FVector &Value) {
    return FMath::IsFinite(Value.X) && FMath::IsFinite(Value.Y)
        && FMath::IsFinite(Value.Z);
}

bool IsReplicableNodeState(const EMythicHarvestNodeState State) {
    switch (State) {
        case EMythicHarvestNodeState::Available:
        case EMythicHarvestNodeState::Depleted:
        case EMythicHarvestNodeState::Regrowing:
            return true;
        default:
            return false;
    }
}

int32 CompareWrappedCounter(const uint32 Left, const uint32 Right) {
    if (Left == Right) {
        return 0;
    }

    // RFC-1982-style serial arithmetic keeps authority's nonzero uint32 wrap
    // ordered correctly. Exactly half-range-apart values are intrinsically
    // ambiguous, so use numeric order as a deterministic fail-closed tie break.
    const uint32 ForwardDistance = Left - Right;
    if (ForwardDistance == 0x80000000u) {
        return Left < Right ? -1 : 1;
    }
    return ForwardDistance < 0x80000000u ? 1 : -1;
}
}

bool FMythicHarvestReplicatedNodeItem::TryCompareVersion(
    const FMythicHarvestReplicatedNodeItem &Left,
    const FMythicHarvestReplicatedNodeItem &Right,
    int32 &OutComparison) {
    OutComparison = 0;
    if (!Left.PresentationStream.IsValid()
        || Left.PresentationStream != Right.PresentationStream) {
        return false;
    }
    const int32 GenerationOrder =
        CompareWrappedCounter(Left.Generation, Right.Generation);
    if (GenerationOrder != 0) {
        OutComparison = GenerationOrder;
        return true;
    }
    OutComparison = CompareWrappedCounter(Left.Revision, Right.Revision);
    return true;
}

bool FMythicHarvestReplicatedNodeItem::HasSameReplicatedPayload(
    const FMythicHarvestReplicatedNodeItem &Other) const {
    return PresentationStream == Other.PresentationStream
        && NodeId == Other.NodeId && Generation == Other.Generation
        && Revision == Other.Revision && State == Other.State
        && QuantizedRemainingWork == Other.QuantizedRemainingWork
        && QuantizedMaxWork == Other.QuantizedMaxWork
        && RespawnServerDeadline == Other.RespawnServerDeadline;
}

void FMythicHarvestReplicatedNodeItem::CopyReplicatedPayloadFrom(
    const FMythicHarvestReplicatedNodeItem &Other) {
    PresentationStream = Other.PresentationStream;
    NodeId = Other.NodeId;
    Generation = Other.Generation;
    Revision = Other.Revision;
    State = Other.State;
    QuantizedRemainingWork = Other.QuantizedRemainingWork;
    QuantizedMaxWork = Other.QuantizedMaxWork;
    RespawnServerDeadline = Other.RespawnServerDeadline;
}

void FMythicHarvestReplicatedNodeArray::PostReplicatedAdd(
    const TArrayView<int32> &AddedIndices, const int32 /*FinalSize*/) {
    if (!Owner) {
        return;
    }
    for (const int32 Index : AddedIndices) {
        if (Items.IsValidIndex(Index)) {
            Owner->HandleReplicatedNodeAdded(Items[Index]);
        }
    }
}

void FMythicHarvestReplicatedNodeArray::PostReplicatedChange(
    const TArrayView<int32> &ChangedIndices, const int32 /*FinalSize*/) {
    if (!Owner) {
        return;
    }
    for (const int32 Index : ChangedIndices) {
        if (Items.IsValidIndex(Index)) {
            Owner->HandleReplicatedNodeChanged(Items[Index]);
        }
    }
}

void FMythicHarvestReplicatedNodeArray::PreReplicatedRemove(
    const TArrayView<int32> &RemovedIndices, const int32 /*FinalSize*/) {
    if (!Owner) {
        return;
    }
    for (const int32 Index : RemovedIndices) {
        if (Items.IsValidIndex(Index)) {
            Owner->HandleReplicatedNodeRemoved(Items[Index]);
        }
    }
}

void FMythicHarvestReplicatedNodeArray::PostReplicatedReceive(
    const FFastArraySerializer::FPostReplicatedReceiveParameters & /*Parameters*/) {
    if (Owner) {
        Owner->HandleReplicationBatchReceived();
    }
}

AMythicHarvestReplicationCell::AMythicHarvestReplicationCell(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    PrimaryActorTick.bCanEverTick = false;
    PrimaryActorTick.bStartWithTickEnabled = false;

    SetReplicates(true);
    SetReplicateMovement(false);
    bAlwaysRelevant = false;
    bOnlyRelevantToOwner = false;
    bNetUseOwnerRelevancy = false;
    bNetLoadOnClient = false;
    NetDormancy = DORM_DormantAll;
    SetNetUpdateFrequency(10.0f);
    SetMinNetUpdateFrequency(1.0f);
    NetPriority = 1.0f;
    SetNetCullDistanceSquared(FMath::Square(
        static_cast<float>(DefaultHarvestCellCullDistanceCentimeters)));

    USceneComponent *SpatialRoot =
        ObjectInitializer.CreateDefaultSubobject<USceneComponent>(this, TEXT("SpatialRoot"));
    SetRootComponent(SpatialRoot);

    ReplicatedNodes.SetOwner(this);
}

void AMythicHarvestReplicationCell::BeginPlay() {
    Super::BeginPlay();
    if (UWorld *World = GetWorld()) {
        if (UMythicHarvestWorldSubsystem *Subsystem =
                World->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
            Subsystem->RegisterReplicationCell(*this);
        }
    }
}

void AMythicHarvestReplicationCell::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // The value is immutable after configuration, so ordinary delta replication costs no recurring bandwidth and
    // still handles a cell configured after an existing actor channel produced its initial bunch.
    DOREPLIFETIME(ThisClass, CellCoordinate);
    DOREPLIFETIME(ThisClass, ReplicatedPresentationStream);
    DOREPLIFETIME(ThisClass, ReplicatedNodes);
}

bool AMythicHarvestReplicationCell::IsNetRelevantFor(
    const AActor * /*RealViewer*/, const AActor * /*ViewTarget*/,
    const FVector &SrcLocation) const {
    if (!bSpatialCellConfigured || !IsFiniteHarvestReplicationVector(SrcLocation)) {
        return false;
    }

    // Resource cells are horizontal world buckets. A cavern and the surface may legitimately share XY, so vertical
    // separation must not remove authoritative state from relevance. The server still applies the configured radial
    // budget in the horizontal plane and retains ordinary network cull scalability.
    return FVector::DistSquaredXY(GetActorLocation(), SrcLocation)
        <= static_cast<double>(GetNetCullDistanceSquared());
}

void AMythicHarvestReplicationCell::EndPlay(
    const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        if (UMythicHarvestWorldSubsystem *Subsystem =
                World->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
            Subsystem->UnregisterReplicationCell(*this);
        }
    }
    OnCellEndingPlay.Broadcast(*this);
    OnNodeDeltaAdded.Clear();
    OnNodeDeltaChanged.Clear();
    OnNodeDeltaRemoved.Clear();
    OnReplicationBatchReceived.Clear();
    OnCellEndingPlay.Clear();
    NodeIndexById.Reset();

    Super::EndPlay(EndPlayReason);
}

bool AMythicHarvestReplicationCell::ConfigureSpatialCell(
    const FIntPoint InCellCoordinate, const FVector &InCellCenter,
    const double InNetCullDistanceCentimeters,
    const FMythicHarvestPresentationStreamToken &InPresentationStream) {
    if (!HasAuthority() || bSpatialCellConfigured
        || !IsFiniteHarvestReplicationVector(InCellCenter)
        || !FMath::IsFinite(InNetCullDistanceCentimeters)
        || InNetCullDistanceCentimeters <= KINDA_SMALL_NUMBER
        || !InPresentationStream.IsValid()) {
        return false;
    }

    const double CullDistanceSquared =
        InNetCullDistanceCentimeters * InNetCullDistanceCentimeters;
    if (!FMath::IsFinite(CullDistanceSquared)
        || CullDistanceSquared > static_cast<double>(MAX_flt)) {
        return false;
    }

    // Configuration mutates replicated identity and the spatial location used by the NetDriver/Replication Graph.
    // Flush first so a runtime-spawned dormant cell cannot silently retain an origin/default snapshot.
    WakeForReplicatedMutation();
    CellCoordinate = InCellCoordinate;
    SetActorLocation(InCellCenter, false, nullptr, ETeleportType::TeleportPhysics);
    SetNetCullDistanceSquared(static_cast<float>(CullDistanceSquared));
    ReplicatedPresentationStream = InPresentationStream;
    bSpatialCellConfigured = true;
    ForceNetUpdate();
    return true;
}

bool AMythicHarvestReplicationCell::CanResetForPresentationStream(
    const FMythicHarvestPresentationStreamToken &NextStream) const {
    return HasAuthority() && bSpatialCellConfigured
        && FMythicHarvestPresentationStreamToken::Compare(
               NextStream, ReplicatedPresentationStream)
            == EMythicHarvestPresentationStreamOrder::Newer;
}

bool AMythicHarvestReplicationCell::ResetForPresentationStream(
    const FMythicHarvestPresentationStreamToken &NextStream) {
    if (!CanResetForPresentationStream(NextStream)) {
        return false;
    }

    WakeForReplicatedMutation();
    ReplicatedPresentationStream = NextStream;
    ReplicatedNodes.Items.Reset();
    ReplicatedNodes.MarkArrayDirty();
    NodeIndexById.Reset();
    ForceNetUpdate();
    return true;
}

bool AMythicHarvestReplicationCell::UpsertNodeDelta(
    const FMythicHarvestReplicatedNodeItem &Delta) {
    if (!HasAuthority() || !bSpatialCellConfigured
        || Delta.PresentationStream != ReplicatedPresentationStream
        || !Delta.NodeId.IsValid() || Delta.Generation == 0
        || Delta.Revision == 0 || !IsReplicableNodeState(Delta.State)
        || Delta.QuantizedRemainingWork > Delta.QuantizedMaxWork
        || !FMath::IsFinite(Delta.RespawnServerDeadline)
        || Delta.RespawnServerDeadline < 0.0) {
        return false;
    }

    int32 *FoundIndex = NodeIndexById.Find(Delta.NodeId);
    if (FoundIndex
        && (!ReplicatedNodes.Items.IsValidIndex(*FoundIndex)
            || !(ReplicatedNodes.Items[*FoundIndex].NodeId == Delta.NodeId))) {
        RebuildNodeIndex();
        FoundIndex = NodeIndexById.Find(Delta.NodeId);
    }

    if (!FoundIndex) {
        WakeForReplicatedMutation();
        FMythicHarvestReplicatedNodeItem &Added =
            ReplicatedNodes.Items.AddDefaulted_GetRef();
        Added.CopyReplicatedPayloadFrom(Delta);
        const int32 AddedIndex = ReplicatedNodes.Items.Num() - 1;
        NodeIndexById.Add(Added.NodeId, AddedIndex);
        ReplicatedNodes.MarkItemDirty(Added);
        ForceNetUpdate();
        return true;
    }

    FMythicHarvestReplicatedNodeItem &Existing =
        ReplicatedNodes.Items[*FoundIndex];
    int32 VersionOrder = 0;
    if (!FMythicHarvestReplicatedNodeItem::TryCompareVersion(
            Delta, Existing, VersionOrder)) {
        return false;
    }
    if (VersionOrder == 0) {
        return Existing.HasSameReplicatedPayload(Delta);
    }
    if (VersionOrder < 0) {
        return false;
    }

    WakeForReplicatedMutation();
    Existing.CopyReplicatedPayloadFrom(Delta);
    ReplicatedNodes.MarkItemDirty(Existing);
    ForceNetUpdate();
    return true;
}

const FMythicHarvestReplicatedNodeItem *
AMythicHarvestReplicationCell::FindNodeDelta(
    const FMythicHarvestNodeId &NodeId) const {
    const int32 *FoundIndex = NodeIndexById.Find(NodeId);
    if (FoundIndex && ReplicatedNodes.Items.IsValidIndex(*FoundIndex)
        && ReplicatedNodes.Items[*FoundIndex].NodeId == NodeId) {
        return &ReplicatedNodes.Items[*FoundIndex];
    }

    return ReplicatedNodes.Items.FindByPredicate(
        [&NodeId](const FMythicHarvestReplicatedNodeItem &Item) {
            return Item.NodeId == NodeId;
        });
}

void AMythicHarvestReplicationCell::HandleReplicatedNodeAdded(
    const FMythicHarvestReplicatedNodeItem &Item) {
    OnNodeDeltaAdded.Broadcast(*this, Item);
}

void AMythicHarvestReplicationCell::HandleReplicatedNodeChanged(
    const FMythicHarvestReplicatedNodeItem &Item) {
    OnNodeDeltaChanged.Broadcast(*this, Item);
}

void AMythicHarvestReplicationCell::HandleReplicatedNodeRemoved(
    const FMythicHarvestReplicatedNodeItem &Item) {
    OnNodeDeltaRemoved.Broadcast(*this, Item);
}

void AMythicHarvestReplicationCell::HandleReplicationBatchReceived() {
    RebuildNodeIndex();
    OnReplicationBatchReceived.Broadcast(*this);
}

void AMythicHarvestReplicationCell::OnRep_PresentationStream() {
    // Replicated properties and Fast Array callbacks may be delivered in either callback order. Replaying the final
    // cell snapshot here makes the cell-level token the authoritative classifier for both populated and empty batches.
    HandleReplicationBatchReceived();
}

void AMythicHarvestReplicationCell::WakeForReplicatedMutation() {
    // Dormant Fast Arrays must be flushed before their shadow state changes. Flushing afterwards can cause the
    // mutation to be copied into the new shadow state and never observed as a delta by connected clients.
    FlushNetDormancy();
}

void AMythicHarvestReplicationCell::RebuildNodeIndex() {
    NodeIndexById.Reset();
    NodeIndexById.Reserve(ReplicatedNodes.Items.Num());
    for (int32 Index = 0; Index < ReplicatedNodes.Items.Num(); ++Index) {
        NodeIndexById.Add(ReplicatedNodes.Items[Index].NodeId, Index);
    }
}
