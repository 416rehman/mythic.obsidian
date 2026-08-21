#pragma once

#include "CoreMinimal.h"

namespace MythicCargoRisk {
    inline float ComputeCargoHeat(float CargoValue, int32 DangerTier, float ValueReference = 2000.0f,
                                  int32 MinDangerTier = 2, int32 MaxDangerTier = 4) {
        if (DangerTier < MinDangerTier || CargoValue <= 0.0f) {
            return 0.0f;
        }
        const float ValueRef = FMath::Max(ValueReference, 1.0f);
        const float ValueTerm = FMath::Min(CargoValue / ValueRef, 1.0f);
        const int32 MaxTier = FMath::Max(MaxDangerTier, MinDangerTier);
        const float TierSpan = static_cast<float>(MaxTier - MinDangerTier + 1);
        const float TierTerm = static_cast<float>(FMath::Min(DangerTier, MaxTier) - MinDangerTier + 1) / TierSpan;
        return FMath::Clamp(ValueTerm * TierTerm, 0.0f, 1.0f);
    }
}
