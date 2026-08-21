
#pragma once

#include "CoreMinimal.h"
#include "MythicLivestockGenome.generated.h"

USTRUCT(BlueprintType)
struct FMythicBreedingParams {
    GENERATED_BODY()

    // Max +/- mutation applied to a blended trait per breed, in trait units [0,1]. A gentle default keeps single
    // generations incremental (selection over many generations is the payoff, not one lucky roll).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breeding|Mutation", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MutationMagnitude = 0.05f;

    // Trait value clamp range (every trait lives in [TraitMin, TraitMax]; neutral == TraitMin == 0).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breeding|Mutation", meta = (ClampMin = "0.0"))
    float TraitMin = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breeding|Mutation", meta = (ClampMin = "0.0"))
    float TraitMax = 1.0f;

    // Quality trait needed per +1 produce-TIER step: bonus = floor(Quality / this), capped at MaxTierBonus. With the
    // 0.5 default a genome must reach 0.5 Quality for +1 tier and 1.0 for +2 — a long-tail climb, not a freebie.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breeding|Quality", meta = (ClampMin = "0.01"))
    float QualityPerTierStep = 0.5f;

    // Cap on the produce-tier steps the genome can add (so Pristine stays a bred-for prize, never trivial). The final
    // tier is still clamped to the Pristine ceiling by the yield-quality enum regardless.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breeding|Quality", meta = (ClampMin = "0", ClampMax = "3"))
    int32 MaxTierBonus = 2;

    // Yield trait -> produce-RATE bonus at Yield == 1: rate multiplier = 1 + Yield * this (a bred animal produces up to
    // this fraction faster). Neutral Yield 0 -> exactly 1.0. Applied by SHRINKING the effective seconds-per-unit
    // (AccrueUnits clamps its own 0..1 pause multiplier, so rate must ride the interval, not that multiplier).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Breeding|Yield", meta = (ClampMin = "0.0"))
    float MaxYieldRateBonus = 0.5f;
};

USTRUCT(BlueprintType)
struct FMythicLivestockGenome {
    GENERATED_BODY()

    static constexpr int32 TraitYield = 0;
    static constexpr int32 TraitQuality = 1;
    static constexpr int32 NumTraits = 2;

    // Faster produce cadence (0 = baseline rate, 1 = the full MaxYieldRateBonus faster).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Yield = 0.0f;

    // Higher produce/pelt TIER (0 = feed-tier only, higher = extra tier steps — the selective-breeding headline).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Genome", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Quality = 0.0f;

    float GetTrait(int32 Index) const {
        switch (Index) {
        case TraitYield: return Yield;
        case TraitQuality: return Quality;
        default: return 0.0f;
        }
    }
    void SetTrait(int32 Index, float Value) {
        switch (Index) {
        case TraitYield: Yield = Value; break;
        case TraitQuality: Quality = Value; break;
        default: break;
        }
    }

    bool IsNeutral() const {
        for (int32 i = 0; i < NumTraits; ++i) {
            if (GetTrait(i) != 0.0f) {
                return false;
            }
        }
        return true;
    }
};

struct FMythicLivestockGenomeStatics {
    static float BlendTrait(float A, float B, float RollUnitInterval, const FMythicBreedingParams &Params) {
        const float Blend = 0.5f * (A + B);
        const float Roll = FMath::Clamp(RollUnitInterval, 0.0f, 1.0f);
        const float MutationDelta = (Roll * 2.0f - 1.0f) * FMath::Max(0.0f, Params.MutationMagnitude);
        const float Lo = FMath::Min(Params.TraitMin, Params.TraitMax);
        const float Hi = FMath::Max(Params.TraitMin, Params.TraitMax);
        return FMath::Clamp(Blend + MutationDelta, Lo, Hi);
    }

    static FMythicLivestockGenome Breed(const FMythicLivestockGenome &A, const FMythicLivestockGenome &B,
                                        TConstArrayView<float> MutationRolls, const FMythicBreedingParams &Params) {
        FMythicLivestockGenome Child;
        for (int32 i = 0; i < FMythicLivestockGenome::NumTraits; ++i) {
            const float Roll = MutationRolls.IsValidIndex(i) ? MutationRolls[i] : 0.5f;
            Child.SetTrait(i, BlendTrait(A.GetTrait(i), B.GetTrait(i), Roll, Params));
        }
        return Child;
    }

    static FMythicLivestockGenome Breed(const FMythicLivestockGenome &A, const FMythicLivestockGenome &B,
                                        float RollUnitInterval, const FMythicBreedingParams &Params) {
        float Rolls[FMythicLivestockGenome::NumTraits];
        for (int32 i = 0; i < FMythicLivestockGenome::NumTraits; ++i) {
            Rolls[i] = RollUnitInterval;
        }
        return Breed(A, B, TConstArrayView<float>(Rolls, FMythicLivestockGenome::NumTraits), Params);
    }

    static int32 ProduceTierBonusFromGenome(const FMythicLivestockGenome &Genome, const FMythicBreedingParams &Params) {
        const float PerStep = FMath::Max(0.01f, Params.QualityPerTierStep);
        const int32 Steps = FMath::FloorToInt(FMath::Max(0.0f, Genome.Quality) / PerStep);
        return FMath::Clamp(Steps, 0, FMath::Max(0, Params.MaxTierBonus));
    }

    static float ProductionRateMultiplierFromGenome(const FMythicLivestockGenome &Genome, const FMythicBreedingParams &Params) {
        return 1.0f + FMath::Max(0.0f, Genome.Yield) * FMath::Max(0.0f, Params.MaxYieldRateBonus);
    }

    static float EffectiveProduceIntervalSeconds(float BaseIntervalSeconds, const FMythicLivestockGenome &Genome,
                                                 const FMythicBreedingParams &Params) {
        const float Mult = ProductionRateMultiplierFromGenome(Genome, Params);
        if (BaseIntervalSeconds <= 0.0f || Mult <= 1.0f) {
            return BaseIntervalSeconds;
        }
        return BaseIntervalSeconds / Mult;
    }
};
