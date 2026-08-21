#pragma once

#include "CoreMinimal.h"
#include "Containers/ArrayView.h"
#include "World/LivingWorld/LivingWorldTypes.h"

struct FMythicTradeLedgerEntry {
    int32 SettlementId = INDEX_NONE;
    uint8 GoverningFactionIndex = 0xFF;
    FMythicResourceStock Prices;
    FMythicResourceStock Reserves;
    double SampledAtSeconds = 0.0;
};

namespace MythicTradeLedger {
    inline float PriceDifferential(float FromPrice, float ToPrice) {
        return ToPrice - FromPrice;
    }

    inline float ExpectedProfit(int32 Units, float UnitBuyCost, float UnitSellPayout) {
        if (Units <= 0) {
            return 0.0f;
        }
        return static_cast<float>(Units) * (UnitSellPayout - UnitBuyCost);
    }

    inline float ComputeStaleness(double AgeSeconds, float HalfLifeSeconds) {
        if (AgeSeconds <= 0.0) {
            return 0.0f;
        }
        const float HalfLife = FMath::Max(HalfLifeSeconds, 1.0f);
        return 1.0f - FMath::Pow(0.5f, static_cast<float>(AgeSeconds) / HalfLife);
    }

    inline float ConfidenceFromStaleness(float Staleness) {
        return FMath::Clamp(1.0f - Staleness, 0.0f, 1.0f);
    }

    inline float QuantizeStalePrice(float Price, float Staleness, float MaxBandWidth = 0.5f) {
        const float S = FMath::Clamp(Staleness, 0.0f, 1.0f);
        const float Band = FMath::Max(MaxBandWidth, 0.0f) * S;
        if (Band <= KINDA_SMALL_NUMBER) {
            return Price;
        }
        const float Snapped = (FMath::FloorToFloat(Price / Band) + 0.5f) * Band;
        return FMath::Max(Snapped, 0.0f);
    }

    inline bool FindBestArbitrage(TConstArrayView<FMythicTradeLedgerEntry> Entries, EMythicResourceType Axis,
                                  int32 &OutFromIdx, int32 &OutToIdx, float &OutDifferential) {
        OutFromIdx = INDEX_NONE;
        OutToIdx = INDEX_NONE;
        OutDifferential = 0.0f;
        int32 CheapestIdx = INDEX_NONE;
        int32 DearestIdx = INDEX_NONE;
        for (int32 i = 0; i < Entries.Num(); ++i) {
            if (Entries[i].SettlementId == INDEX_NONE) {
                continue;
            }
            const float P = Entries[i].Prices.GetResource(Axis);
            if (CheapestIdx == INDEX_NONE || P < Entries[CheapestIdx].Prices.GetResource(Axis)) {
                CheapestIdx = i;
            }
            if (DearestIdx == INDEX_NONE || P > Entries[DearestIdx].Prices.GetResource(Axis)) {
                DearestIdx = i;
            }
        }
        if (CheapestIdx == INDEX_NONE || DearestIdx == INDEX_NONE || CheapestIdx == DearestIdx) {
            return false;
        }
        const float Diff = PriceDifferential(Entries[CheapestIdx].Prices.GetResource(Axis),
                                             Entries[DearestIdx].Prices.GetResource(Axis));
        if (Diff <= 0.0f) {
            return false;
        }
        OutFromIdx = CheapestIdx;
        OutToIdx = DearestIdx;
        OutDifferential = Diff;
        return true;
    }

    inline bool IsLedgerLiveForReader(bool bPOIUnlocked, bool bStandingAtLeastNeutral) {
        return bPOIUnlocked || bStandingAtLeastNeutral;
    }
}
