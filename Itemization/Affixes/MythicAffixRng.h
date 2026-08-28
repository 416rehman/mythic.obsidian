#pragma once

#include "CoreMinimal.h"

class MYTHIC_API FMythicAffixCanonicalWriter {
public:
    explicit FMythicAffixCanonicalWriter(const ANSICHAR *DomainWithNull);

    void AddUInt8(uint8 Value);
    void AddInt32(int32 Value);
    void AddUInt32(uint32 Value);
    void AddUInt64(uint64 Value);
    void AddGuid(const FGuid &Value);
    void AddBytes(TConstArrayView<uint8> Value);
    bool AddString(const FString &Value);
    bool AddName(FName Value) { return AddString(Value.ToString()); }
    bool AddPrimaryAssetId(const FPrimaryAssetId &Value);

    const TArray<uint8> &GetBytes() const { return Bytes; }
    bool IsValid() const { return bValid; }

private:
    TArray<uint8> Bytes;
    bool bValid = true;
};

class MYTHIC_API FMythicAffixRngV1 {
public:
    FMythicAffixRngV1(uint64 InitState, uint64 InitStream);

    uint32 NextUInt32();
    bool NextBounded64(uint64 Bound, uint64 &OutValue);
    bool PickWeightedIndex(TConstArrayView<int64> Weights, int32 &OutIndex, uint64 *OutDraw = nullptr);

private:
    uint64 State = 0;
    uint64 Increment = 1;
};

/** Frozen semantic draw lanes for deterministic affix generation. */
enum class EMythicAffixRngPurpose : uint8 {
    SliceSelection,
    RowSelection,
    TierSelection,
    Magnitude
};

struct MYTHIC_API FMythicAffixRngFactory {
    /** Derives the stable server seed used by every item-affix creation path from canonical persisted identities. */
    static bool DeriveItemSeed(const FGuid &ItemInstanceGuid, const FPrimaryAssetId &ProfileId,
                               uint64 &OutSeed);
    static bool FromCanonicalBytes(TConstArrayView<uint8> Bytes, FMythicAffixRngV1 &OutRng);
    static bool BuildSubstream(uint64 ServerSeed, int32 AlgorithmVersion,
                               const FPrimaryAssetId &StreamOwnerId,
                               const FGuid &OriginGuid, const FGuid &PoolRowGuid,
                               const FPrimaryAssetId &AffixDefinitionId, int32 TierRank,
                               int32 RollOrdinal, EMythicAffixRngPurpose Purpose,
                               FMythicAffixRngV1 &OutRng);
    static FGuid GuidFromCanonicalBytes(const ANSICHAR *DomainWithNull, TConstArrayView<uint8> Payload);
};
