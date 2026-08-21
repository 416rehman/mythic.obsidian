#pragma once

#include "CoreMinimal.h"

struct FLootTierBonus {
    int32 ExtraDropCount = 0;

    float RarityMult = 1.0f;

    int32 GuaranteedMinRarity = 0;

    float FractionalDropChance = 0.0f;
};

struct FMythicLootScaling {
    static void AdjustWeightsForRarityFind(TArrayView<float> Weights, float RarityFind) {
        const int32 N = Weights.Num();
        if (N <= 1 || RarityFind <= 0.0f) {
            return;
        }
        const float Denom = static_cast<float>(N - 1);
        for (int32 i = 0; i < N; ++i) {
            const float Frac = static_cast<float>(i) / Denom;
            const float Factor = 1.0f + RarityFind * Frac;
            Weights[i] = FMath::Max(0.0f, Weights[i] * Factor);
        }
    }

    static FLootTierBonus ComputeTierLootBonus(int32 EnemyTierInt, float QuantityFind) {
        FLootTierBonus Bonus;

        const int32 TierAboveNormal = FMath::Max(0, EnemyTierInt - 1);
        Bonus.ExtraDropCount = FMath::Max(0, EnemyTierInt - 2);
        Bonus.RarityMult = 1.0f + 0.15f * static_cast<float>(TierAboveNormal);
        Bonus.GuaranteedMinRarity = (EnemyTierInt >= 5) ? 1 : 0;

        const float SafeQuantityFind = FMath::Max(0.0f, QuantityFind);
        Bonus.ExtraDropCount += FMath::FloorToInt(SafeQuantityFind);
        Bonus.FractionalDropChance = FMath::Frac(SafeQuantityFind);

        return Bonus;
    }
};
