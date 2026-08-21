#pragma once

#include "CoreMinimal.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"

namespace MythicStallSales {
    inline float ComputeSaleChance(float ListedPrice, float FairPrice, float BaseChance, float CeilingRatio = 2.0f) {
        if (FairPrice <= 0.0f || ListedPrice <= 0.0f || BaseChance <= 0.0f) {
            return 0.0f;
        }
        const float Ceiling = FMath::Max(CeilingRatio, 1.01f);
        const float Ratio = ListedPrice / FairPrice;
        if (Ratio >= Ceiling) {
            return 0.0f;
        }
        const float Chance = BaseChance * (Ceiling - Ratio) / (Ceiling - 1.0f);
        return FMath::Clamp(Chance, 0.0f, FMath::Min(2.0f * BaseChance, 1.0f));
    }

    inline int32 RollUnitsSold(int32 StackUnits, float SaleChance, float RandomSample01) {
        return (StackUnits > 0 && SaleChance > 0.0f && RandomSample01 < SaleChance) ? 1 : 0;
    }

    inline int32 ComputeAccruedDrains(double ElapsedSeconds, float DrainIntervalSeconds, int32 MaxAccrued) {
        if (ElapsedSeconds <= 0.0 || DrainIntervalSeconds <= 0.0f || MaxAccrued <= 0) {
            return 0;
        }
        const int64 Passes = static_cast<int64>(ElapsedSeconds / static_cast<double>(DrainIntervalSeconds));
        return static_cast<int32>(FMath::Clamp<int64>(Passes, 0, MaxAccrued));
    }


    constexpr uint8 StallPayloadVersion = 1;

    inline void SerializeStallState(TArray<uint8> &OutData, int32 TillCoins, int64 LastDrainUnixTime,
                                    float ListedPriceMultiplier, const FString &OwnerPlayerKey,
                                    const TArray<uint8> &BasePayload) {
        FMemoryWriter Writer(OutData);
        uint8 Version = StallPayloadVersion;
        int32 Till = FMath::Max(TillCoins, 0);
        int64 Anchor = LastDrainUnixTime;
        float ListedMult = FMath::Max(ListedPriceMultiplier, 0.0f);
        FString OwnerKey = OwnerPlayerKey;
        int32 BaseLen = BasePayload.Num();
        Writer << Version;
        Writer << Till;
        Writer << Anchor;
        Writer << ListedMult;
        Writer << OwnerKey;
        Writer << BaseLen;
        if (BaseLen > 0) {
            Writer.Serialize(const_cast<uint8 *>(BasePayload.GetData()), BaseLen);
        }
    }

    inline bool DeserializeStallState(const TArray<uint8> &InData, int32 &OutTillCoins, int64 &OutLastDrainUnixTime,
                                      float &OutListedPriceMultiplier, FString &OutOwnerPlayerKey,
                                      TArray<uint8> &OutBasePayload) {
        OutTillCoins = 0;
        OutLastDrainUnixTime = 0;
        OutListedPriceMultiplier = 1.0f;
        OutOwnerPlayerKey.Reset();
        OutBasePayload.Reset();
        if (InData.Num() == 0) {
            return false;
        }
        FMemoryReader Reader(InData);
        uint8 Version = 0;
        Reader << Version;
        if (Version != StallPayloadVersion) {
            return false;
        }
        const int64 FixedBytes = static_cast<int64>(sizeof(int32) + sizeof(int64) + sizeof(float));
        if (Reader.TotalSize() - Reader.Tell() < FixedBytes) {
            return false;
        }
        int32 Till = 0;
        int64 Anchor = 0;
        float ListedMult = 1.0f;
        FString OwnerKey;
        int32 BaseLen = 0;
        Reader << Till;
        Reader << Anchor;
        Reader << ListedMult;
        Reader << OwnerKey;
        if (Reader.IsError() || Reader.TotalSize() - Reader.Tell() < static_cast<int64>(sizeof(int32))) {
            return false;
        }
        Reader << BaseLen;
        if (BaseLen < 0 || Reader.TotalSize() - Reader.Tell() < static_cast<int64>(BaseLen)) {
            return false;
        }
        if (BaseLen > 0) {
            OutBasePayload.SetNumUninitialized(BaseLen);
            Reader.Serialize(OutBasePayload.GetData(), BaseLen);
        }
        OutTillCoins = FMath::Max(Till, 0);
        OutLastDrainUnixTime = Anchor;
        OutListedPriceMultiplier = FMath::Max(ListedMult, 0.0f);
        OutOwnerPlayerKey = MoveTemp(OwnerKey);
        return true;
    }
}
