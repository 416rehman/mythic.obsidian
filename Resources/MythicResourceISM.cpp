#include "Resources/MythicResourceISM.h"

#include "Misc/DataValidation.h"
#include "Mythic.h"
#include "TimerManager.h"
#include "Engine/World.h"
#include "World/Harvesting/MythicHarvestAuthoredIdentity.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"
#include "World/Harvesting/MythicHarvestSettings.h"
#include "World/Harvesting/MythicHarvestWorldSubsystem.h"
#include "World/Harvesting/MythicHarvestableDefinition.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicResourceISM)

namespace {
const FName HarvestCollisionProfileName(TEXT("Destructible"));
constexpr int32 MaxIdentityRefreshRetryAttempts = 8;
constexpr float InitialIdentityRefreshRetrySeconds = 0.1F;
constexpr float MaxIdentityRefreshRetrySeconds = 5.0F;
constexpr float RecoveryIdentityRefreshRetrySeconds = 30.0F;
}

UMythicResourceISM::UMythicResourceISM() {
    SetCollisionProfileName(HarvestCollisionProfileName);
}

void UMythicResourceISM::BakeMissingAuthoredIdentities() {
#if WITH_EDITOR
    TryBakeMissingAuthoredIdentities();
#else
    UE_LOG(Myth, Error,
           TEXT("Authored harvesting identity baking is editor-only; '%s' was not modified."),
           *GetPathName());
#endif
}

#if WITH_EDITOR
bool UMythicResourceISM::TryBakeMissingAuthoredIdentities() {
    constexpr int32 PackedCount = MythicHarvestPCGIdentity::PackedFloatCount;
    if (const UWorld *World = GetWorld(); World && World->IsGameWorld()) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity bake rejected '%s': identity generation is forbidden in PIE and runtime worlds."),
               *GetPathName());
        return false;
    }
    if (IdentitySource != EMythicHarvestIdentitySource::EditorAuthored) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity bake rejected '%s': Identity Source must be Editor Authored."),
               *GetPathName());
        return false;
    }
    if (IdentityCustomDataStartIndex < 0
        || IdentityCustomDataStartIndex > MAX_int32 - PackedCount) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity bake rejected '%s': identity custom-data start is invalid."),
               *GetPathName());
        return false;
    }

    const int32 InstanceCount = GetInstanceCount();
    const int32 PreviousStride = FMath::Max(0, NumCustomDataFloats);
    const int32 RequiredStride = IdentityCustomDataStartIndex + PackedCount;
    const int32 NewStride = FMath::Max(PreviousStride, RequiredStride);
    const int64 RequiredValueCount =
        static_cast<int64>(InstanceCount) * static_cast<int64>(NewStride);
    if (RequiredValueCount < 0 || RequiredValueCount > MAX_int32) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity bake rejected '%s': custom-data allocation is out of range."),
               *GetPathName());
        return false;
    }

    const FGuid ProspectiveNodeSetGuid = AuthoredNodeSetGuid.IsValid()
        ? AuthoredNodeSetGuid : FGuid::NewGuid();
    if (!ProspectiveNodeSetGuid.IsValid()) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity bake rejected '%s': failed to create a valid node-set GUID."),
               *GetPathName());
        return false;
    }

    TArray<FGuid> InstanceGuids;
    InstanceGuids.SetNum(InstanceCount);
    TMap<FGuid, int32> FirstValidIndexByGuid;
    FirstValidIndexByGuid.Reserve(InstanceCount);
    TBitArray<> NeedsIdentity(false, InstanceCount);
    bool bGeneratedAnyIdentity = false;

    for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount;
         ++InstanceIndex) {
        const int64 PackedStart =
            static_cast<int64>(InstanceIndex)
                * static_cast<int64>(PreviousStride)
            + IdentityCustomDataStartIndex;
        const bool bPayloadAddressable = PreviousStride >= RequiredStride
            && PackedStart >= 0
            && PackedStart + PackedCount <= PerInstanceSMCustomData.Num();
        FGuid ExistingGuid;
        const bool bExistingValid = bPayloadAddressable
            && MythicHarvestAuthoredIdentity::TryDecodePackedInstanceGuid(
                MakeArrayView(PerInstanceSMCustomData.GetData()
                                  + static_cast<int32>(PackedStart),
                              PackedCount),
                ExistingGuid);
        if (!bExistingValid) {
            NeedsIdentity[InstanceIndex] = true;
            continue;
        }
        if (const int32 *FirstIndex =
                FirstValidIndexByGuid.Find(ExistingGuid)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest identity bake rejected complete provider '%s': valid authored GUID is duplicated at instances %d and %d; duplicates are never silently repaired."),
                   *GetPathName(), *FirstIndex, InstanceIndex);
            return false;
        }
        FirstValidIndexByGuid.Add(ExistingGuid, InstanceIndex);
        InstanceGuids[InstanceIndex] = ExistingGuid;
    }

    for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount;
         ++InstanceIndex) {
        if (!NeedsIdentity[InstanceIndex]) {
            continue;
        }
        FGuid NewGuid;
        do {
            NewGuid = FGuid::NewGuid();
        } while (!NewGuid.IsValid() || FirstValidIndexByGuid.Contains(NewGuid));
        FirstValidIndexByGuid.Add(NewGuid, InstanceIndex);
        InstanceGuids[InstanceIndex] = NewGuid;
        bGeneratedAnyIdentity = true;
    }

    TArray<FMythicHarvestNodeId> DerivedNodeIds;
    const FMythicHarvestAuthoredIdentityValidationResult Validation =
        MythicHarvestAuthoredIdentity::ValidateAndBuildNodeIds(
            ProspectiveNodeSetGuid, InstanceGuids, DerivedNodeIds);
    if (!Validation.IsValid()
        || DerivedNodeIds.Num() != InstanceCount) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity bake rejected complete provider '%s': canonical authored batch validation failed at instance %d."),
               *GetPathName(), Validation.FailureIndex);
        return false;
    }

    TArray<float> NewCustomData;
    NewCustomData.SetNumZeroed(static_cast<int32>(RequiredValueCount));
    for (int32 InstanceIndex = 0; InstanceIndex < InstanceCount;
         ++InstanceIndex) {
        const int64 PreviousRowStart =
            static_cast<int64>(InstanceIndex) * PreviousStride;
        const int64 NewRowStart = static_cast<int64>(InstanceIndex) * NewStride;
        for (int32 CustomDataIndex = 0;
             CustomDataIndex < PreviousStride; ++CustomDataIndex) {
            const int64 PreviousIndex = PreviousRowStart + CustomDataIndex;
            if (PreviousIndex >= 0
                && PreviousIndex < PerInstanceSMCustomData.Num()) {
                NewCustomData[static_cast<int32>(NewRowStart
                                                 + CustomDataIndex)] =
                    PerInstanceSMCustomData[static_cast<int32>(PreviousIndex)];
            }
        }

        TArray<float> PackedGuid;
        if (!MythicHarvestAuthoredIdentity::AppendPackedInstanceGuid(
                InstanceGuids[InstanceIndex], PackedGuid)
            || PackedGuid.Num() != PackedCount) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest identity bake rejected complete provider '%s': failed to encode instance %d."),
                   *GetPathName(), InstanceIndex);
            return false;
        }
        for (int32 PackedIndex = 0; PackedIndex < PackedCount; ++PackedIndex) {
            NewCustomData[static_cast<int32>(NewRowStart
                                             + IdentityCustomDataStartIndex
                                             + PackedIndex)] =
                PackedGuid[PackedIndex];
        }
    }

    const bool bNeedsMutation = !AuthoredNodeSetGuid.IsValid()
        || NewStride != NumCustomDataFloats
        || bGeneratedAnyIdentity
        || NewCustomData != PerInstanceSMCustomData;
    if (!bNeedsMutation) {
        return true;
    }

    Modify();
    {
        TGuardValue<bool> RefreshGuard(bIdentityRefreshInProgress, true);
        AuthoredNodeSetGuid = ProspectiveNodeSetGuid;
        if (NumCustomDataFloats != NewStride) {
            // This engine API intentionally zeroes the whole buffer, so the complete
            // snapshot above must be restored after expanding the row stride.
            Super::SetNumCustomDataFloats(NewStride);
        }
        if (InstanceCount > 0) {
            const bool bCopiedCompleteBuffer = Super::SetCustomData(
                0, InstanceCount - 1, NewCustomData, true);
            check(bCopiedCompleteBuffer);
        }
        else {
            MarkRenderStateDirty();
        }
        MarkPackageDirty();
        PostEditChange();
    }

    UE_LOG(Myth, Display,
           TEXT("Baked persistent authored harvesting identity for '%s' (%d instances, custom-data offset %d)."),
           *GetPathName(), InstanceCount, IdentityCustomDataStartIndex);
    RequestDeferredIdentityRefresh();
    return true;
}
#endif

FRewardsToGive UMythicResourceISM::GetOnKillRewards(AActor * /*Killer*/) {
    // Resource rewards are planned and committed by the harvesting transaction. Returning an empty generic reward
    // prevents IDestructible compatibility from becoming a second, repeatable reward source.
    return FRewardsToGive();
}

void UMythicResourceISM::OnRegister() {
    Super::OnRegister();
    if (const UWorld *World = GetWorld(); World && World->IsGameWorld()) {
        // This is intentionally irreversible for the lifetime of the object.
        // Unregister/register cycles are normal under World Partition and must
        // never reopen a mutation window on cooked authority content.
        bRuntimeMutationSealEngaged = true;
        // A component is not a valid gameplay query target until its complete
        // identity batch and current replicated availability are reconciled.
        SuppressHarvestQueryCollision();
    }
    RequestDeferredIdentityRefresh();
}

void UMythicResourceISM::OnUnregister() {
    // Component registration can cycle without EndPlay at World Partition and
    // render-state boundaries. Detach the transient registry lifetime now, but
    // retain original transforms so a hidden node can restore correctly when
    // this same component registers again.
    QuarantineHarvestIdentityRegistration();
    Super::OnUnregister();
}

void UMythicResourceISM::BeginPlay() {
    Super::BeginPlay();
    RequestDeferredIdentityRefresh();
}

void UMythicResourceISM::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(IdentityRefreshRetryTimer);
    }
    bProviderQuarantinePending = false;
    bDeferredIdentityRefreshRequested = false;
    bLastIdentityRefreshFailureRetryable = false;
    bIdentityRefreshInRecoveryMode = false;
    UnregisterHarvestIdentityProvider();
    Super::EndPlay(EndPlayReason);
}

bool UMythicResourceISM::IsRuntimeMutationSealed() const {
    return bRuntimeMutationSealEngaged && NativeMutationDepth <= 0;
}

#if WITH_DEV_AUTOMATION_TESTS
void UMythicResourceISM::SetIdentityRefreshRuntimeFailureInjectionForTests(
    const int32 PrimitiveMapFailures, const int32 TransformReadFailures) {
    TestPrimitiveMapFailureCount = FMath::Max(0, PrimitiveMapFailures);
    TestTransformReadFailureCount = FMath::Max(0, TransformReadFailures);
}
#endif

bool UMythicResourceISM::RejectSealedRuntimeMutation(
    const TCHAR *MutationName) const {
    if (!IsRuntimeMutationSealed()) {
        return false;
    }
    UE_LOG(Myth, Error,
           TEXT("Rejected runtime mutation '%s' on authoritative harvest provider '%s'. Instances, identity, mesh, and collision are immutable after game-world registration."),
           MutationName, *GetPathName());
    return true;
}

bool UMythicResourceISM::SetStaticMesh(UStaticMesh *NewMesh) {
    if (NewMesh == GetStaticMesh()) {
        return true;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetStaticMesh"))) {
        return false;
    }
    return Super::SetStaticMesh(NewMesh);
}

void UMythicResourceISM::SetCollisionEnabled(
    const ECollisionEnabled::Type NewType) {
    if (bHarvestQueryCollisionSuppressed) {
        if (RejectSealedRuntimeMutation(TEXT("SetCollisionEnabled"))) {
            return;
        }
        // Construction code may finish authoring after an identity mutation
        // has already quarantined an unregistered game-world component. Record
        // that intent without reopening its query surface.
        AuthoredHarvestCollisionEnabled = NewType;
        bHasAuthoredHarvestCollisionEnabled = true;
        return;
    }
    if (NewType == GetCollisionEnabled()) {
        return;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetCollisionEnabled"))) {
        return;
    }
    Super::SetCollisionEnabled(NewType);
}

void UMythicResourceISM::SetCollisionProfileName(
    const FName InCollisionProfileName, const bool bUpdateOverlaps) {
    if (InCollisionProfileName == GetCollisionProfileName()) {
        return;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetCollisionProfileName"))) {
        return;
    }
    Super::SetCollisionProfileName(InCollisionProfileName, bUpdateOverlaps);
    if (bHarvestQueryCollisionSuppressed) {
        AuthoredHarvestCollisionEnabled = GetCollisionEnabled();
        bHasAuthoredHarvestCollisionEnabled = true;
        TGuardValue<int32> NativeMutationGuard(NativeMutationDepth,
                                               NativeMutationDepth + 1);
        Super::SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

void UMythicResourceISM::SetCollisionObjectType(
    const ECollisionChannel Channel) {
    if (Channel == GetCollisionObjectType()) {
        return;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetCollisionObjectType"))) {
        return;
    }
    Super::SetCollisionObjectType(Channel);
}

void UMythicResourceISM::SetCollisionResponseToChannel(
    const ECollisionChannel Channel, const ECollisionResponse NewResponse) {
    if (GetCollisionResponseToChannel(Channel) == NewResponse) {
        return;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetCollisionResponseToChannel"))) {
        return;
    }
    Super::SetCollisionResponseToChannel(Channel, NewResponse);
}

void UMythicResourceISM::SetCollisionResponseToAllChannels(
    const ECollisionResponse NewResponse) {
    if (RejectSealedRuntimeMutation(
            TEXT("SetCollisionResponseToAllChannels"))) {
        return;
    }
    Super::SetCollisionResponseToAllChannels(NewResponse);
}

void UMythicResourceISM::SetCollisionResponseToChannels(
    const FCollisionResponseContainer &NewResponses) {
    if (GetCollisionResponseToChannels() == NewResponses) {
        return;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetCollisionResponseToChannels"))) {
        return;
    }
    Super::SetCollisionResponseToChannels(NewResponses);
}

void UMythicResourceISM::SetMobility(
    const EComponentMobility::Type NewMobility) {
    if (NewMobility == Mobility) {
        return;
    }
    if (RejectSealedRuntimeMutation(TEXT("SetMobility"))) {
        return;
    }
    Super::SetMobility(NewMobility);
}

int32 UMythicResourceISM::AddInstance(const FTransform &InstanceTransform,
                                      const bool bWorldSpace) {
    if (RejectSealedRuntimeMutation(TEXT("AddInstance"))) {
        return INDEX_NONE;
    }
    QuarantineHarvestIdentityRegistration();
    const int32 Result = Super::AddInstance(InstanceTransform, bWorldSpace);
    RequestDeferredIdentityRefresh();
    return Result;
}

TArray<int32> UMythicResourceISM::AddInstances(
    const TArray<FTransform> &InstanceTransforms,
    const bool bShouldReturnIndices,
    const bool bWorldSpace,
    const bool bUpdateNavigation) {
    if (RejectSealedRuntimeMutation(TEXT("AddInstances"))) {
        return TArray<int32>();
    }
    QuarantineHarvestIdentityRegistration();
    TArray<int32> Result = Super::AddInstances(InstanceTransforms,
                                               bShouldReturnIndices,
                                               bWorldSpace,
                                               bUpdateNavigation);
    RequestDeferredIdentityRefresh();
    return Result;
}

bool UMythicResourceISM::SetCustomData(
    const int32 InstanceIndex,
    TArrayView<const float> CustomDataFloats,
    const bool bMarkRenderStateDirty) {
    if (RejectSealedRuntimeMutation(TEXT("SetCustomData"))) {
        return false;
    }
    const bool bTouchesIdentity = IdentityCustomDataStartIndex >= 0
        && CustomDataFloats.Num() > IdentityCustomDataStartIndex;
    if (bTouchesIdentity) {
        QuarantineHarvestIdentityRegistration();
    }
    const bool bResult = Super::SetCustomData(InstanceIndex, CustomDataFloats,
                                              bMarkRenderStateDirty);
    if (bTouchesIdentity) {
        RequestDeferredIdentityRefresh();
    }
    return bResult;
}

bool UMythicResourceISM::SetCustomDataValue(
    const int32 InstanceIndex, const int32 CustomDataIndex,
    const float CustomDataValue, const bool bMarkRenderStateDirty) {
    if (RejectSealedRuntimeMutation(TEXT("SetCustomDataValue"))) {
        return false;
    }
    const bool bTouchesIdentity = IdentityCustomDataStartIndex >= 0
        && IdentityCustomDataStartIndex
            <= MAX_int32 - MythicHarvestPCGIdentity::PackedFloatCount
        && CustomDataIndex >= IdentityCustomDataStartIndex
        && CustomDataIndex < IdentityCustomDataStartIndex
                + MythicHarvestPCGIdentity::PackedFloatCount;
    if (bTouchesIdentity) {
        QuarantineHarvestIdentityRegistration();
    }
    const bool bResult = Super::SetCustomDataValue(
        InstanceIndex, CustomDataIndex, CustomDataValue,
        bMarkRenderStateDirty);
    if (bTouchesIdentity) {
        RequestDeferredIdentityRefresh();
    }
    return bResult;
}

void UMythicResourceISM::SetNumCustomDataFloats(
    const int32 InNumCustomDataFloats) {
    if (RejectSealedRuntimeMutation(TEXT("SetNumCustomDataFloats"))) {
        return;
    }
    const bool bChangesLayout =
        FMath::Max(InNumCustomDataFloats, 0) != NumCustomDataFloats;
    if (bChangesLayout) {
        QuarantineHarvestIdentityRegistration();
    }
    Super::SetNumCustomDataFloats(InNumCustomDataFloats);
    if (bChangesLayout) {
        RequestDeferredIdentityRefresh();
    }
}

bool UMythicResourceISM::UpdateInstanceTransform(
    const int32 InstanceIndex, const FTransform &NewInstanceTransform,
    const bool bWorldSpace, const bool bMarkRenderStateDirty,
    const bool bTeleport) {
    if (RejectSealedRuntimeMutation(TEXT("UpdateInstanceTransform"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::UpdateInstanceTransform(
        InstanceIndex, NewInstanceTransform, bWorldSpace,
        bMarkRenderStateDirty, bTeleport);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::BatchUpdateInstancesTransforms(
    const int32 StartInstanceIndex,
    const TArray<FTransform> &NewInstancesTransforms,
    const bool bWorldSpace, const bool bMarkRenderStateDirty,
    const bool bTeleport) {
    if (RejectSealedRuntimeMutation(
            TEXT("BatchUpdateInstancesTransforms"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::BatchUpdateInstancesTransforms(
        StartInstanceIndex, NewInstancesTransforms, bWorldSpace,
        bMarkRenderStateDirty, bTeleport);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::BatchUpdateInstancesTransforms(
    const int32 StartInstanceIndex,
    TArrayView<const FTransform> NewInstancesTransforms,
    const bool bWorldSpace, const bool bMarkRenderStateDirty,
    const bool bTeleport) {
    if (RejectSealedRuntimeMutation(
            TEXT("BatchUpdateInstancesTransformsView"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::BatchUpdateInstancesTransforms(
        StartInstanceIndex, NewInstancesTransforms, bWorldSpace,
        bMarkRenderStateDirty, bTeleport);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::BatchUpdateInstancesTransforms(
    const int32 StartInstanceIndex,
    const TArray<FTransform> &NewInstancesTransforms,
    const TArray<FTransform> &NewInstancesPrevTransforms,
    const bool bWorldSpace, const bool bMarkRenderStateDirty,
    const bool bTeleport) {
    if (RejectSealedRuntimeMutation(
            TEXT("BatchUpdateInstancesTransformsWithPrevious"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::BatchUpdateInstancesTransforms(
        StartInstanceIndex, NewInstancesTransforms,
        NewInstancesPrevTransforms, bWorldSpace, bMarkRenderStateDirty,
        bTeleport);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::BatchUpdateInstancesTransform(
    const int32 StartInstanceIndex, const int32 NumInstances,
    const FTransform &NewInstancesTransform, const bool bWorldSpace,
    const bool bMarkRenderStateDirty, const bool bTeleport) {
    if (RejectSealedRuntimeMutation(
            TEXT("BatchUpdateInstancesTransform"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::BatchUpdateInstancesTransform(
        StartInstanceIndex, NumInstances, NewInstancesTransform,
        bWorldSpace, bMarkRenderStateDirty, bTeleport);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::BatchUpdateInstancesData(
    const int32 StartInstanceIndex, const int32 NumInstances,
    FInstancedStaticMeshInstanceData *StartInstanceData,
    const bool bMarkRenderStateDirty, const bool bTeleport) {
    if (RejectSealedRuntimeMutation(TEXT("BatchUpdateInstancesData"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::BatchUpdateInstancesData(
        StartInstanceIndex, NumInstances, StartInstanceData,
        bMarkRenderStateDirty, bTeleport);
    RequestDeferredIdentityRefresh();
    return bResult;
}

void UMythicResourceISM::RemoveInstancesById(
    const TArrayView<const FPrimitiveInstanceId> &InstanceIds,
    const bool bUpdateNavigation) {
    if (RejectSealedRuntimeMutation(TEXT("RemoveInstancesById"))) {
        return;
    }
    QuarantineHarvestIdentityRegistration();
    Super::RemoveInstancesById(InstanceIds, bUpdateNavigation);
    RequestDeferredIdentityRefresh();
}

bool UMythicResourceISM::RemoveInstance(const int32 InstanceIndex) {
    if (RejectSealedRuntimeMutation(TEXT("RemoveInstance"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::RemoveInstance(InstanceIndex);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::RemoveInstances(
    const TArray<int32> &InstancesToRemove) {
    if (RejectSealedRuntimeMutation(TEXT("RemoveInstances"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::RemoveInstances(InstancesToRemove);
    RequestDeferredIdentityRefresh();
    return bResult;
}

bool UMythicResourceISM::RemoveInstances(
    const TArray<int32> &InstancesToRemove,
    const bool bInstanceArrayAlreadySortedInReverseOrder) {
    if (RejectSealedRuntimeMutation(TEXT("RemoveInstancesSorted"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::RemoveInstances(
        InstancesToRemove, bInstanceArrayAlreadySortedInReverseOrder);
    RequestDeferredIdentityRefresh();
    return bResult;
}

void UMythicResourceISM::ClearInstances() {
    if (RejectSealedRuntimeMutation(TEXT("ClearInstances"))) {
        return;
    }
    QuarantineHarvestIdentityRegistration();
    Super::ClearInstances();
    RequestDeferredIdentityRefresh();
}

bool UMythicResourceISM::SetCustomData(
    const int32 InstanceIndexStart,
    const int32 InstanceIndexEnd,
    TConstArrayView<float> CustomDataFloats,
    const bool bMarkRenderStateDirty) {
    if (RejectSealedRuntimeMutation(TEXT("SetCustomDataRange"))) {
        return false;
    }
    QuarantineHarvestIdentityRegistration();
    const bool bResult = Super::SetCustomData(InstanceIndexStart,
                                              InstanceIndexEnd,
                                              CustomDataFloats,
                                              bMarkRenderStateDirty);
    RequestDeferredIdentityRefresh();
    return bResult;
}

void UMythicResourceISM::QueueProviderQuarantineAndRetry(
    const TCHAR *FailureReason) {
    UE_LOG(Myth, Error,
           TEXT("Harvest presentation integrity failure on '%s': %s. The provider will be quarantined next tick and retried with bounded exponential backoff."),
           *GetPathName(), FailureReason);
    // Suppress query collision synchronously. Waiting for the deferred registry
    // detach would leave a one-frame ghost that can occlude LOS or consume an
    // attack sweep even though stable hit resolution already fails.
    SuppressHarvestQueryCollision();
    UWorld *World = GetWorld();
    if (bProviderQuarantinePending || !World || !World->IsGameWorld()
        || World->bIsTearingDown) {
        return;
    }

    bProviderQuarantinePending = true;
    TWeakObjectPtr<UMythicResourceISM> WeakThis(this);
    World->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateLambda([WeakThis]() {
            if (!WeakThis.IsValid()) {
                return;
            }
            WeakThis->bProviderQuarantinePending = false;
            WeakThis->QuarantineHarvestIdentityRegistration();
            WeakThis->bLastIdentityRefreshFailureRetryable = true;
            WeakThis->ScheduleIdentityRefreshRetry();
        }));
}

void UMythicResourceISM::ScheduleIdentityRefreshRetry() {
    UWorld *World = GetWorld();
    if (!World || !World->IsGameWorld() || World->bIsTearingDown
        || !IsRegistered()
        || !bLastIdentityRefreshFailureRetryable
        || World->GetTimerManager().IsTimerActive(
            IdentityRefreshRetryTimer)) {
        return;
    }

    float DelaySeconds = RecoveryIdentityRefreshRetrySeconds;
    if (IdentityRefreshRetryAttempt < MaxIdentityRefreshRetryAttempts) {
        const int32 Attempt = ++IdentityRefreshRetryAttempt;
        const int32 Exponent = FMath::Clamp(Attempt - 1, 0, 20);
        DelaySeconds = FMath::Min(
            InitialIdentityRefreshRetrySeconds
                * static_cast<float>(1 << Exponent),
            MaxIdentityRefreshRetrySeconds);
    }
    else if (!bIdentityRefreshInRecoveryMode) {
        bIdentityRefreshInRecoveryMode = true;
        UE_LOG(Myth, Warning,
               TEXT("Harvest provider '%s' exhausted %d fast registration retries; it remains quarantined and enters a %.1f-second health-recovery loop."),
               *GetPathName(), MaxIdentityRefreshRetryAttempts,
               RecoveryIdentityRefreshRetrySeconds);
    }
    TWeakObjectPtr<UMythicResourceISM> WeakThis(this);
    World->GetTimerManager().SetTimer(
        IdentityRefreshRetryTimer,
        FTimerDelegate::CreateLambda([WeakThis]() {
            if (!WeakThis.IsValid()) {
                return;
            }
            if (!WeakThis->RefreshHarvestIdentityRegistration()) {
                WeakThis->ScheduleIdentityRefreshRetry();
            }
        }),
        DelaySeconds, false);
}

ECollisionEnabled::Type
UMythicResourceISM::GetHarvestCollisionModeForValidation() const {
    return bHarvestQueryCollisionSuppressed
            && bHasAuthoredHarvestCollisionEnabled
        ? AuthoredHarvestCollisionEnabled
        : GetCollisionEnabled();
}

// The runtime gate tests capability, never the profile's label: every FBodyInstance collision setter calls
// InvalidateCollisionProfileName, so quarantining a provider renames its profile to Custom and a name check could
// never pass again, leaving a recovered tree permanently inert. Object channel and channel responses survive that
// invalidation, and IsDataValid still holds authored assets to the Destructible profile.
bool UMythicResourceISM::HasValidHarvestCollisionContract() const {
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    return CollisionEnabledHasQuery(GetHarvestCollisionModeForValidation())
        && GetCollisionObjectType() == ECC_Destructible && Settings
        && GetCollisionResponseToChannel(
               Settings->LineOfSightTraceChannel.GetValue()) == ECR_Block;
}

void UMythicResourceISM::SuppressHarvestQueryCollision() {
    const UWorld *World = GetWorld();
    if (!World || !World->IsGameWorld()) {
        return;
    }
    if (!bHarvestQueryCollisionSuppressed) {
        AuthoredHarvestCollisionEnabled = GetCollisionEnabled();
        bHasAuthoredHarvestCollisionEnabled = true;
        bHarvestQueryCollisionSuppressed = true;
    }
    if (GetCollisionEnabled() != ECollisionEnabled::NoCollision) {
        TGuardValue<int32> NativeMutationGuard(NativeMutationDepth,
                                               NativeMutationDepth + 1);
        Super::SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
}

bool UMythicResourceISM::RestoreHarvestQueryCollision() {
    if (!bHarvestQueryCollisionSuppressed
        || !bHasAuthoredHarvestCollisionEnabled
        || !HasValidHarvestCollisionContract()) {
        return false;
    }
    const ECollisionEnabled::Type CollisionToRestore =
        AuthoredHarvestCollisionEnabled;
    {
        TGuardValue<int32> NativeMutationGuard(NativeMutationDepth,
                                               NativeMutationDepth + 1);
        Super::SetCollisionEnabled(CollisionToRestore);
    }
    if (GetCollisionEnabled() != CollisionToRestore
        || !CollisionEnabledHasQuery(GetCollisionEnabled())) {
        TGuardValue<int32> NativeMutationGuard(NativeMutationDepth,
                                               NativeMutationDepth + 1);
        Super::SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return false;
    }
    bHarvestQueryCollisionSuppressed = false;
    bHasAuthoredHarvestCollisionEnabled = false;
    return true;
}

void UMythicResourceISM::QuarantineHarvestIdentityRegistration() {
    SuppressHarvestQueryCollision();
    if (UWorld *World = GetWorld()) {
        if (UMythicHarvestWorldSubsystem *Subsystem =
                World->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
            if (World->GetNetMode() == NM_Client) {
                Subsystem->UnregisterClientPresentationProvider(*this);
            }
            else {
                Subsystem->UnregisterResourceProvider(*this);
            }
        }
    }

    // Primitive ids and their index relation are invalid as soon as any
    // instance/layout/identity mutation starts. Original transforms and the
    // hidden set intentionally survive this same-object quarantine so an
    // unavailable node can be repaired/re-registered without becoming stuck at
    // its hidden presentation transform.
    StableNodeByPrimitiveValue.Reset();
    PrimitiveByStableNode.Reset();
}

void UMythicResourceISM::RequestDeferredIdentityRefresh() {
    if (bIdentityRefreshInProgress || bDeferredIdentityRefreshRequested
        || !IsRegistered() || !GetWorld() || !GetWorld()->IsGameWorld()
        || GetWorld()->bIsTearingDown) {
        return;
    }
    if (GetInstanceCount() <= 0) {
        UnregisterHarvestIdentityProvider();
        return;
    }
    bDeferredIdentityRefreshRequested = true;
    TWeakObjectPtr<UMythicResourceISM> WeakThis(this);
    GetWorld()->GetTimerManager().SetTimerForNextTick(
        FTimerDelegate::CreateLambda([WeakThis]() {
            if (!WeakThis.IsValid()) {
                return;
            }
            WeakThis->bDeferredIdentityRefreshRequested = false;
            if (!WeakThis->RefreshHarvestIdentityRegistration()
                && WeakThis->bLastIdentityRefreshFailureRetryable) {
                WeakThis->ScheduleIdentityRefreshRetry();
            }
        }));
}

bool UMythicResourceISM::DecodeStableNodeIdAtIndex(
    const int32 CurrentInstanceIndex,
    FMythicHarvestNodeId &OutNodeId) const {
    OutNodeId = FMythicHarvestNodeId();
    constexpr int32 PackedCount = MythicHarvestPCGIdentity::PackedFloatCount;
    if (CurrentInstanceIndex < 0 || CurrentInstanceIndex >= GetInstanceCount()
        || IdentityCustomDataStartIndex < 0
        || IdentityCustomDataStartIndex > MAX_int32 - PackedCount
        || NumCustomDataFloats
            < IdentityCustomDataStartIndex + PackedCount) {
        return false;
    }
    const int64 Start = static_cast<int64>(CurrentInstanceIndex)
            * static_cast<int64>(NumCustomDataFloats)
        + IdentityCustomDataStartIndex;
    if (Start < 0 || Start + PackedCount > PerInstanceSMCustomData.Num()) {
        return false;
    }
    const TConstArrayView<float> PackedData = MakeArrayView(
        PerInstanceSMCustomData.GetData() + static_cast<int32>(Start),
        PackedCount);
    if (IdentitySource == EMythicHarvestIdentitySource::PCGPacked) {
        return MythicHarvestPCGIdentity::TryDecodePackedNodeId(PackedData,
                                                               OutNodeId);
    }
    if (IdentitySource == EMythicHarvestIdentitySource::EditorAuthored) {
        FGuid InstanceGuid;
        return MythicHarvestAuthoredIdentity::TryDecodePackedInstanceGuid(
                   PackedData, InstanceGuid)
            && MythicHarvestAuthoredIdentity::TryBuildNodeId(
                {AuthoredNodeSetGuid, InstanceGuid}, OutNodeId);
    }
    return false;
}

bool UMythicResourceISM::RefreshHarvestIdentityRegistration() {
    bLastIdentityRefreshFailureRetryable = false;
    if (bIdentityRefreshInProgress) {
        return false;
    }
    if (!IsRegistered() || !GetWorld() || !GetWorld()->IsGameWorld()
        || !HarvestableDefinition || GetInstanceCount() <= 0) {
        QuarantineHarvestIdentityRegistration();
        return false;
    }
    if (!HasValidHarvestCollisionContract()) {
        UE_LOG(Myth, Error,
               TEXT("Harvest identity registration rejected provider '%s': collision must use the Destructible object channel, include query collision, and block the configured harvesting line-of-sight trace."),
               *GetPathName());
        QuarantineHarvestIdentityRegistration();
        return false;
    }
    // Manual health checks and WP re-registration may begin from a healthy
    // provider. Close the query surface for the complete rebuild so hits can
    // never observe caches from two provider incarnations.
    SuppressHarvestQueryCollision();
    TGuardValue<bool> RefreshGuard(bIdentityRefreshInProgress, true);

    TMap<int32, FMythicHarvestNodeId> NewStableByPrimitive;
    TMap<FMythicHarvestNodeId, FPrimitiveInstanceId> NewPrimitiveByStable;
    TMap<FMythicHarvestNodeId, FTransform> NewOriginalTransforms;
    NewStableByPrimitive.Reserve(GetInstanceCount());
    NewPrimitiveByStable.Reserve(GetInstanceCount());
    NewOriginalTransforms.Reserve(GetInstanceCount());

    for (int32 CurrentIndex = 0; CurrentIndex < GetInstanceCount(); ++CurrentIndex) {
        const FPrimitiveInstanceId PrimitiveId =
            PrimitiveInstanceDataManager.IndexToId(CurrentIndex);
#if WITH_DEV_AUTOMATION_TESTS
        const bool bInjectPrimitiveMapFailure =
            TestPrimitiveMapFailureCount > 0;
        if (bInjectPrimitiveMapFailure) {
            --TestPrimitiveMapFailureCount;
        }
#else
        constexpr bool bInjectPrimitiveMapFailure = false;
#endif
        if (bInjectPrimitiveMapFailure || !PrimitiveId.IsValid()
            || GetInstanceIndexForId(PrimitiveId) != CurrentIndex
            || NewStableByPrimitive.Contains(PrimitiveId.GetAsIndex())) {
            UE_LOG(Myth, Warning,
                   TEXT("Harvest provider '%s' temporarily lacks a coherent primitive-id/index map at current index %d; it remains quarantined for recovery."),
                   *GetPathName(), CurrentIndex);
            bLastIdentityRefreshFailureRetryable = true;
            QuarantineHarvestIdentityRegistration();
            return false;
        }
        FMythicHarvestNodeId NodeId;
        if (!DecodeStableNodeIdAtIndex(CurrentIndex, NodeId)
            || !NodeId.IsValid() || NewPrimitiveByStable.Contains(NodeId)) {
            UE_LOG(Myth, Error,
                   TEXT("Harvest identity registration rejected complete provider '%s': invalid or duplicate identity at current index %d."),
                   *GetPathName(), CurrentIndex);
            QuarantineHarvestIdentityRegistration();
            return false;
        }
        FTransform WorldTransform;
#if WITH_DEV_AUTOMATION_TESTS
        const bool bInjectTransformReadFailure =
            TestTransformReadFailureCount > 0;
        if (bInjectTransformReadFailure) {
            --TestTransformReadFailureCount;
        }
#else
        constexpr bool bInjectTransformReadFailure = false;
#endif
        if (bInjectTransformReadFailure
            || !GetInstanceTransform(CurrentIndex, WorldTransform, true)) {
            UE_LOG(Myth, Warning,
                   TEXT("Harvest provider '%s' temporarily cannot read the world transform for current instance %d; it remains quarantined for recovery."),
                   *GetPathName(), CurrentIndex);
            bLastIdentityRefreshFailureRetryable = true;
            QuarantineHarvestIdentityRegistration();
            return false;
        }
        NewStableByPrimitive.Add(PrimitiveId.GetAsIndex(), NodeId);
        NewPrimitiveByStable.Add(NodeId, PrimitiveId);
        if (const FTransform *ExistingOriginal =
                OriginalWorldTransformByNode.Find(NodeId)) {
            NewOriginalTransforms.Add(NodeId, *ExistingOriginal);
        }
        else {
            NewOriginalTransforms.Add(NodeId, WorldTransform);
        }
    }

    // Publish the prospective provider cache only for the duration of the
    // subsystem's atomic complete-batch validation. A rejected batch is
    // quarantined, never rolled back to an identity map that no longer matches
    // the component's serialized custom data.
    TSet<FMythicHarvestNodeId> PreviousHiddenNodes = MoveTemp(HiddenNodes);

    StableNodeByPrimitiveValue = MoveTemp(NewStableByPrimitive);
    PrimitiveByStableNode = MoveTemp(NewPrimitiveByStable);
    OriginalWorldTransformByNode = MoveTemp(NewOriginalTransforms);
    for (const FMythicHarvestNodeId &HiddenNode : PreviousHiddenNodes) {
        if (PrimitiveByStableNode.Contains(HiddenNode)) {
            HiddenNodes.Add(HiddenNode);
        }
    }

    if (UMythicHarvestWorldSubsystem *Subsystem =
            GetWorld()->GetSubsystem<UMythicHarvestWorldSubsystem>()) {
        bLastIdentityRefreshFailureRetryable = true;
        const bool bProviderRegistered = GetWorld()->GetNetMode() == NM_Client
            ? Subsystem->RefreshClientPresentationProvider(*this)
            : Subsystem->RefreshResourceProvider(*this);
        if (bProviderRegistered) {
            if (bProviderQuarantinePending) {
                QuarantineHarvestIdentityRegistration();
                return false;
            }
            if (!RestoreHarvestQueryCollision()) {
                UE_LOG(Myth, Warning,
                       TEXT("Harvest provider '%s' completed identity and availability reconciliation but could not restore query collision; it remains quarantined for recovery."),
                       *GetPathName());
                bLastIdentityRefreshFailureRetryable = true;
                QuarantineHarvestIdentityRegistration();
                return false;
            }
            IdentityRefreshRetryAttempt = 0;
            bIdentityRefreshInRecoveryMode = false;
            bLastIdentityRefreshFailureRetryable = false;
            GetWorld()->GetTimerManager().ClearTimer(
                IdentityRefreshRetryTimer);
            return true;
        }
    }
    else {
        // The world subsystem may not have completed initialization on this
        // deferred component tick; this is ordering, not authoring corruption.
        bLastIdentityRefreshFailureRetryable = true;
    }

    QuarantineHarvestIdentityRegistration();
    return false;
}

void UMythicResourceISM::UnregisterHarvestIdentityProvider() {
    QuarantineHarvestIdentityRegistration();
    OriginalWorldTransformByNode.Reset();
    HiddenNodes.Reset();
}

bool UMythicResourceISM::ResolveStableNodeId(
    const FPrimitiveInstanceId PrimitiveInstanceId,
    FMythicHarvestNodeId &OutNodeId) const {
    OutNodeId = FMythicHarvestNodeId();
    if (!PrimitiveInstanceId.IsValid()) {
        return false;
    }
    const FMythicHarvestNodeId *Found =
        StableNodeByPrimitiveValue.Find(PrimitiveInstanceId.GetAsIndex());
    if (!Found || !Found->IsValid()) {
        return false;
    }
    OutNodeId = *Found;
    return true;
}

bool UMythicResourceISM::ResolveAuthoritativeHitInstance(
    const int32 CurrentInstanceIndex,
    FPrimitiveInstanceId &OutPrimitiveInstanceId,
    FMythicHarvestNodeId &OutNodeId) const {
    OutPrimitiveInstanceId = FPrimitiveInstanceId();
    OutNodeId = FMythicHarvestNodeId();
    if (CurrentInstanceIndex < 0 || CurrentInstanceIndex >= GetInstanceCount()) {
        return false;
    }
    const FPrimitiveInstanceId PrimitiveId =
        PrimitiveInstanceDataManager.IndexToId(CurrentInstanceIndex);
    if (!PrimitiveId.IsValid()
        || GetInstanceIndexForId(PrimitiveId) != CurrentInstanceIndex
        || !ResolveStableNodeId(PrimitiveId, OutNodeId)) {
        return false;
    }
    OutPrimitiveInstanceId = PrimitiveId;
    return true;
}

bool UMythicResourceISM::ResolvePrimitiveInstanceId(
    const FMythicHarvestNodeId &NodeId,
    FPrimitiveInstanceId &OutPrimitiveInstanceId) const {
    OutPrimitiveInstanceId = FPrimitiveInstanceId();
    const FPrimitiveInstanceId *Found = PrimitiveByStableNode.Find(NodeId);
    if (!Found || !Found->IsValid()
        || GetInstanceIndexForId(*Found) == INDEX_NONE) {
        return false;
    }
    OutPrimitiveInstanceId = *Found;
    return true;
}

void UMythicResourceISM::GetHarvestProviderNodes(
    TArray<FMythicHarvestProviderNode> &OutNodes) const {
    OutNodes.Reset(PrimitiveByStableNode.Num());
    for (const TPair<FMythicHarvestNodeId, FPrimitiveInstanceId> &Pair :
         PrimitiveByStableNode) {
        const FTransform *Original = OriginalWorldTransformByNode.Find(Pair.Key);
        if (Original) {
            FMythicHarvestProviderNode &Row = OutNodes.AddDefaulted_GetRef();
            Row.NodeId = Pair.Key;
            Row.PrimitiveInstanceId = Pair.Value;
            Row.OriginalWorldTransform = *Original;
        }
    }
}

bool UMythicResourceISM::ApplyNodeAvailability(
    const FMythicHarvestNodeId &NodeId,
    const bool bAvailable) {
    const FMythicHarvestNodePresentationUpdate Update{NodeId, bAvailable};
    return ApplyNodeAvailabilityBatch(MakeArrayView(&Update, 1));
}

bool UMythicResourceISM::ApplyNodeAvailabilityBatch(
    const TConstArrayView<FMythicHarvestNodePresentationUpdate> Updates) {
    struct FPendingTransformUpdate {
        int32 CurrentIndex = INDEX_NONE;
        FTransform DesiredWorldTransform;
    };

    TArray<FPendingTransformUpdate> PendingTransforms;
    PendingTransforms.Reserve(Updates.Num());
    TSet<FMythicHarvestNodeId> SeenNodeIds;
    SeenNodeIds.Reserve(Updates.Num());

    for (const FMythicHarvestNodePresentationUpdate &Update : Updates) {
        FPrimitiveInstanceId PrimitiveId;
        const FTransform *Original =
            OriginalWorldTransformByNode.Find(Update.NodeId);
        if (!Update.NodeId.IsValid() || SeenNodeIds.Contains(Update.NodeId)
            || !Original
            || !ResolvePrimitiveInstanceId(Update.NodeId, PrimitiveId)) {
            QueueProviderQuarantineAndRetry(
                TEXT("availability batch contains a duplicate/unresolved stable node"));
            return false;
        }
        SeenNodeIds.Add(Update.NodeId);

        const int32 CurrentIndex = GetInstanceIndexForId(PrimitiveId);
        if (CurrentIndex == INDEX_NONE
            || PrimitiveInstanceDataManager.IndexToId(CurrentIndex)
                != PrimitiveId) {
            QueueProviderQuarantineAndRetry(
                TEXT("primitive id/index round-trip validation failed"));
            return false;
        }

        FTransform Desired = *Original;
        if (!Update.bAvailable) {
            Desired.AddToTranslation(FVector(0.0, 0.0, -1000000.0));
        }
        FTransform CurrentWorldTransform;
        if (!GetInstanceTransform(CurrentIndex, CurrentWorldTransform, true)) {
            QueueProviderQuarantineAndRetry(
                TEXT("availability preflight could not read an instance transform"));
            return false;
        }
        if (!CurrentWorldTransform.Equals(Desired, 0.1)) {
            FPendingTransformUpdate &Pending =
                PendingTransforms.AddDefaulted_GetRef();
            Pending.CurrentIndex = CurrentIndex;
            Pending.DesiredWorldTransform = Desired;
        }
    }

    PendingTransforms.Sort(
        [](const FPendingTransformUpdate &Left,
           const FPendingTransformUpdate &Right) {
            return Left.CurrentIndex < Right.CurrentIndex;
        });
    for (int32 PendingIndex = 1; PendingIndex < PendingTransforms.Num();
         ++PendingIndex) {
        if (PendingTransforms[PendingIndex - 1].CurrentIndex
            == PendingTransforms[PendingIndex].CurrentIndex) {
            QueueProviderQuarantineAndRetry(
                TEXT("availability batch maps multiple stable nodes to one primitive"));
            return false;
        }
    }

    int32 RunStart = 0;
    while (RunStart < PendingTransforms.Num()) {
        int32 RunEnd = RunStart + 1;
        while (RunEnd < PendingTransforms.Num()
               && PendingTransforms[RunEnd].CurrentIndex
                   == PendingTransforms[RunEnd - 1].CurrentIndex + 1) {
            ++RunEnd;
        }
        TArray<FTransform> RunTransforms;
        RunTransforms.Reserve(RunEnd - RunStart);
        for (int32 PendingIndex = RunStart; PendingIndex < RunEnd;
             ++PendingIndex) {
            RunTransforms.Add(
                PendingTransforms[PendingIndex].DesiredWorldTransform);
        }
        bool bApplied = false;
        {
            TGuardValue<int32> NativeMutationGuard(NativeMutationDepth,
                                                   NativeMutationDepth + 1);
            bApplied = Super::BatchUpdateInstancesTransforms(
                PendingTransforms[RunStart].CurrentIndex, RunTransforms,
                true, RunEnd == PendingTransforms.Num(), true);
        }
#if WITH_DEV_AUTOMATION_TESTS
        AvailabilityTransformWriteCountForTests += RunTransforms.Num();
        ++AvailabilityNativeBatchCallCountForTests;
#endif
        if (!bApplied) {
            QueueProviderQuarantineAndRetry(
                TEXT("native availability transform batch failed"));
            return false;
        }
        RunStart = RunEnd;
    }

    for (const FPendingTransformUpdate &Pending : PendingTransforms) {
        FTransform AppliedWorldTransform;
        if (!GetInstanceTransform(Pending.CurrentIndex, AppliedWorldTransform,
                                  true)
            || !AppliedWorldTransform.Equals(Pending.DesiredWorldTransform,
                                             0.1)) {
            QueueProviderQuarantineAndRetry(
                TEXT("verified availability transform batch failed"));
            return false;
        }
    }

    for (const FMythicHarvestNodePresentationUpdate &Update : Updates) {
        if (Update.bAvailable) {
            HiddenNodes.Remove(Update.NodeId);
        }
        else {
            HiddenNodes.Add(Update.NodeId);
        }
    }
    return true;
}

#if WITH_EDITOR
EDataValidationResult UMythicResourceISM::IsDataValid(
    FDataValidationContext &Context) const {
    const EDataValidationResult ParentResult = Super::IsDataValid(Context);
    if (!HarvestableDefinition) {
        Context.AddError(NSLOCTEXT("MythicHarvest", "MissingISMDefinition",
                                  "Resource ISM requires one direct Harvestable Definition."));
        return EDataValidationResult::Invalid;
    }
    if (IdentityCustomDataStartIndex < 0
        || IdentityCustomDataStartIndex
            > MAX_int32 - MythicHarvestPCGIdentity::PackedFloatCount) {
        Context.AddError(NSLOCTEXT(
            "MythicHarvest", "InvalidISMIdentityOffset",
            "Resource ISM identity custom-data start must be non-negative and leave room for eight packed values."));
        return EDataValidationResult::Invalid;
    }
    if (IdentitySource == EMythicHarvestIdentitySource::EditorAuthored
        && !AuthoredNodeSetGuid.IsValid()) {
        Context.AddError(NSLOCTEXT(
            "MythicHarvest", "MissingISMAuthoredNodeSetGuid",
            "Editor-authored Resource ISM identity requires a persistent Node Set GUID; run Bake Missing Authored Identities."));
        return EDataValidationResult::Invalid;
    }
    if (IdentitySource != EMythicHarvestIdentitySource::PCGPacked
        && IdentitySource != EMythicHarvestIdentitySource::EditorAuthored) {
        Context.AddError(NSLOCTEXT(
            "MythicHarvest", "InvalidISMIdentitySource",
            "Resource ISM has an unknown identity source and cannot register."));
        return EDataValidationResult::Invalid;
    }
    if (GetInstanceCount() > 0) {
        const int32 RequiredStride = IdentityCustomDataStartIndex
            + MythicHarvestPCGIdentity::PackedFloatCount;
        if (NumCustomDataFloats < RequiredStride) {
            Context.AddError(NSLOCTEXT(
                "MythicHarvest", "MissingISMIdentityCustomData",
                "Resource ISM instances do not contain the configured eight-value identity custom-data block."));
            return EDataValidationResult::Invalid;
        }
        const int64 ExpectedCustomDataCount =
            static_cast<int64>(GetInstanceCount()) * NumCustomDataFloats;
        if (ExpectedCustomDataCount != PerInstanceSMCustomData.Num()) {
            Context.AddError(NSLOCTEXT(
                "MythicHarvest", "MalformedISMCustomDataShape",
                "Resource ISM per-instance custom-data buffer does not match its instance count and row stride."));
            return EDataValidationResult::Invalid;
        }
        TMap<FMythicHarvestNodeId, int32> FirstIndexByNodeId;
        FirstIndexByNodeId.Reserve(GetInstanceCount());
        for (int32 InstanceIndex = 0; InstanceIndex < GetInstanceCount();
             ++InstanceIndex) {
            FMythicHarvestNodeId NodeId;
            if (!DecodeStableNodeIdAtIndex(InstanceIndex, NodeId)
                || !NodeId.IsValid()) {
                Context.AddError(FText::Format(
                    NSLOCTEXT("MythicHarvest", "InvalidISMInstanceIdentity",
                              "Resource ISM instance {0} has missing or malformed stable identity data."),
                    FText::AsNumber(InstanceIndex)));
                return EDataValidationResult::Invalid;
            }
            if (const int32 *FirstIndex = FirstIndexByNodeId.Find(NodeId)) {
                Context.AddError(FText::Format(
                    NSLOCTEXT("MythicHarvest", "DuplicateISMInstanceIdentity",
                              "Resource ISM instances {0} and {1} resolve to the same stable node identity."),
                    FText::AsNumber(*FirstIndex),
                    FText::AsNumber(InstanceIndex)));
                return EDataValidationResult::Invalid;
            }
            FirstIndexByNodeId.Add(NodeId, InstanceIndex);
        }
    }
    if (GetCollisionProfileName() != HarvestCollisionProfileName
        || !CollisionEnabledHasQuery(GetHarvestCollisionModeForValidation())
        || GetCollisionObjectType() != ECC_Destructible) {
        Context.AddError(NSLOCTEXT(
            "MythicHarvest", "InvalidISMCollisionProfile",
            "Resource ISM requires the Destructible collision profile, enabled query collision, and the Destructible object channel."));
        return EDataValidationResult::Invalid;
    }
    const UMythicHarvestSettings *Settings =
        GetDefault<UMythicHarvestSettings>();
    if (!Settings
        || GetCollisionResponseToChannel(
               Settings->LineOfSightTraceChannel.GetValue()) != ECR_Block) {
        Context.AddError(NSLOCTEXT(
            "MythicHarvest", "InvalidISMLineOfSightCollision",
            "Resource ISM must block the configured harvesting line-of-sight trace channel."));
        return EDataValidationResult::Invalid;
    }
    return ParentResult;
}

void UMythicResourceISM::PostEditImport() {
    Super::PostEditImport();
    if (IdentitySource != EMythicHarvestIdentitySource::EditorAuthored) {
        return;
    }
    if (const UWorld *World = GetWorld(); World && World->IsGameWorld()) {
        return;
    }

    // Copy/paste must create a new node namespace while retaining every
    // per-instance GUID and material custom-data value on the duplicate.
    Modify();
    AuthoredNodeSetGuid = FGuid::NewGuid();
    MarkPackageDirty();
}
#endif
