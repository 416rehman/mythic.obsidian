#include "Itemization/Affixes/MythicAffixRng.h"

#include "Itemization/Affixes/MythicItemizationHash.h"

namespace {
constexpr uint64 PcgMultiplier = 6364136223846793005ULL;

void AppendUInt32LE(TArray<uint8> &Bytes, uint32 Value) {
    Bytes.Add(static_cast<uint8>(Value));
    Bytes.Add(static_cast<uint8>(Value >> 8));
    Bytes.Add(static_cast<uint8>(Value >> 16));
    Bytes.Add(static_cast<uint8>(Value >> 24));
}

void AppendUInt64LE(TArray<uint8> &Bytes, uint64 Value) {
    AppendUInt32LE(Bytes, static_cast<uint32>(Value));
    AppendUInt32LE(Bytes, static_cast<uint32>(Value >> 32));
}

uint32 ReadUInt32LE(const uint8 *Bytes) {
    return static_cast<uint32>(Bytes[0]) | (static_cast<uint32>(Bytes[1]) << 8)
        | (static_cast<uint32>(Bytes[2]) << 16) | (static_cast<uint32>(Bytes[3]) << 24);
}

uint64 ReadUInt64LE(const uint8 *Bytes) {
    return static_cast<uint64>(ReadUInt32LE(Bytes)) | (static_cast<uint64>(ReadUInt32LE(Bytes + 4)) << 32);
}

}

FMythicAffixCanonicalWriter::FMythicAffixCanonicalWriter(const ANSICHAR *DomainWithNull) {
    if (!DomainWithNull) {
        bValid = false;
        return;
    }
    const int32 LengthWithNull = FCStringAnsi::Strlen(DomainWithNull) + 1;
    Bytes.Append(reinterpret_cast<const uint8 *>(DomainWithNull), LengthWithNull);
}

void FMythicAffixCanonicalWriter::AddUInt8(uint8 Value) { Bytes.Add(Value); }
void FMythicAffixCanonicalWriter::AddInt32(int32 Value) { AppendUInt32LE(Bytes, static_cast<uint32>(Value)); }
void FMythicAffixCanonicalWriter::AddUInt32(uint32 Value) { AppendUInt32LE(Bytes, Value); }
void FMythicAffixCanonicalWriter::AddUInt64(uint64 Value) { AppendUInt64LE(Bytes, Value); }
void FMythicAffixCanonicalWriter::AddGuid(const FGuid &Value) {
    AddUInt32(Value.A); AddUInt32(Value.B); AddUInt32(Value.C); AddUInt32(Value.D);
}
void FMythicAffixCanonicalWriter::AddBytes(TConstArrayView<uint8> Value) {
    if (Value.Num() < 0 || static_cast<uint64>(Value.Num()) > MAX_uint32) {
        bValid = false;
        return;
    }
    AddUInt32(static_cast<uint32>(Value.Num()));
    Bytes.Append(Value.GetData(), Value.Num());
}

bool FMythicAffixCanonicalWriter::AddString(const FString &Value) {
    FString Canonical = Value;
    Canonical.ReplaceInline(TEXT("\\"), TEXT("/"), ESearchCase::CaseSensitive);
    Canonical.ToLowerInline();
    const FTCHARToUTF8 Utf8(*Canonical);
    if (Utf8.Length() < 0 || static_cast<uint64>(Utf8.Length()) > MAX_uint32) {
        bValid = false;
        return false;
    }
    AddUInt32(static_cast<uint32>(Utf8.Length()));
    Bytes.Append(reinterpret_cast<const uint8 *>(Utf8.Get()), Utf8.Length());
    return true;
}

bool FMythicAffixCanonicalWriter::AddPrimaryAssetId(const FPrimaryAssetId &Value) {
    return AddName(Value.PrimaryAssetType.GetName()) && AddName(Value.PrimaryAssetName);
}

FMythicAffixRngV1::FMythicAffixRngV1(uint64 InitState, uint64 InitStream) {
    State = 0;
    Increment = (InitStream << 1u) | 1u;
    NextUInt32();
    State += InitState;
    NextUInt32();
}

uint32 FMythicAffixRngV1::NextUInt32() {
    const uint64 OldState = State;
    State = OldState * PcgMultiplier + Increment;
    const uint32 XorShifted = static_cast<uint32>(((OldState >> 18u) ^ OldState) >> 27u);
    const uint32 Rotate = static_cast<uint32>(OldState >> 59u);
    return (XorShifted >> Rotate) | (XorShifted << ((32u - Rotate) & 31u));
}

bool FMythicAffixRngV1::NextBounded64(uint64 Bound, uint64 &OutValue) {
    if (Bound == 0) return false;
    const uint64 Threshold = (~Bound + 1ULL) % Bound;
    for (;;) {
        const uint64 Draw = (static_cast<uint64>(NextUInt32()) << 32) | NextUInt32();
        if (Draw >= Threshold) {
            OutValue = Draw % Bound;
            return true;
        }
    }
}

bool FMythicAffixRngV1::PickWeightedIndex(TConstArrayView<int64> Weights, int32 &OutIndex, uint64 *OutDraw) {
    OutIndex = INDEX_NONE;
    uint64 Total = 0;
    for (int64 Weight : Weights) {
        if (Weight <= 0 || static_cast<uint64>(Weight) > MAX_uint64 - Total) return false;
        Total += static_cast<uint64>(Weight);
    }
    uint64 Draw = 0;
    if (!NextBounded64(Total, Draw)) return false;
    if (OutDraw) *OutDraw = Draw;
    uint64 Cumulative = 0;
    for (int32 Index = 0; Index < Weights.Num(); ++Index) {
        Cumulative += static_cast<uint64>(Weights[Index]);
        if (Draw < Cumulative) {
            OutIndex = Index;
            return true;
        }
    }
    return false;
}

bool FMythicAffixRngFactory::DeriveItemSeed(const FGuid &ItemInstanceGuid,
                                            const FPrimaryAssetId &ProfileId,
                                            uint64 &OutSeed) {
    OutSeed = 0;
    if (!ItemInstanceGuid.IsValid() || !ProfileId.IsValid()) return false;
    FMythicAffixCanonicalWriter Writer("MYTHIC_AFFIX_ITEM_SEED_V1");
    Writer.AddGuid(ItemInstanceGuid);
    Writer.AddPrimaryAssetId(ProfileId);
    FMythicAffixRngV1 Rng(0, 0);
    if (!Writer.IsValid() || !FromCanonicalBytes(Writer.GetBytes(), Rng)) return false;
    OutSeed = (static_cast<uint64>(Rng.NextUInt32()) << 32) | Rng.NextUInt32();
    return true;
}

bool FMythicAffixRngFactory::FromCanonicalBytes(TConstArrayView<uint8> Bytes, FMythicAffixRngV1 &OutRng) {
    FSHA256Signature Digest{};
    if (!MythicItemizationHash::Sha256(Bytes, Digest)) return false;
    OutRng = FMythicAffixRngV1(ReadUInt64LE(Digest.Signature), ReadUInt64LE(Digest.Signature + 8));
    return true;
}

bool FMythicAffixRngFactory::BuildSubstream(uint64 ServerSeed, int32 AlgorithmVersion,
                                            const FPrimaryAssetId &StreamOwnerId,
                                            const FGuid &OriginGuid, const FGuid &PoolRowGuid,
                                            const FPrimaryAssetId &AffixDefinitionId,
                                            int32 TierRank, int32 RollOrdinal,
                                            EMythicAffixRngPurpose Purpose,
                                            FMythicAffixRngV1 &OutRng) {
    if (AlgorithmVersion != 1 || !StreamOwnerId.IsValid() || !AffixDefinitionId.IsValid()
        || RollOrdinal < 0 || TierRank < 0) return false;
    FMythicAffixCanonicalWriter Writer("MYTHIC_AFFIX_RNG_V2");
    Writer.AddUInt64(ServerSeed);
    Writer.AddInt32(AlgorithmVersion);
    Writer.AddPrimaryAssetId(StreamOwnerId);
    Writer.AddGuid(OriginGuid);
    Writer.AddGuid(PoolRowGuid);
    Writer.AddPrimaryAssetId(AffixDefinitionId);
    Writer.AddInt32(TierRank);
    Writer.AddInt32(RollOrdinal);
    Writer.AddUInt8(static_cast<uint8>(Purpose));
    return Writer.IsValid() && FromCanonicalBytes(Writer.GetBytes(), OutRng);
}

FGuid FMythicAffixRngFactory::GuidFromCanonicalBytes(const ANSICHAR *DomainWithNull,
                                                      TConstArrayView<uint8> Payload) {
    FMythicAffixCanonicalWriter Writer(DomainWithNull);
    TArray<uint8> Bytes = Writer.GetBytes();
    Bytes.Append(Payload.GetData(), Payload.Num());
    FSHA256Signature Digest{};
    if (!Writer.IsValid() || !MythicItemizationHash::Sha256(Bytes, Digest)) return FGuid();
    uint8 GuidBytes[16];
    FMemory::Memcpy(GuidBytes, Digest.Signature, sizeof(GuidBytes));
    GuidBytes[6] = static_cast<uint8>((GuidBytes[6] & 0x0f) | 0x50);
    GuidBytes[8] = static_cast<uint8>((GuidBytes[8] & 0x3f) | 0x80);
    return FGuid(ReadUInt32LE(GuidBytes), ReadUInt32LE(GuidBytes + 4),
                 ReadUInt32LE(GuidBytes + 8), ReadUInt32LE(GuidBytes + 12));
}
