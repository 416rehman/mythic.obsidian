#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Info.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "World/Harvesting/MythicHarvestReplicationTypes.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#include "MythicHarvestReplicationCell.generated.h"

class AMythicHarvestReplicationCell;

/**
 * Authoritative, versioned presentation state for one harvest node.
 *
 * The Fast Array replication identifiers inherited from FFastArraySerializerItem are transport details only.
 * PresentationStream scopes ordering across in-place restore; Generation and Revision order rows only within that
 * stream and must be advanced by the server-owned harvest subsystem.
 */
USTRUCT()
struct MYTHIC_API FMythicHarvestReplicatedNodeItem : public FFastArraySerializerItem {
    GENERATED_BODY()

    /** Opaque client-presentation incarnation; this is unrelated to the authority-only durable WorldEpoch. */
    UPROPERTY()
    FMythicHarvestPresentationStreamToken PresentationStream;

    /** Stable cooked-world identity; runtime ISM indices and component references never cross the network. */
    UPROPERTY()
    FMythicHarvestNodeId NodeId;

    /** Nonzero lifecycle generation ordered with uint32 serial arithmetic across authority wrap. */
    UPROPERTY()
    uint32 Generation = 0;

    /** Nonzero revision within the authoritative node stream, ordered safely across uint32 wrap. */
    UPROPERTY()
    uint32 Revision = 0;

    /** Current authoritative lifecycle state; clients use it for presentation only. */
    UPROPERTY()
    EMythicHarvestNodeState State = static_cast<EMythicHarvestNodeState>(0);

    /** Fixed-point remaining work used for progress presentation; its quantum is owned by harvest settings. */
    UPROPERTY()
    uint16 QuantizedRemainingWork = 0;

    /** Fixed-point maximum work paired with QuantizedRemainingWork. */
    UPROPERTY()
    uint16 QuantizedMaxWork = 0;

    /** Server world-time deadline in seconds; zero means this state has no active respawn deadline. */
    UPROPERTY()
    double RespawnServerDeadline = 0.0;

    /**
     * Compares gameplay versions without consulting Fast Array transport bookkeeping.
     * Returns false unless both rows belong to the same valid presentation stream.
     */
    static bool TryCompareVersion(
        const FMythicHarvestReplicatedNodeItem &Left,
        const FMythicHarvestReplicatedNodeItem &Right,
        int32 &OutComparison);

    /** Returns true when all replicated gameplay fields are identical. */
    bool HasSameReplicatedPayload(const FMythicHarvestReplicatedNodeItem &Other) const;

    /** Copies gameplay fields while preserving this item's Fast Array replication identity. */
    void CopyReplicatedPayloadFrom(const FMythicHarvestReplicatedNodeItem &Other);
};

/** Native notification for one received node delta; it never grants mutation authority to listeners. */
DECLARE_MULTICAST_DELEGATE_TwoParams(FMythicHarvestReplicatedNodeDelegate,
                                    AMythicHarvestReplicationCell &,
                                    const FMythicHarvestReplicatedNodeItem &);

/** Native notification emitted after a received Fast Array batch is fully reconciled. */
DECLARE_MULTICAST_DELEGATE_OneParam(FMythicHarvestReplicationCellDelegate,
                                   AMythicHarvestReplicationCell &);

/** Fast Array wrapper for the changed-node snapshot owned by one spatial replication cell. */
USTRUCT()
struct MYTHIC_API FMythicHarvestReplicatedNodeArray : public FFastArraySerializer {
    GENERATED_BODY()

public:
    /** Associates callbacks with the actor that owns this non-shareable array. */
    void SetOwner(AMythicHarvestReplicationCell *InOwner) { Owner = InOwner; }

    /** Read-only snapshot view; callers must use the cell's authority-only mutation API. */
    const TArray<FMythicHarvestReplicatedNodeItem> &GetItems() const { return Items; }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FMythicHarvestReplicatedNodeItem,
                                       FMythicHarvestReplicatedNodeArray>(Items, DeltaParms, *this);
    }

    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedReceive(const FFastArraySerializer::FPostReplicatedReceiveParameters &Parameters);

private:
    friend class AMythicHarvestReplicationCell;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FMythicHarvestClientPresentationRegistryTest;
#endif

    UPROPERTY()
    TArray<FMythicHarvestReplicatedNodeItem> Items;

    AMythicHarvestReplicationCell *Owner = nullptr;
};

template <>
struct TStructOpsTypeTraits<FMythicHarvestReplicatedNodeArray>
    : TStructOpsTypeTraitsBase2<FMythicHarvestReplicatedNodeArray> {
    enum { WithNetDeltaSerializer = true };
};

/**
 * Runtime-spawned, spatially relevant replication proxy for changed harvest nodes in one configured grid bucket.
 *
 * It has no owner, never mutates gameplay on clients, and is dormant between authoritative Fast Array changes.
 * Static transforms and harvestable definitions remain cooked world content and are deliberately absent here.
 */
UCLASS(NotBlueprintable, NotPlaceable, Transient)
class MYTHIC_API AMythicHarvestReplicationCell final : public AInfo {
    GENERATED_BODY()

public:
    AMythicHarvestReplicationCell(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    virtual void BeginPlay() override;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    virtual bool IsNetRelevantFor(const AActor *RealViewer,
                                  const AActor *ViewTarget,
                                  const FVector &SrcLocation) const override;

    /**
     * Configures immutable grid identity, spatial location, and cull radius before first replication.
     * Authority only; returns false for repeat calls or non-finite/non-positive cull distances.
     */
    bool ConfigureSpatialCell(
        FIntPoint InCellCoordinate, const FVector &InCellCenter,
        double InNetCullDistanceCentimeters,
        const FMythicHarvestPresentationStreamToken &InPresentationStream);

    /**
     * Returns whether authority can atomically clear this cell into a strictly newer presentation stream.
     * This pure preflight lets a whole-world restore reject before mutating any cell.
     */
    bool CanResetForPresentationStream(
        const FMythicHarvestPresentationStreamToken &NextStream) const;

    /**
     * Flushes dormancy, clears every old Fast Array row, and installs a strictly newer authority stream.
     * New lower generation/revision rows may be inserted only after this succeeds.
     */
    bool ResetForPresentationStream(
        const FMythicHarvestPresentationStreamToken &NextStream);

    /**
     * Adds or advances one changed-node snapshot row on authority.
     * The row must match this cell's configured stream. Equal versions are idempotent only when their complete
     * payload matches; older/conflicting versions fail closed.
     */
    bool UpsertNodeDelta(const FMythicHarvestReplicatedNodeItem &Delta);

    /** Returns one current snapshot row, or null when this cell carries no change for NodeId. */
    const FMythicHarvestReplicatedNodeItem *FindNodeDelta(const FMythicHarvestNodeId &NodeId) const;

    /** Returns the immutable grid coordinate replicated with this cell. */
    FIntPoint GetCellCoordinate() const { return CellCoordinate; }

    /** Returns the cell-level stream barrier, including when the changed-node snapshot is empty. */
    const FMythicHarvestPresentationStreamToken &GetPresentationStream() const {
        return ReplicatedPresentationStream;
    }

    /** Returns the complete changed-node snapshot in arbitrary Fast Array order. */
    const TArray<FMythicHarvestReplicatedNodeItem> &GetNodeDeltas() const {
        return ReplicatedNodes.GetItems();
    }

    /** Received after a client adds one replicated row. Presentation subscribers may reconcile by stable ID. */
    FMythicHarvestReplicatedNodeDelegate OnNodeDeltaAdded;

    /** Received after a client advances one replicated row. Presentation subscribers may reconcile by version. */
    FMythicHarvestReplicatedNodeDelegate OnNodeDeltaChanged;

    /** Received before transport removes one row; gameplay state remains versioned and never derives from absence. */
    FMythicHarvestReplicatedNodeDelegate OnNodeDeltaRemoved;

    /** Received after each complete Fast Array replication batch, including its initial snapshot. */
    FMythicHarvestReplicationCellDelegate OnReplicationBatchReceived;

    /** Received during EndPlay so client caches can discard every delta owned by this cell. */
    FMythicHarvestReplicationCellDelegate OnCellEndingPlay;

private:
    friend struct FMythicHarvestReplicatedNodeArray;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FMythicHarvestClientPresentationRegistryTest;
#endif

    void HandleReplicatedNodeAdded(const FMythicHarvestReplicatedNodeItem &Item);
    void HandleReplicatedNodeChanged(const FMythicHarvestReplicatedNodeItem &Item);
    void HandleReplicatedNodeRemoved(const FMythicHarvestReplicatedNodeItem &Item);
    void HandleReplicationBatchReceived();
    void WakeForReplicatedMutation();
    void RebuildNodeIndex();

    UFUNCTION()
    void OnRep_PresentationStream();

    /** Spatial grid coordinate; immutable after ConfigureSpatialCell succeeds. */
    UPROPERTY(Replicated)
    FIntPoint CellCoordinate = FIntPoint::ZeroValue;

    /** Cell-level restore barrier; unlike row tokens, this survives an empty Fast Array clear bunch. */
    UPROPERTY(ReplicatedUsing = OnRep_PresentationStream)
    FMythicHarvestPresentationStreamToken ReplicatedPresentationStream;

    /** Current changed-node snapshot; server mutation is restricted to UpsertNodeDelta/RemoveNodeDelta. */
    UPROPERTY(Replicated)
    FMythicHarvestReplicatedNodeArray ReplicatedNodes;

    TMap<FMythicHarvestNodeId, int32> NodeIndexById;
    bool bSpatialCellConfigured = false;
};
