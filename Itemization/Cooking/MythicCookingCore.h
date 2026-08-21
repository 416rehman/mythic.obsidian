
#pragma once

#include "CoreMinimal.h"

struct FMythicCookingCore {
    static constexpr float MaxPotency = 2.0f;

    static constexpr float PotencyStep = 0.01f;

    static float QuantizePotency(float Potency, float Step = PotencyStep) {
        if (Step <= 0.0f) {
            return Potency;
        }
        return FMath::RoundToFloat(Potency / Step) * Step;
    }

    static float FreshnessPotencyFactor(float FreshFraction, float MinFactor = 0.75f) {
        const float Min = FMath::Clamp(MinFactor, 0.0f, 1.0f);
        return FMath::Lerp(Min, 1.0f, FMath::Clamp(FreshFraction, 0.0f, 1.0f));
    }

    static float ComputePotency(float QualityPotencyMult, float FreshnessFactor, int32 CookingLevel, float PotencyPerLevel,
                                float MaxPotencyCap = MaxPotency) {
        const float Quality = FMath::Max(0.0f, QualityPotencyMult);
        const float Freshness = FMath::Max(0.0f, FreshnessFactor);
        const float LevelScale = 1.0f + static_cast<float>(FMath::Max(0, CookingLevel)) * FMath::Max(0.0f, PotencyPerLevel);
        const float Cap = FMath::Min(FMath::Max(0.0f, MaxPotencyCap), MaxPotency);
        const float Potency = FMath::Clamp(Quality * Freshness * LevelScale, 0.0f, Cap);
        return QuantizePotency(Potency);
    }

    static float PortionCritChance(int32 CookingLevel, float BaseChance, float ChancePerLevel, float MaxChance) {
        const float Chance = FMath::Max(0.0f, BaseChance) + static_cast<float>(FMath::Max(0, CookingLevel)) * FMath::Max(0.0f, ChancePerLevel);
        return FMath::Clamp(Chance, 0.0f, FMath::Clamp(MaxChance, 0.0f, 0.5f));
    }
};
