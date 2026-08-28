#pragma once

#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestTypes.h"

/**
 * Canonical identity for one manually authored harvest instance. NodeSetGuid identifies one persistent authored ISM
 * component placement and InstanceGuid travels with one serialized per-instance custom-data row. Neither value may
 * be synthesized at runtime or derived from instance order, transforms, names, paths, or strings.
 */
struct MYTHIC_API FMythicHarvestAuthoredIdentityInput {
    FGuid NodeSetGuid;
    FGuid InstanceGuid;

    bool IsValid() const {
        return NodeSetGuid.IsValid() && InstanceGuid.IsValid();
    }
};

/** Closed validation result for a complete batch of manually authored harvesting identities. */
enum class EMythicHarvestAuthoredIdentityError : uint8 {
    None,
    InvalidNodeSetGuid,
    InvalidInstanceGuid,
    CanonicalizationFailed,
    DuplicateInstanceGuid,
    DuplicateStableIdentity,
};

/** Identifies the first invalid or duplicate authored row without exposing a mutable runtime instance index. */
struct MYTHIC_API FMythicHarvestAuthoredIdentityValidationResult {
    EMythicHarvestAuthoredIdentityError Error =
        EMythicHarvestAuthoredIdentityError::None;
    int32 FailureIndex = INDEX_NONE;
    int32 ConflictingIndex = INDEX_NONE;

    bool IsValid() const {
        return Error == EMythicHarvestAuthoredIdentityError::None;
    }
};

namespace MythicHarvestAuthoredIdentity {

/** Frozen canonical-input version. Incrementing it intentionally changes every manually authored node identity. */
inline constexpr uint32 CanonicalVersion = 1;

/**
 * Builds frozen, platform-independent bytes in this order: the NUL-terminated ASCII protocol domain, big-endian
 * uint32 version, node-set GUID words A-D, then per-instance GUID words A-D.
 */
MYTHIC_API bool BuildCanonicalBytes(
    const FMythicHarvestAuthoredIdentityInput &Input,
    TArray<uint8> &OutBytes);

/** Hashes one valid canonical authored input into an opaque stable node id; failure always clears OutNodeId. */
MYTHIC_API bool TryBuildNodeId(
    const FMythicHarvestAuthoredIdentityInput &Input,
    FMythicHarvestNodeId &OutNodeId);

/**
 * Validates a complete authored batch and builds node ids in input order. Invalid node-set/instance GUIDs and
 * duplicate instance or derived identities reject the entire batch without returning a partial result.
 */
MYTHIC_API FMythicHarvestAuthoredIdentityValidationResult
ValidateAndBuildNodeIds(const FGuid &NodeSetGuid,
                        TConstArrayView<FGuid> InstanceGuids,
                        TArray<FMythicHarvestNodeId> &OutNodeIds);

/** Appends one persistent per-instance GUID using the frozen eight-exact-float ISM encoding. */
MYTHIC_API bool AppendPackedInstanceGuid(const FGuid &InstanceGuid,
                                         TArray<float> &InOutFloats);

/** Decodes one persistent per-instance GUID from exactly eight frozen ISM custom-data floats. */
MYTHIC_API bool TryDecodePackedInstanceGuid(
    TConstArrayView<float> PackedFloats, FGuid &OutInstanceGuid);

} // namespace MythicHarvestAuthoredIdentity
