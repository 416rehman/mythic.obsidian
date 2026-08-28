#include "World/Harvesting/MythicHarvestPCGIdentity.h"

#include "Hash/Blake3.h"

namespace {

// The trailing NUL is part of the frozen domain separator. It prevents prefix extension from sharing this protocol.
constexpr uint8 CanonicalDomain[] = {
    'M', 'Y', 'T', 'H', 'I', 'C', '_', 'H', 'A', 'R', 'V', 'E', 'S', 'T', '_',
    'P', 'C', 'G', '_', 'N', 'O', 'D', 'E', '_', 'I', 'D', 0,
};

// Separate protocol domain from the final node-id hash so either canonical form can evolve independently.
constexpr uint8 PointIdentityDomain[] = {
    'M', 'Y', 'T', 'H', 'I', 'C', '_', 'H', 'A', 'R', 'V', 'E', 'S', 'T', '_',
    'P', 'C', 'G', '_', 'P', 'O', 'I', 'N', 'T', '_', 'I', 'D', 0,
};

constexpr uint32 PointIdentityVersion = 1;

void AppendUInt32BigEndian(const uint32 Value, TArray<uint8> &OutBytes) {
    OutBytes.Add(static_cast<uint8>(Value >> 24));
    OutBytes.Add(static_cast<uint8>(Value >> 16));
    OutBytes.Add(static_cast<uint8>(Value >> 8));
    OutBytes.Add(static_cast<uint8>(Value));
}

void AppendUInt64BigEndian(const uint64 Value, TArray<uint8> &OutBytes) {
    for (int32 Shift = 56; Shift >= 0; Shift -= 8) {
        OutBytes.Add(static_cast<uint8>(Value >> Shift));
    }
}

void AppendInt64BigEndian(const int64 Value, TArray<uint8> &OutBytes) {
    AppendUInt64BigEndian(static_cast<uint64>(Value), OutBytes);
}

void AppendGuidBigEndian(const FGuid &Guid, TArray<uint8> &OutBytes) {
    AppendUInt32BigEndian(Guid.A, OutBytes);
    AppendUInt32BigEndian(Guid.B, OutBytes);
    AppendUInt32BigEndian(Guid.C, OutBytes);
    AppendUInt32BigEndian(Guid.D, OutBytes);
}

uint32 ReadUInt32BigEndian(const uint8 *Bytes) {
    return (static_cast<uint32>(Bytes[0]) << 24)
        | (static_cast<uint32>(Bytes[1]) << 16)
        | (static_cast<uint32>(Bytes[2]) << 8)
        | static_cast<uint32>(Bytes[3]);
}

uint64 ReadUInt64BigEndian(const uint8 *Bytes) {
    uint64 Value = 0;
    for (int32 Index = 0; Index < 8; ++Index) {
        Value = (Value << 8) | static_cast<uint64>(Bytes[Index]);
    }
    return Value;
}

EMythicHarvestPCGIdentityError GetInputError(
    const FMythicHarvestPCGIdentityInput &Input) {
    if (!Input.ProviderGuid.IsValid()) {
        return EMythicHarvestPCGIdentityError::InvalidProviderGuid;
    }
    if (!Input.DomainGuid.IsValid()) {
        return EMythicHarvestPCGIdentityError::InvalidDomainGuid;
    }
    if (Input.PointIdentity < 0) {
        return EMythicHarvestPCGIdentityError::InvalidPointIdentity;
    }
    return EMythicHarvestPCGIdentityError::None;
}

} // namespace

bool MythicHarvestPCGIdentity::TryBuildDeterministicPointIdentity(
    const int32 NativePointSeed, const FVector &SourcePointWorldPosition,
    int64 &OutIdentity) {
    OutIdentity = INDEX_NONE;
    if (!FMath::IsFinite(SourcePointWorldPosition.X)
        || !FMath::IsFinite(SourcePointWorldPosition.Y)
        || !FMath::IsFinite(SourcePointWorldPosition.Z)) {
        return false;
    }

    const double Coordinates[] = {
        SourcePointWorldPosition.X,
        SourcePointWorldPosition.Y,
        SourcePointWorldPosition.Z,
    };
    int64 QuantizedCoordinates[UE_ARRAY_COUNT(Coordinates)];
    for (int32 Index = 0; Index < UE_ARRAY_COUNT(Coordinates); ++Index) {
        constexpr double SafeIntegerLimit = 9007199254740991.0; // Largest exact integer in an IEEE-754 double.
        if (FMath::Abs(Coordinates[Index]) > SafeIntegerLimit) {
            return false;
        }
        QuantizedCoordinates[Index] = FMath::RoundToInt64(Coordinates[Index]);
    }

    TArray<uint8> CanonicalBytes;
    CanonicalBytes.Reserve(UE_ARRAY_COUNT(PointIdentityDomain)
        + sizeof(uint32) * 2 + sizeof(int64) * 3);
    CanonicalBytes.Append(PointIdentityDomain,
                          UE_ARRAY_COUNT(PointIdentityDomain));
    AppendUInt32BigEndian(PointIdentityVersion, CanonicalBytes);
    AppendUInt32BigEndian(static_cast<uint32>(NativePointSeed),
                          CanonicalBytes);
    for (const int64 Coordinate : QuantizedCoordinates) {
        AppendInt64BigEndian(Coordinate, CanonicalBytes);
    }

    const FBlake3Hash Digest = FBlake3::HashBuffer(
        CanonicalBytes.GetData(), static_cast<uint64>(CanonicalBytes.Num()));
    const uint64 PositiveIdentity =
        ReadUInt64BigEndian(Digest.GetBytes()) & MAX_int64;
    OutIdentity = static_cast<int64>(PositiveIdentity);
    return true;
}

bool MythicHarvestPCGIdentity::BuildCanonicalBytes(
    const FMythicHarvestPCGIdentityInput &Input, TArray<uint8> &OutBytes) {
    OutBytes.Reset();
    if (!Input.IsValid()) {
        return false;
    }

    constexpr int32 CanonicalByteCount = UE_ARRAY_COUNT(CanonicalDomain)
        + sizeof(uint32) + sizeof(uint32) * 4 * 2 + sizeof(uint64);
    OutBytes.Reserve(CanonicalByteCount);
    OutBytes.Append(CanonicalDomain, UE_ARRAY_COUNT(CanonicalDomain));
    AppendUInt32BigEndian(CanonicalVersion, OutBytes);
    AppendGuidBigEndian(Input.ProviderGuid, OutBytes);
    AppendGuidBigEndian(Input.DomainGuid, OutBytes);
    AppendUInt64BigEndian(static_cast<uint64>(Input.PointIdentity), OutBytes);

    return OutBytes.Num() == CanonicalByteCount;
}

bool MythicHarvestPCGIdentity::TryBuildNodeId(
    const FMythicHarvestPCGIdentityInput &Input,
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
        ReadUInt32BigEndian(DigestBytes),
        ReadUInt32BigEndian(DigestBytes + 4),
        ReadUInt32BigEndian(DigestBytes + 8),
        ReadUInt32BigEndian(DigestBytes + 12));
    if (!Guid.IsValid()) {
        return false;
    }

    OutNodeId = FMythicHarvestNodeId(Guid);
    return true;
}

FMythicHarvestPCGIdentityValidationResult
MythicHarvestPCGIdentity::ValidateAndBuildNodeIds(
    const TConstArrayView<FMythicHarvestPCGIdentityInput> Inputs,
    TArray<FMythicHarvestNodeId> &OutNodeIds) {
    OutNodeIds.Reset();
    OutNodeIds.Reserve(Inputs.Num());

    TMap<FMythicHarvestNodeId, int32> FirstIndexByNodeId;
    FirstIndexByNodeId.Reserve(Inputs.Num());
    for (int32 Index = 0; Index < Inputs.Num(); ++Index) {
        const EMythicHarvestPCGIdentityError InputError = GetInputError(Inputs[Index]);
        if (InputError != EMythicHarvestPCGIdentityError::None) {
            OutNodeIds.Reset();
            return {InputError, Index, INDEX_NONE};
        }

        FMythicHarvestNodeId NodeId;
        if (!TryBuildNodeId(Inputs[Index], NodeId)) {
            OutNodeIds.Reset();
            return {EMythicHarvestPCGIdentityError::CanonicalizationFailed, Index,
                    INDEX_NONE};
        }

        if (const int32 *FirstIndex = FirstIndexByNodeId.Find(NodeId)) {
            OutNodeIds.Reset();
            return {EMythicHarvestPCGIdentityError::DuplicateStableIdentity, Index,
                    *FirstIndex};
        }

        FirstIndexByNodeId.Add(NodeId, Index);
        OutNodeIds.Add(NodeId);
    }

    return {};
}

bool MythicHarvestPCGIdentity::AppendPackedNodeId(
    const FMythicHarvestNodeId &NodeId, TArray<float> &InOutFloats) {
    if (!NodeId.IsValid()) {
        return false;
    }

    static_assert(65535u < (1u << 24),
                  "Every uint16 value must be exactly representable by float.");
    const FGuid &Guid = NodeId.GetGuid();
    const uint32 Words[] = {Guid.A, Guid.B, Guid.C, Guid.D};
    InOutFloats.Reserve(InOutFloats.Num() + PackedFloatCount);
    for (const uint32 Word : Words) {
        InOutFloats.Add(static_cast<float>((Word >> 16) & 0xffffu));
        InOutFloats.Add(static_cast<float>(Word & 0xffffu));
    }
    return true;
}

bool MythicHarvestPCGIdentity::TryDecodePackedNodeId(
    const TConstArrayView<float> PackedFloats,
    FMythicHarvestNodeId &OutNodeId) {
    OutNodeId = FMythicHarvestNodeId();
    if (PackedFloats.Num() != PackedFloatCount) {
        return false;
    }

    uint16 Halves[PackedFloatCount];
    for (int32 Index = 0; Index < PackedFloatCount; ++Index) {
        const float Value = PackedFloats[Index];
        if (!FMath::IsFinite(Value) || Value < 0.0f || Value > 65535.0f
            || FMath::FloorToFloat(Value) != Value) {
            return false;
        }
        Halves[Index] = static_cast<uint16>(Value);
    }

    uint32 Words[4];
    for (int32 WordIndex = 0; WordIndex < 4; ++WordIndex) {
        Words[WordIndex] = (static_cast<uint32>(Halves[WordIndex * 2]) << 16)
            | static_cast<uint32>(Halves[WordIndex * 2 + 1]);
    }

    const FGuid Guid(Words[0], Words[1], Words[2], Words[3]);
    if (!Guid.IsValid()) {
        return false;
    }

    OutNodeId = FMythicHarvestNodeId(Guid);
    return true;
}
