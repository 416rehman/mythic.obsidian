#pragma once

#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Destructible.h"
#include "World/Harvesting/MythicHarvestTypes.h"

#include "MythicResourceISM.generated.h"

class UMythicHarvestableDefinition;
#if WITH_EDITOR
class UMythicHarvestIdentityValidationBuilder;
#endif

/** Selects the one serialized stable-identity contract consumed by a resource ISM provider. */
UENUM(BlueprintType)
enum class EMythicHarvestIdentitySource : uint8 {
    /** PCG/cook tooling writes the final opaque node GUID into each instance's reserved custom-data block. */
    PCGPacked UMETA(DisplayName = "PCG Packed Node Identity"),

    /** Editor baking writes a persistent per-instance GUID; runtime combines it with AuthoredNodeSetGuid. */
    EditorAuthored UMETA(DisplayName = "Editor-Authored Instance Identity"),
};

/** Immutable registration row built from one live ISM primitive id and its cooked stable identity payload. */
struct FMythicHarvestProviderNode {
    FMythicHarvestNodeId NodeId;
    FPrimitiveInstanceId PrimitiveInstanceId;
    FTransform OriginalWorldTransform;
};

/** One authoritative or replicated presentation target consumed by an atomic provider batch. */
struct FMythicHarvestNodePresentationUpdate {
    FMythicHarvestNodeId NodeId;
    bool bAvailable = true;
};

/**
 * Rendering and collision bridge for instanced harvestables.
 *
 * Gameplay balance lives exclusively in HarvestableDefinition. This component owns only the direct definition,
 * stable identity decoding, current-lifetime primitive-id caches, and native presentation application. It exposes no
 * Blueprint mutation path and never stores mutable instance indices as node identity.
 */
UCLASS(Blueprintable, ClassGroup = (Mythic),
       HideFunctions = (AddInstance, AddInstances, AddInstanceWorldSpace,
                        SetCustomDataValue, SetNumCustomDataFloats,
                        UpdateInstanceTransform, BatchUpdateInstancesTransforms,
                        BatchUpdateInstancesTransform, RemoveInstance,
                        RemoveInstances, ClearInstances, SetStaticMesh,
                        SetCollisionEnabled, SetCollisionProfileName,
                        SetCollisionObjectType, SetCollisionResponseToChannel,
                        SetCollisionResponseToAllChannels,
                        SetCollisionResponseToChannels, SetMobility),
       meta = (BlueprintSpawnableComponent))
class MYTHIC_API UMythicResourceISM : public UInstancedStaticMeshComponent,
                                     public IDestructible {
    GENERATED_BODY()

public:
    UMythicResourceISM();

    /**
     * Chooses whether reserved per-instance custom data contains a final PCG node id or an editor-baked instance
     * GUID. Blueprint may inspect this authoring contract but cannot switch it at runtime; invalid values fail closed.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvesting|Identity")
    EMythicHarvestIdentitySource IdentitySource =
        EMythicHarvestIdentitySource::PCGPacked;

    /**
     * Persistent namespace for one manually placed ISM component. The editor bake command creates it only when
     * missing; runtime never generates or repairs it, and changing it intentionally changes every derived node id.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvesting|Identity",
              meta = (EditCondition =
                          "IdentitySource == EMythicHarvestIdentitySource::EditorAuthored",
                      EditConditionHides))
    FGuid AuthoredNodeSetGuid;

    /**
     * First reserved custom-data float for the eight-value identity payload. Blueprint may inspect the authored
     * layout but cannot mutate it at runtime; negative or overlapping content layouts fail validation.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvesting|Identity",
              meta = (ClampMin = "0", UIMin = "0"))
    int32 IdentityCustomDataStartIndex =
        MythicHarvestPCGIdentity::MaterialReservedLeadingFloats;

    /**
     * Sole definition for work, tool eligibility, progression, rewards, lifecycle, and feedback on every instance in
     * this component. Blueprint may inspect but cannot replace it at runtime; null fails registration and validation.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Harvesting")
    TObjectPtr<UMythicHarvestableDefinition> HarvestableDefinition = nullptr;

    /**
     * Transactionally bakes only missing/invalid manual per-instance GUIDs while preserving all valid identity and
     * material custom data. Valid duplicates reject the complete operation; runtime builds never generate identity.
     */
    UFUNCTION(CallInEditor, Category = "Harvesting|Identity")
    void BakeMissingAuthoredIdentities();

#if WITH_EDITOR
    /** Native success/failure form used by validation automation and the CallInEditor wrapper. */
    bool TryBakeMissingAuthoredIdentities();
#endif

    virtual FRewardsToGive GetOnKillRewards(AActor *Killer = nullptr) override;
    virtual void OnRegister() override;
    virtual void OnUnregister() override;
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

    virtual bool SetStaticMesh(UStaticMesh *NewMesh) override;
    virtual void SetCollisionEnabled(ECollisionEnabled::Type NewType) override;
    virtual void SetCollisionProfileName(FName InCollisionProfileName,
                                         bool bUpdateOverlaps = true) override;
    virtual void SetCollisionObjectType(ECollisionChannel Channel) override;
    virtual void SetCollisionResponseToChannel(
        ECollisionChannel Channel, ECollisionResponse NewResponse) override;
    virtual void SetCollisionResponseToAllChannels(
        ECollisionResponse NewResponse) override;
    virtual void SetCollisionResponseToChannels(
        const FCollisionResponseContainer &NewResponses) override;
    virtual void SetMobility(EComponentMobility::Type NewMobility) override;

    virtual int32 AddInstance(const FTransform &InstanceTransform,
                              bool bWorldSpace = false) override;
    virtual TArray<int32> AddInstances(const TArray<FTransform> &InstanceTransforms,
                                       bool bShouldReturnIndices,
                                       bool bWorldSpace = false,
                                       bool bUpdateNavigation = true) override;
    virtual bool SetCustomData(int32 InstanceIndex,
                               TArrayView<const float> CustomDataFloats,
                               bool bMarkRenderStateDirty = false) override;
    virtual bool SetCustomData(int32 InstanceIndexStart, int32 InstanceIndexEnd,
                               TConstArrayView<float> CustomDataFloats,
                               bool bMarkRenderStateDirty = false) override;
    virtual bool SetCustomDataValue(int32 InstanceIndex, int32 CustomDataIndex,
                                    float CustomDataValue,
                                    bool bMarkRenderStateDirty = false) override;
    virtual void SetNumCustomDataFloats(int32 InNumCustomDataFloats) override;
    virtual bool UpdateInstanceTransform(
        int32 InstanceIndex, const FTransform &NewInstanceTransform,
        bool bWorldSpace = false, bool bMarkRenderStateDirty = false,
        bool bTeleport = false) override;
    virtual bool BatchUpdateInstancesTransforms(
        int32 StartInstanceIndex,
        const TArray<FTransform> &NewInstancesTransforms,
        bool bWorldSpace = false, bool bMarkRenderStateDirty = false,
        bool bTeleport = false) override;
    virtual bool BatchUpdateInstancesTransforms(
        int32 StartInstanceIndex,
        TArrayView<const FTransform> NewInstancesTransforms,
        bool bWorldSpace = false, bool bMarkRenderStateDirty = false,
        bool bTeleport = false) override;
    virtual bool BatchUpdateInstancesTransforms(
        int32 StartInstanceIndex,
        const TArray<FTransform> &NewInstancesTransforms,
        const TArray<FTransform> &NewInstancesPrevTransforms,
        bool bWorldSpace = false, bool bMarkRenderStateDirty = false,
        bool bTeleport = false) override;
    virtual bool BatchUpdateInstancesTransform(
        int32 StartInstanceIndex, int32 NumInstances,
        const FTransform &NewInstancesTransform, bool bWorldSpace = false,
        bool bMarkRenderStateDirty = false, bool bTeleport = false) override;
    virtual bool BatchUpdateInstancesData(
        int32 StartInstanceIndex, int32 NumInstances,
        FInstancedStaticMeshInstanceData *StartInstanceData,
        bool bMarkRenderStateDirty = false, bool bTeleport = false) override;
    virtual void RemoveInstancesById(
        const TArrayView<const FPrimitiveInstanceId> &InstanceIds,
        bool bUpdateNavigation = true) override;
    virtual bool RemoveInstance(int32 InstanceIndex) override;
    virtual bool RemoveInstances(const TArray<int32> &InstancesToRemove) override;
    virtual bool RemoveInstances(
        const TArray<int32> &InstancesToRemove,
        bool bInstanceArrayAlreadySortedInReverseOrder) override;
    virtual void ClearInstances() override;

    /**
     * Rebuilds the complete current identity batch, then registers authority state on servers or presentation-only
     * lookup state on clients. Partial, malformed, or cross-provider duplicate batches fail closed atomically.
     */
    bool RefreshHarvestIdentityRegistration();

    /**
     * Verifies the complete runtime query contract used by weapon object sweeps and harvesting line-of-sight traces.
     * While quarantined, this validates the preserved authored collision mode rather than the forced fail-closed mode.
     */
    bool HasValidHarvestCollisionContract() const;

    /**
     * Removes this component lifetime from its role-specific world index; authority durable rows and cached client
     * replication deltas both outlive a World Partition presentation provider.
     */
    void UnregisterHarvestIdentityProvider();

    /** Resolves a current primitive id to its opaque stable identity without converting it back into durable index. */
    bool ResolveStableNodeId(FPrimitiveInstanceId PrimitiveInstanceId,
                             FMythicHarvestNodeId &OutNodeId) const;

    /**
     * Converts the transient instance index carried by one same-frame physics hit into its current primitive id and
     * cooked stable node identity. Callers must discard both outputs after the transaction; indices never persist.
     */
    bool ResolveAuthoritativeHitInstance(int32 CurrentInstanceIndex,
                                         FPrimitiveInstanceId &OutPrimitiveInstanceId,
                                         FMythicHarvestNodeId &OutNodeId) const;

    /** Resolves a stable identity to the primitive id valid only for this registered component lifetime. */
    bool ResolvePrimitiveInstanceId(const FMythicHarvestNodeId &NodeId,
                                    FPrimitiveInstanceId &OutPrimitiveInstanceId) const;

    /** Copies the complete validated batch for authority registration or client presentation indexing. */
    void GetHarvestProviderNodes(TArray<FMythicHarvestProviderNode> &OutNodes) const;

    /** Applies cooked available or hidden/depleted presentation by stable id, including individual physics movement. */
    bool ApplyNodeAvailability(const FMythicHarvestNodeId &NodeId,
                               bool bAvailable);

    /**
     * Prevalidates every stable-id/primitive/transform mapping before applying only changed transforms in contiguous
     * native batches. Any invalid row quarantines the whole provider; no partial batch is admitted for registration.
     */
    bool ApplyNodeAvailabilityBatch(
        TConstArrayView<FMythicHarvestNodePresentationUpdate> Updates);

    /** Returns true after this component has entered a game world and runtime instance mutation has been sealed. */
    bool IsRuntimeMutationSealed() const;

#if WITH_DEV_AUTOMATION_TESTS
    /** Injects retryable primitive-map and transform-read failures into subsequent registration attempts. */
    void SetIdentityRefreshRuntimeFailureInjectionForTests(
        int32 PrimitiveMapFailures, int32 TransformReadFailures);

    /** Returns whether quarantine currently forces this provider to NoCollision. */
    bool IsHarvestQueryCollisionSuppressedForTests() const {
        return bHarvestQueryCollisionSuppressed;
    }

    /** Returns the number of instance transforms submitted by availability APIs, excluding verified no-ops. */
    int32 GetAvailabilityTransformWriteCountForTests() const {
        return AvailabilityTransformWriteCountForTests;
    }

    /** Returns native contiguous-batch submissions, allowing scale tests to distinguish batching from per-row calls. */
    int32 GetAvailabilityNativeBatchCallCountForTests() const {
        return AvailabilityNativeBatchCallCountForTests;
    }
#endif

#if WITH_EDITOR
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
    virtual void PostEditImport() override;
#endif

private:
#if WITH_EDITOR
    friend class UMythicHarvestIdentityValidationBuilder;
#endif

    bool DecodeStableNodeIdAtIndex(int32 CurrentInstanceIndex,
                                   FMythicHarvestNodeId &OutNodeId) const;
    bool RejectSealedRuntimeMutation(const TCHAR *MutationName) const;
    ECollisionEnabled::Type GetHarvestCollisionModeForValidation() const;
    void SuppressHarvestQueryCollision();
    bool RestoreHarvestQueryCollision();
    void QuarantineHarvestIdentityRegistration();
    void RequestDeferredIdentityRefresh();
    void QueueProviderQuarantineAndRetry(const TCHAR *FailureReason);
    void ScheduleIdentityRefreshRetry();

    // These preliminary non-virtual ID APIs can otherwise bypass every guarded
    // virtual. Keep them unavailable through UMythicResourceISM-typed native
    // code; runtime behavior is still protected for every reflected path.
    using UInstancedStaticMeshComponent::AddInstanceById;
    using UInstancedStaticMeshComponent::AddInstancesById;
    using UInstancedStaticMeshComponent::SetCustomDataById;
    using UInstancedStaticMeshComponent::SetCustomDataValueById;
    using UInstancedStaticMeshComponent::SetPreviousTransformById;
    using UInstancedStaticMeshComponent::UpdateInstanceTransformById;

    TMap<int32, FMythicHarvestNodeId> StableNodeByPrimitiveValue;
    TMap<FMythicHarvestNodeId, FPrimitiveInstanceId> PrimitiveByStableNode;
    TMap<FMythicHarvestNodeId, FTransform> OriginalWorldTransformByNode;
    TSet<FMythicHarvestNodeId> HiddenNodes;
    FTimerHandle IdentityRefreshRetryTimer;
    int32 NativeMutationDepth = 0;
    int32 IdentityRefreshRetryAttempt = 0;
    ECollisionEnabled::Type AuthoredHarvestCollisionEnabled =
        ECollisionEnabled::NoCollision;
    bool bLastIdentityRefreshFailureRetryable = false;
    bool bIdentityRefreshInRecoveryMode = false;
    bool bRuntimeMutationSealEngaged = false;
    bool bIdentityRefreshInProgress = false;
    bool bDeferredIdentityRefreshRequested = false;
    bool bProviderQuarantinePending = false;
    bool bHasAuthoredHarvestCollisionEnabled = false;
    bool bHarvestQueryCollisionSuppressed = false;
#if WITH_DEV_AUTOMATION_TESTS
    int32 TestPrimitiveMapFailureCount = 0;
    int32 TestTransformReadFailureCount = 0;
    int32 AvailabilityTransformWriteCountForTests = 0;
    int32 AvailabilityNativeBatchCallCountForTests = 0;
#endif
};
