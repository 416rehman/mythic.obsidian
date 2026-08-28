#pragma once

#include "CoreMinimal.h"
#include "World/Harvesting/MythicHarvestTypes.h"

/**
 * Canonical native identity supplied by a PCG harvesting provider. ProviderGuid identifies the persistent graph or
 * spawner, DomainGuid identifies its persistent generation domain (for example a cell), and PointIdentity is a
 * non-negative deterministic value emitted by the generator. None of these values may be derived from instance order.
 */
struct MYTHIC_API FMythicHarvestPCGIdentityInput {
    FGuid ProviderGuid;
    FGuid DomainGuid;
    int64 PointIdentity = INDEX_NONE;

    bool IsValid() const {
        return ProviderGuid.IsValid() && DomainGuid.IsValid() && PointIdentity >= 0;
    }
};

/** Closed validation result for a batch of PCG harvesting identities. */
enum class EMythicHarvestPCGIdentityError : uint8 {
    None,
    InvalidProviderGuid,
    InvalidDomainGuid,
    InvalidPointIdentity,
    CanonicalizationFailed,
    DuplicateStableIdentity,
};

/** Identifies the first invalid or duplicate input without exposing mutable runtime handles. */
struct MYTHIC_API FMythicHarvestPCGIdentityValidationResult {
    EMythicHarvestPCGIdentityError Error = EMythicHarvestPCGIdentityError::None;
    int32 FailureIndex = INDEX_NONE;
    int32 ConflictingIndex = INDEX_NONE;

    bool IsValid() const {
        return Error == EMythicHarvestPCGIdentityError::None;
    }
};

namespace MythicHarvestPCGIdentity {

/** Frozen canonical-input version. Incrementing it intentionally changes every derived PCG node identity. */
inline constexpr uint32 CanonicalVersion = 1;

/** Number of exact uint16-in-float values used to carry one opaque node GUID through ISM custom data. */
inline constexpr int32 PackedFloatCount = 8;

/**
 * Custom-data floats reserved ahead of the identity payload. Zero today: PCG refuses to re-emit a changed stride for
 * an already-generated graph, so moving the payload silently leaves the world on the old layout. Tracked separately
 * with the material collision it exists to solve.
 */
inline constexpr int32 MaterialReservedLeadingFloats = 0;

/** Total custom-data stride a generated harvest provider carries: reserved material floats then the identity. */
inline constexpr int32 PackedStride =
    MaterialReservedLeadingFloats + PackedFloatCount;

/**
 * Derives an order-independent 63-bit point identity from PCG's typed native seed and source-point world position.
 * Position is quantized to whole centimeters before hashing, so array/metadata order never participates and a
 * deliberate point move creates a new cooked node identity. Invalid/nonfinite inputs fail and clear OutIdentity.
 */
MYTHIC_API bool TryBuildDeterministicPointIdentity(
    int32 NativePointSeed, const FVector &SourcePointWorldPosition,
    int64 &OutIdentity);

/**
 * Builds frozen, platform-independent bytes in this order: the NUL-terminated ASCII protocol domain, big-endian
 * uint32 version, provider GUID words A-D, domain GUID words A-D, and the big-endian uint64 point identity.
 */
MYTHIC_API bool BuildCanonicalBytes(const FMythicHarvestPCGIdentityInput &Input,
                                    TArray<uint8> &OutBytes);

/** Hashes one valid canonical input into an opaque stable node id; failure always clears OutNodeId. */
MYTHIC_API bool TryBuildNodeId(const FMythicHarvestPCGIdentityInput &Input,
                              FMythicHarvestNodeId &OutNodeId);

/**
 * Validates a batch and builds ids in input order. Missing values, invalid GUIDs, and duplicate derived ids fail the
 * entire batch without returning a partial set; duplicate indices identify both conflicting inputs.
 */
MYTHIC_API FMythicHarvestPCGIdentityValidationResult ValidateAndBuildNodeIds(
    TConstArrayView<FMythicHarvestPCGIdentityInput> Inputs,
    TArray<FMythicHarvestNodeId> &OutNodeIds);

/** Appends one node id as eight exactly representable integral floats in GUID word/high-half/low-half order. */
MYTHIC_API bool AppendPackedNodeId(const FMythicHarvestNodeId &NodeId,
                                  TArray<float> &InOutFloats);

/**
 * Decodes exactly eight integral floats in [0, 65535] into a node id. NaN, infinity, fractional, out-of-range, and
 * all-zero encodings fail and clear OutNodeId.
 */
MYTHIC_API bool TryDecodePackedNodeId(TConstArrayView<float> PackedFloats,
                                     FMythicHarvestNodeId &OutNodeId);

} // namespace MythicHarvestPCGIdentity
