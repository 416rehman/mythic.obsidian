#include "World/Harvesting/MythicHarvestPCGInstanceDataPacker.h"

#include "Data/PCGBasePointData.h"
#include "MeshSelectors/PCGMeshSelectorBase.h"
#include "Misc/DataValidation.h"
#include "PCGContext.h"
#include "PCGElement.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicHarvestPCGInstanceDataPacker)

#define LOCTEXT_NAMESPACE "MythicHarvestPCGInstanceDataPacker"

namespace {

void InitializeFailedOutput(const int32 NumInstances,
                            FPCGPackedCustomData &OutPackedCustomData) {
    OutPackedCustomData.Reset();
    OutPackedCustomData.NumCustomDataFloats =
        MythicHarvestPCGIdentity::PackedStride;
    OutPackedCustomData.CustomData.SetNumZeroed(
        NumInstances * MythicHarvestPCGIdentity::PackedStride);
}

FText DescribeValidationFailure(
    const FMythicHarvestPCGIdentityValidationResult &Failure) {
    switch (Failure.Error) {
    case EMythicHarvestPCGIdentityError::InvalidProviderGuid:
        return LOCTEXT("InvalidProviderGuid", "ProviderGuid is invalid.");
    case EMythicHarvestPCGIdentityError::InvalidDomainGuid:
        return LOCTEXT("InvalidDomainGuid", "DomainGuid is invalid.");
    case EMythicHarvestPCGIdentityError::InvalidPointIdentity:
        return FText::Format(
            LOCTEXT("InvalidPointIdentity",
                    "Point identity at packed instance {0} is negative."),
            FText::AsNumber(Failure.FailureIndex));
    case EMythicHarvestPCGIdentityError::CanonicalizationFailed:
        return FText::Format(
            LOCTEXT("CanonicalizationFailed",
                    "Stable-id canonicalization failed at packed instance {0}."),
            FText::AsNumber(Failure.FailureIndex));
    case EMythicHarvestPCGIdentityError::DuplicateStableIdentity:
        return FText::Format(
            LOCTEXT("DuplicateIdentity",
                    "Packed instances {0} and {1} derive the same stable node id."),
            FText::AsNumber(Failure.ConflictingIndex),
            FText::AsNumber(Failure.FailureIndex));
    default:
        return LOCTEXT("UnknownIdentityFailure",
                       "Stable harvesting identity validation failed.");
    }
}

} // namespace

void UMythicHarvestPCGInstanceDataPacker::PackInstances_Implementation(
    FPCGContext &Context, const UPCGSpatialData *InSpatialData,
    const FPCGMeshInstanceList &InstanceList,
    FPCGPackedCustomData &OutPackedCustomData) const {
    const int32 NumInstances = InstanceList.InstancesIndices.Num();
    InitializeFailedOutput(NumInstances, OutPackedCustomData);

    if (!InSpatialData) {
        PCGE_LOG_C(Error, GraphAndLog, &Context,
                   LOCTEXT("MissingSpatialData",
                           "Harvest identity packing requires spatial input data."));
        return;
    }

    const UPCGBasePointData *PointData = InstanceList.PointData.Get();
    if (!PointData || InstanceList.Instances.Num() != NumInstances) {
        PCGE_LOG_C(
            Error, GraphAndLog, &Context,
            LOCTEXT("InvalidInstanceList",
                    "Harvest identity packing requires one valid source point per mesh instance."));
        return;
    }

    if (!ProviderGuid.IsValid() || !DomainGuid.IsValid()) {
        PCGE_LOG_C(
            Error, GraphAndLog, &Context,
            LOCTEXT("InvalidPackerConfiguration",
                    "Harvest identity packer requires valid persistent provider and domain GUIDs."));
        return;
    }

    TArray<FMythicHarvestPCGIdentityInput> Inputs;
    Inputs.Reserve(NumInstances);
    for (int32 PackedIndex = 0; PackedIndex < NumInstances; ++PackedIndex) {
        const int32 PointIndex = InstanceList.InstancesIndices[PackedIndex];
        if (PointIndex < 0 || PointIndex >= PointData->GetNumPoints()) {
            PCGE_LOG_C(
                Error, GraphAndLog, &Context,
                FText::Format(
                    LOCTEXT("InvalidPointIndex",
                            "Packed instance {0} references invalid PCG point index {1}."),
                    FText::AsNumber(PackedIndex), FText::AsNumber(PointIndex)));
            return;
        }

        int64 PointIdentity = INDEX_NONE;
        if (!MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
                PointData->GetSeed(PointIndex),
                PointData->GetTransform(PointIndex).GetLocation(),
                PointIdentity)) {
            PCGE_LOG_C(
                Error, GraphAndLog, &Context,
                FText::Format(
                    LOCTEXT("InvalidPointIdentityInput",
                            "Packed instance {0} has invalid native seed/position identity input."),
                    FText::AsNumber(PackedIndex)));
            return;
        }

        FMythicHarvestPCGIdentityInput &Input = Inputs.AddDefaulted_GetRef();
        Input.ProviderGuid = ProviderGuid;
        Input.DomainGuid = DomainGuid;
        Input.PointIdentity = PointIdentity;
    }

    TArray<FMythicHarvestNodeId> NodeIds;
    const FMythicHarvestPCGIdentityValidationResult Validation =
        MythicHarvestPCGIdentity::ValidateAndBuildNodeIds(Inputs, NodeIds);
    if (!Validation.IsValid()) {
        PCGE_LOG_C(Error, GraphAndLog, &Context,
                   DescribeValidationFailure(Validation));
        return;
    }

    OutPackedCustomData.NumCustomDataFloats =
        MythicHarvestPCGIdentity::PackedStride;
    OutPackedCustomData.CustomData.Reset(NumInstances
        * MythicHarvestPCGIdentity::PackedStride);
    for (const FMythicHarvestNodeId &NodeId : NodeIds) {
        OutPackedCustomData.CustomData.AddZeroed(
            MythicHarvestPCGIdentity::MaterialReservedLeadingFloats);
        if (!MythicHarvestPCGIdentity::AppendPackedNodeId(
                NodeId, OutPackedCustomData.CustomData)) {
            InitializeFailedOutput(NumInstances, OutPackedCustomData);
            PCGE_LOG_C(
                Error, GraphAndLog, &Context,
                LOCTEXT("PackNodeIdFailed",
                        "A validated harvest node id could not be packed."));
            return;
        }
    }
}

bool UMythicHarvestPCGInstanceDataPacker::GetAttributeNames(
    TArray<FName> * /*OutNames*/) {
    return true;
}

#if WITH_EDITOR
EDataValidationResult UMythicHarvestPCGInstanceDataPacker::IsDataValid(
    FDataValidationContext &Context) const {
    EDataValidationResult Result = Super::IsDataValid(Context);
    auto Error = [&Context, &Result](const FText &Message) {
        Context.AddError(Message);
        Result = EDataValidationResult::Invalid;
    };

    if (!ProviderGuid.IsValid()) {
        Error(LOCTEXT("ValidateProviderGuid",
                      "ProviderGuid must be a persistent non-zero GUID."));
    }
    if (!DomainGuid.IsValid()) {
        Error(LOCTEXT("ValidateDomainGuid",
                      "DomainGuid must be a persistent non-zero GUID."));
    }
    return Result;
}
#endif

#undef LOCTEXT_NAMESPACE
