
#pragma once

#include "CoreMinimal.h"
#include "MythicCorpseTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicDecompStage : uint8 {
    Fresh = 0,
    Bloated = 1,
    Decayed = 2,
    Skeletal = 3
};

struct FMythicCorpseRules {
    static EMythicDecompStage StageForAge(float AgeSeconds, TConstArrayView<float> StageThresholds) {
        int32 Stage = 0;
        for (int32 i = 0; i < StageThresholds.Num(); ++i) {
            if (AgeSeconds >= StageThresholds[i]) {
                ++Stage;
            }
            else {
                break;
            }
        }
        const int32 MaxStage = static_cast<int32>(EMythicDecompStage::Skeletal);
        return static_cast<EMythicDecompStage>(FMath::Clamp(Stage, 0, MaxStage));
    }

    static bool IsRaisable(EMythicDecompStage Stage, EMythicDecompStage MaxRaisableStage, bool bAlreadyRaised) {
        if (bAlreadyRaised) {
            return false;
        }
        return static_cast<uint8>(Stage) <= static_cast<uint8>(MaxRaisableStage);
    }

    static float DecayLifetimeForTier(int32 Tier, float BaseLifetime, float PerTier) {
        const int32 TiersAboveNormal = FMath::Max(0, Tier - 1);
        return BaseLifetime + static_cast<float>(TiersAboveNormal) * PerTier;
    }
};
