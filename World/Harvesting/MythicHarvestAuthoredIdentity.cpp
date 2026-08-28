#include "World/Harvesting/MythicHarvestAuthoredIdentity.h"

#include "Hash/Blake3.h"
#include "World/Harvesting/MythicHarvestPCGIdentity.h"

namespace {

// The trailing NUL is part of the frozen domain separator and prevents protocol-prefix aliasing.
constexpr uint8 AuthoredCanonicalDomain[] = {
    'M', 'Y', 'T', 'H', 'I', 'C', '_', 'H', 'A', 'R', 'V', 'E', 'S', 'T', '_',
    'A', 'U', 'T', 'H', 'O', 'R', 'E', 'D', '_', 'N', 'O', 'D', 'E', '_', 'I',
    'D', 0,
};

void AppendAuthoredUInt32BigEndian(const uint32 Value,
                                   TArray<uint8> &OutBytes) {
    OutBytes.Add(static_cast<uint8>(Value >> 24));
    OutBytes.Add(static_cast<uint8>(Value >> 16));
    OutBytes.Add(static_cast<uint8>(Value >> 8));
    OutBytes.Add(static_cast<uint8>(Value));
}

void AppendAuthoredGuidBigEndian(const FGuid &Guid,
                                 TArray<uint8> &OutBytes) {
    AppendAuthoredUInt32BigEndian(Guid.A, OutBytes);
    AppendAuthoredUInt32BigEndian(Guid.B, OutBytes);
    AppendAuthoredUInt32BigEndian(Guid.C, OutBytes);
    AppendAuthoredUInt32BigEndian(Guid.D, OutBytes);
}

uint32 ReadAuthoredUInt32BigEndian(const uint8 *Bytes) {
    return (static_cast<uint32>(Bytes[0]) << 24)
        | (static_cast<uint32>(Bytes[1]) << 16)
        | (static_cast<uint32>(Bytes[2]) << 8)
        | static_cast<uint32>(Bytes[3]);
}

} // namespace

bool MythicHarvestAuthoredIdentity::BuildCanonicalBytes(
    const FMythicHarvestAuthoredIdentityInput &Input,
    TArray<uint8> &OutBytes) {
    OutBytes.Reset();
    if (!Input.IsValid()) {
        return false;
    }

    constexpr int32 CanonicalByteCount = UE_ARRAY_COUNT(AuthoredCanonicalDomain)
        + sizeof(uint32) + sizeof(uint32) * 4 * 2;
    OutBytes.Reserve(CanonicalByteCount);
    OutBytes.Append(AuthoredCanonicalDomain,
                    UE_ARRAY_COUNT(AuthoredCanonicalDomain));
    AppendAuthoredUInt32BigEndian(CanonicalVersion, OutBytes);
    AppendAuthoredGuidBigEndian(Input.NodeSetGuid, OutBytes);
    AppendAuthoredGuidBigEndian(Input.InstanceGuid, OutBytes);
    return OutBytes.Num() == CanonicalByteCount;
}

bool MythicHarvestAuthoredIdentity::TryBuildNodeId(
    const FMythicHarvestAuthoredIdentityInput &Input,
    FMythicHarvestNodeId &OutNodeId) {
    OutNodeId = FMythicHarvestNodeId();

    TArray<uint8> CanonicalBytes;
    if (!BuildCanonicalBytes(Input, CanonicalBytes)) {
        return false;
    }

    const FBlake3Hash Digest = FBlake3::HashBuffer(
        CanonicalBytes.GetData(), static_cast<uint64>(CanonicalBytes.Num()));
    const uint8 *DigestBytes = Digest.GetBytes();
    const FGuid Guid(
        ReadAuthoredUInt32BigEndian(DigestBytes),
        ReadAuthoredUInt32BigEndian(DigestBytes + 4),
        ReadAuthoredUInt32BigEndian(DigestBytes + 8),
        ReadAuthoredUInt32BigEndian(DigestBytes + 12));
    if (!Guid.IsValid()) {
        return false;
    }

    OutNodeId = FMythicHarvestNodeId(Guid);
    return true;
}

FMythicHarvestAuthoredIdentityValidationResult
MythicHarvestAuthoredIdentity::ValidateAndBuildNodeIds(
    const FGuid &NodeSetGuid, const TConstArrayView<FGuid> InstanceGuids,
    TArray<FMythicHarvestNodeId> &OutNodeIds) {
    OutNodeIds.Reset();
    if (!NodeSetGuid.IsValid()) {
        return {EMythicHarvestAuthoredIdentityError::InvalidNodeSetGuid,
                INDEX_NONE, INDEX_NONE};
    }

    OutNodeIds.Reserve(InstanceGuids.Num());
    TMap<FGuid, int32> FirstIndexByInstanceGuid;
    TMap<FMythicHarvestNodeId, int32> FirstIndexByNodeId;
    FirstIndexByInstanceGuid.Reserve(InstanceGuids.Num());
    FirstIndexByNodeId.Reserve(InstanceGuids.Num());

    for (int32 Index = 0; Index < InstanceGuids.Num(); ++Index) {
        const FGuid &InstanceGuid = InstanceGuids[Index];
        if (!InstanceGuid.IsValid()) {
            OutNodeIds.Reset();
            return {EMythicHarvestAuthoredIdentityError::InvalidInstanceGuid,
                    Index, INDEX_NONE};
        }
        if (const int32 *FirstIndex =
                FirstIndexByInstanceGuid.Find(InstanceGuid)) {
            OutNodeIds.Reset();
            return {EMythicHarvestAuthoredIdentityError::DuplicateInstanceGuid,
                    Index, *FirstIndex};
        }

        FMythicHarvestNodeId NodeId;
        if (!TryBuildNodeId({NodeSetGuid, InstanceGuid}, NodeId)) {
            OutNodeIds.Reset();
            return {
                EMythicHarvestAuthoredIdentityError::CanonicalizationFailed,
                Index, INDEX_NONE};
        }
        if (const int32 *FirstIndex = FirstIndexByNodeId.Find(NodeId)) {
            OutNodeIds.Reset();
            return {
                EMythicHarvestAuthoredIdentityError::DuplicateStableIdentity,
                Index, *FirstIndex};
        }

        FirstIndexByInstanceGuid.Add(InstanceGuid, Index);
        FirstIndexByNodeId.Add(NodeId, Index);
        OutNodeIds.Add(NodeId);
    }
    return {};
}

bool MythicHarvestAuthoredIdentity::AppendPackedInstanceGuid(
    const FGuid &InstanceGuid, TArray<float> &InOutFloats) {
    return MythicHarvestPCGIdentity::AppendPackedNodeId(
        FMythicHarvestNodeId(InstanceGuid), InOutFloats);
}

bool MythicHarvestAuthoredIdentity::TryDecodePackedInstanceGuid(
    const TConstArrayView<float> PackedFloats, FGuid &OutInstanceGuid) {
    OutInstanceGuid.Invalidate();
    FMythicHarvestNodeId PackedGuid;
    if (!MythicHarvestPCGIdentity::TryDecodePackedNodeId(PackedFloats,
                                                         PackedGuid)) {
        return false;
    }
    OutInstanceGuid = PackedGuid.GetGuid();
    return OutInstanceGuid.IsValid();
}
