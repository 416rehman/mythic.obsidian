#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "World/Harvesting/MythicHarvestClaimMembershipSubsystem.h"
#include "World/Harvesting/MythicHarvestParticipantSnapshot.h"
#include "World/Harvesting/MythicHarvestReplicationCell.h"
#include "World/Harvesting/MythicHarvestSaveTypes.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#include "MythicHarvestWorldSubsystem.generated.h"

class AMythicHarvestReplicationCell;
class AMythicGameState;
class AMythicPlayerController;
class UAttackFragment;
class UMythicHarvestableDefinition;
class UMythicHarvestToolTypeDefinition;
class UMythicResourceISM;
class UMythicWeaponAttackAbility;
struct FMythicHarvestRewardOutboxSaveV1;

/**
 * Sole authoritative owner of harvest-node work, lifecycle, claims, progression commits, and spatial replication.
 * Resource ISMs are identity/presentation providers only; attack abilities submit exact server provenance and never
 * mutate nodes directly. On clients this subsystem only reconciles replicated presentation by stable node identity.
 */
UCLASS()
class MYTHIC_API UMythicHarvestWorldSubsystem final : public UTickableWorldSubsystem {
    GENERATED_BODY()

public:
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    virtual void Tick(float DeltaTime) override;
    virtual TStatId GetStatId() const override;
    virtual bool IsTickableInEditor() const override { return false; }
    virtual bool IsTickableWhenPaused() const override { return false; }

    /**
     * Atomically replaces one authority-world ISM provider's complete stable-identity batch. Clients are rejected;
     * they must use RefreshClientPresentationProvider and never create authoritative runtime-node state.
     */
    bool RefreshResourceProvider(UMythicResourceISM &Provider);

    /**
     * Atomically indexes one client's cooked stable identities for replicated presentation only. This never creates
     * work, lifecycle, claim, reward, save, or authority-node state and is rejected outside NM_Client worlds.
     */
    bool RefreshClientPresentationProvider(UMythicResourceISM &Provider);

    /** Detaches one streamed-out authority provider lifetime while retaining authoritative durable node state. */
    void UnregisterResourceProvider(UMythicResourceISM &Provider);

    /** Detaches one streamed-out client presentation provider without altering replicated or authority state. */
    void UnregisterClientPresentationProvider(UMythicResourceISM &Provider);

    /**
     * Resolves a same-frame ISM hit to a stable node and current lifecycle generation. RuntimeInstanceId is valid only
     * for the returned immediate transaction and must never be stored as save or network identity.
     */
    bool ResolveHarvestTarget(UMythicResourceISM &Provider, int32 CurrentInstanceIndex,
                              FPrimitiveInstanceId &OutRuntimeInstanceId,
                              FMythicHarvestNodeId &OutNodeId,
                              uint32 &OutGeneration) const;

    /**
     * Issues opaque authority provenance for one already-committed attack activation. The token is scoped to the
     * exact ability instance, spec handle, source fragment, and physical item GUID until EndAttackCycle.
     */
    bool BeginAttackCycle(UMythicWeaponAttackAbility &Ability,
                          const UAttackFragment &AttackFragment,
                          FGameplayAbilitySpecHandle AbilitySpecHandle,
                          FMythicHarvestAttackCycleToken &OutToken);

    /** Invalidates one attack-cycle token and releases all cadence/dedup state owned by it. */
    void EndAttackCycle(const FMythicHarvestAttackCycleToken &Token,
                        const UMythicWeaponAttackAbility *ExpectedAbility);

    /** Executes one complete server-side validate/compute/commit/feedback harvest transaction. */
    FMythicHarvestResult TryApplyHarvest(const FMythicHarvestRequest &Request);

    /** Registers a runtime spatial cell so client deltas can be reconciled against streamed ISM providers. */
    void RegisterReplicationCell(AMythicHarvestReplicationCell &Cell);

    /** Discards presentation state sourced by an ending replication cell. */
    void UnregisterReplicationCell(AMythicHarvestReplicationCell &Cell);

    /** Returns authoritative counts for debugger/automation without exposing mutation access. */
    void GetNodeCounts(int32 &OutRegisteredProviders, int32 &OutAvailable,
                       int32 &OutUnavailable) const;

    /**
     * Gates new harvest transactions while one asynchronous world load is pending and invalidates active attack
     * cycles. Authority only; callers must pair the complete load transaction with CompleteSaveRestore or failure
     * with AbortSaveRestore.
     */
    bool BeginSaveRestore(FName &OutDiagnosticCode);

    /** Releases a previously acquired load gate without replacing state, used only when asynchronous load fails. */
    void AbortSaveRestore();

    /** Releases the load gate only after every world domain has finished applying the same validated snapshot. */
    bool CompleteSaveRestore(FName &OutDiagnosticCode);

    /**
     * Captures every unavailable node plus touched Available nodes using exact fixed-point work and contributor
     * inputs. Untouched Available nodes remain implicit; soft claims and controller references never persist.
     */
    bool BuildSaveSnapshot(FMythicHarvestWorldSaveV1 &OutSnapshot,
                           FName &OutDiagnosticCode) const;

    /**
     * Validates the world/outbox pair and reserves every spatial replication bucket before either live snapshot is
     * replaced. This is the fallible phase of the cross-domain restore transaction.
     */
    bool PreflightSaveRestore(
        const FMythicHarvestWorldSaveV1 &WorldSnapshot,
        const FMythicHarvestRewardOutboxSaveV1 &OutboxSnapshot,
        FName &OutDiagnosticCode);

    /**
     * Suspends new harvest transactions for a synchronous complete-world capture. Authority only; callers must pair
     * success with EndSaveCapture on every exit after node, reward-outbox, inventory, and world actors are copied.
     */
    bool BeginSaveCapture(FName &OutDiagnosticCode);

    /** Releases the synchronous save gate and restores transaction readiness after all world payloads are copied. */
    void EndSaveCapture();

    /**
     * Atomically replaces lifecycle state after full validation and stages rows whose providers are streamed out.
     * The caller retains the load gate until every other world domain is restored, then calls CompleteSaveRestore.
     */
    bool RestoreSaveSnapshot(const FMythicHarvestWorldSaveV1 &Snapshot,
                             FName &OutDiagnosticCode);

    /** Returns the authority-owned epoch that scopes deterministic completion/reward idempotency for this world. */
    const FGuid &GetWorldEpoch() const { return WorldEpoch; }

private:
    friend class AMythicGameState;
#if WITH_DEV_AUTOMATION_TESTS
    friend class FMythicHarvestClientPresentationRegistryTest;
    friend class FMythicHarvestExplicitAvailableReplicationTest;
    friend class FMythicHarvestPresentationStreamRestoreTest;
    friend class FMythicHarvestProviderTransientRecoveryTest;
#endif

    struct FRuntimeNode {
        FMythicHarvestNodeId NodeId;
        TWeakObjectPtr<UMythicResourceISM> Provider;
        TWeakObjectPtr<UMythicHarvestableDefinition> Definition;
        FPrimitiveInstanceId RuntimeInstanceId;
        FVector OriginalWorldLocation = FVector::ZeroVector;
        FIntPoint ReplicationCellCoordinate = FIntPoint::ZeroValue;
        bool bHasReplicationCellCoordinate = false;
        FMythicHarvestWork MaximumWork;
        FMythicHarvestWork RemainingWork;
        EMythicHarvestNodeState State = EMythicHarvestNodeState::Available;
        uint32 Generation = 1;
        uint32 Revision = 1;
        double RespawnServerDeadline = 0.0;
        FMythicHarvestClaimIdentity ClaimOwner;
        double ClaimExpiryServerTime = 0.0;
        TMap<FString, FMythicHarvestParticipantSnapshot> Contributors;
        bool bMutationInProgress = false;
    };

    /** Lazy-invalidated minimum-heap row for one unavailable node deadline. */
    struct FRespawnQueueEntry {
        FMythicHarvestNodeId NodeId;
        double RespawnServerDeadline = 0.0;
        uint32 Generation = 0;
        uint32 Revision = 0;

        bool operator<(const FRespawnQueueEntry &Other) const {
            return RespawnServerDeadline > Other.RespawnServerDeadline;
        }
    };

    struct FAttackCycleState {
        TWeakObjectPtr<UMythicWeaponAttackAbility> Ability;
        TWeakObjectPtr<const UAttackFragment> AttackFragment;
        TSet<FMythicHarvestNodeId> ConsumedNodes;
        double IssuedServerTime = 0.0;
        double ExpiresServerTime = 0.0;
    };

    /** Monotonic client presentation high-water retained only while at least one live cell source owns the node. */
    struct FClientNodeHighWater {
        FMythicHarvestPresentationStreamToken PresentationStream;
        FMythicHarvestReplicatedNodeItem Snapshot;
    };

    enum class EClientPresentationStreamDisposition : uint8 {
        Current,
        Future,
        Rejected,
    };

    /** Explicit load transaction phases prevent apply/complete replay under one asynchronous gate. */
    enum class ESaveRestorePhase : uint8 {
        Idle,
        Acquired,
        Preflighted,
        Applied,
    };

    /** Native rejection-coalescing identity; accepted/completed feedback never enters this throttle. */
    struct FFeedbackThrottleKey {
        TWeakObjectPtr<AMythicPlayerController> Controller;
        FMythicHarvestNodeId NodeId;
        TWeakObjectPtr<UMythicHarvestToolTypeDefinition> RequiredToolType;
        EMythicHarvestOutcome Outcome = EMythicHarvestOutcome::Rejected;
        EMythicHarvestRejectReason RejectReason =
            EMythicHarvestRejectReason::None;
        int32 RequiredToolTier = 0;

        bool operator==(const FFeedbackThrottleKey &Other) const {
            return Controller == Other.Controller && NodeId == Other.NodeId
                && RequiredToolType == Other.RequiredToolType
                && Outcome == Other.Outcome
                && RejectReason == Other.RejectReason
                && RequiredToolTier == Other.RequiredToolTier;
        }

        friend uint32 GetTypeHash(const FFeedbackThrottleKey &Key) {
            uint32 Hash = GetTypeHash(Key.Controller);
            Hash = HashCombineFast(Hash, GetTypeHash(Key.NodeId));
            Hash = HashCombineFast(Hash, GetTypeHash(Key.RequiredToolType));
            Hash = HashCombineFast(Hash, GetTypeHash(Key.Outcome));
            Hash = HashCombineFast(Hash, GetTypeHash(Key.RejectReason));
            return HashCombineFast(Hash, GetTypeHash(Key.RequiredToolTier));
        }
    };

    FMythicHarvestResult MakeRejectedResult(
        EMythicHarvestRejectReason Reason, const FRuntimeNode *Node,
        const FMythicHarvestRequest &Request);
    bool ValidateAttackProvenance(const FMythicHarvestRequest &Request,
                                  const FAttackCycleState &Cycle,
                                  AMythicPlayerController *&OutController,
                                  EMythicHarvestRejectReason &OutReason) const;
    /**
     * Answers whether this player has one gear-slotted tool of the node's required family. The tool is passive gear:
     * it is never wielded, never named by the client, and only this server scan may select the item that wears.
     */
    bool ResolveEquippedHarvestTool(
        AMythicPlayerController &Controller,
        const UMythicHarvestToolTypeDefinition *RequiredToolType,
        class UMythicItemInstance *&OutTool,
        const class UHarvestToolFragment *&OutHarvestTool,
        class UDurabilityFragment *&OutDurability,
        EMythicHarvestRejectReason &OutReason) const;
    bool ValidateRangeAndLineOfSight(const FMythicHarvestRequest &Request,
                                     const FRuntimeNode &Node,
                                     EMythicHarvestRejectReason &OutReason) const;
    void SendFeedback(AMythicPlayerController *Controller,
                      const FMythicHarvestRequest &Request,
                      const FRuntimeNode *Node,
                      const FMythicHarvestResult &Result);
    bool TryCaptureParticipantSnapshot(
        AMythicPlayerController &Controller,
        const UMythicHarvestableDefinition &Definition,
        int64 AppliedContributionQuanta,
        FMythicHarvestParticipantSnapshot &OutSnapshot) const;
    void CommitCompletionChannels(FRuntimeNode &Node) const;
    bool PublishNodeDelta(FRuntimeNode &Node);
    /** Restores one expired node and returns true when cell retirement made a providerless row safely implicit. */
    bool RestoreAvailableNode(
        FRuntimeNode &Node,
        TArray<FMythicHarvestNodeId> &OutAdditionalImplicitNodes);
    /** Retires an all-Available spatial cell and prunes its other providerless identity-only runtime rows. */
    bool RetireAuthorityCellIfOnlyAvailable(
        const FIntPoint &Coordinate,
        const FMythicHarvestNodeId &CurrentNodeId,
        TArray<FMythicHarvestNodeId> &OutAdditionalImplicitNodes);
    AMythicHarvestReplicationCell *FindOrCreateCell(const FIntPoint &Coordinate,
                                                    const FVector &NodeLocation);
    FIntPoint MakeCellCoordinate(const FVector &Location) const;
    bool ApplyClientNodeState(AMythicHarvestReplicationCell &Cell,
                              const FMythicHarvestReplicatedNodeItem &Item);
    void ReconcileClientNodePresentation(const FMythicHarvestNodeId &NodeId);
    void HandleClientCellBatch(AMythicHarvestReplicationCell &Cell);
    void ReconcileClientCellBatch(AMythicHarvestReplicationCell &Cell);
    void RegisterPresentationCoordinator(AMythicGameState &Coordinator);
    bool ActivateClientPresentationStream(
        const FMythicHarvestPresentationStreamToken &Token);
    EClientPresentationStreamDisposition ClassifyClientPresentationStream(
        const FMythicHarvestPresentationStreamToken &Token) const;
    void ResetClientPresentationStreamState();
    void ReplayClientPresentationCells();
    bool HasReadyAuthorityPresentationCoordinator() const;
    void RefreshAuthorityReadyState();
    bool CanResetAuthorityReplicationStream(
        const FMythicHarvestPresentationStreamToken &NextStream) const;
    bool ResetAuthorityReplicationStream(
        const FMythicHarvestPresentationStreamToken &NextStream);
    void DiscardPreflightCreatedReplicationCells();
    void TickRespawns(double ServerNowSeconds);
    void ScheduleRespawn(const FRuntimeNode &Node);
    void PruneAttackCycles(double ServerNowSeconds);
    void TickRewardOutbox(double ServerNowSeconds);
    bool HasActiveNodeMutation() const;
    FMythicHarvestClaimIdentity ResolveClaimIdentity(
        AMythicPlayerController &Controller) const;
    uint64 IssueServerSequence();
    bool IsRespawnVisibleToLivingPlayer(const FVector &NodeLocation) const;
    static uint16 QuantizeRemainingWork(FMythicHarvestWork Remaining,
                                        FMythicHarvestWork Maximum);

    TMap<FMythicHarvestNodeId, FRuntimeNode> Nodes;
    TMap<TWeakObjectPtr<UMythicResourceISM>, TSet<FMythicHarvestNodeId>> NodesByProvider;

    /** Client-only stable-id-to-component index used solely to apply replicated presentation deltas. */
    TMap<FMythicHarvestNodeId, TWeakObjectPtr<UMythicResourceISM>>
        ClientPresentationProviderByNode;

    /** Reverse client-only presentation index used for atomic refresh and World Partition detach. */
    TMap<TWeakObjectPtr<UMythicResourceISM>, TSet<FMythicHarvestNodeId>>
        ClientPresentationNodesByProvider;

    /** Active per-cell replicated snapshots; overlapping replacement cells never overwrite each other's lifetime. */
    TMap<FMythicHarvestNodeId,
         TMap<TWeakObjectPtr<AMythicHarvestReplicationCell>,
              FMythicHarvestReplicatedNodeItem>>
        ClientNodeSourcesByNode;

    /** Reverse ownership for deterministic cell teardown without treating relevance loss as authority removal. */
    TMap<TWeakObjectPtr<AMythicHarvestReplicationCell>,
         TSet<FMythicHarvestNodeId>>
        ClientNodeIdsByCell;

    /** Monotonic version/payload high-water blocks stale replay while one or more overlapping cell sources are live. */
    TMap<FMythicHarvestNodeId, FClientNodeHighWater> ClientNodeHighWater;

    /** Cells carrying a valid future stream wait for the always-relevant GameState activation barrier. */
    TSet<TWeakObjectPtr<AMythicHarvestReplicationCell>> ClientDeferredPresentationCells;

    /** Keeps the small direct definition closure resident so streaming cannot erase stable-id collision evidence. */
    UPROPERTY(Transient)
    TSet<TObjectPtr<UMythicHarvestableDefinition>> LoadedDefinitionClosure;

    TMap<FMythicHarvestAttackCycleToken, FAttackCycleState> AttackCycles;
    TArray<FRespawnQueueEntry> RespawnQueue;
    TMap<FIntPoint, TWeakObjectPtr<AMythicHarvestReplicationCell>> AuthorityCells;
    /** Empty cells created by preflight and recoverably removable until the validated snapshot is applied. */
    TSet<FIntPoint> PreflightCreatedAuthorityCellCoordinates;
    /** Canonical SHA-256 binding for the exact world payload admitted by the current preflight. */
    FSHA256Signature PreflightWorldSnapshotFingerprint{};
    /** Exact outbox incarnation that must be installed before the bound world payload may apply. */
    FSHA256Signature PreflightOutboxSnapshotFingerprint{};
    uint64 PreflightOutboxSnapshotSequence = 0;
    bool bHasPreflightSnapshotBinding = false;
    TSet<TWeakObjectPtr<AMythicHarvestReplicationCell>> ClientCells;
    /** Effective presentation snapshots selected from active per-cell sources at the monotonic high-water. */
    TMap<FMythicHarvestNodeId, FMythicHarvestReplicatedNodeItem> ClientNodeState;
    TMap<FFeedbackThrottleKey, double> LastRejectedFeedbackTime;
    uint64 NextAttackCycleSerial = 1;
    uint64 NextServerSequence = 1;
    double NextRewardOutboxRetryServerTime = 0.0;
    FMythicHarvestPresentationStreamToken AuthorityPresentationStream;
    FMythicHarvestPresentationStreamToken ClientPresentationStream;
    TWeakObjectPtr<AMythicGameState> PresentationCoordinator;
    FGuid WorldEpoch;
    bool bWorldHasBegunPlay = false;
    bool bWorldReady = false;
    ESaveRestorePhase SaveRestorePhase = ESaveRestorePhase::Idle;
    bool bSaveCaptureInProgress = false;
    bool bHarvestTransactionInProgress = false;
    bool bShuttingDown = false;
};
