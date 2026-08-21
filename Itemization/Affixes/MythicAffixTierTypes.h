#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "MythicAffixTierTypes.generated.h"

UENUM(BlueprintType)
enum class EMythicAffixGroup : uint8 {
    Prefix,
    Suffix
};

USTRUCT(BlueprintType)
struct FMythicAffixTier {
    GENERATED_BODY()

    // Item level required for this tier to be eligible (tier is eligible when MinItemLevel <= ItemLevel).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    int32 MinItemLevel = 1;

    // Relative selection weight AMONG eligible tiers (cumulative-weight pick). Higher tiers usually carry a lower
    // weight so they remain the "lucky" roll even once eligible.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    float Weight = 1.0f;

    // Roll band (pre level-scaling).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    float Min = 0.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    float Max = 0.0f;

    // Added to Min/Max as ItemLevel * LevelScaling (same formula as FRollDefinition::ScaleValue).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    float LevelScaling = 0.0f;

    // Designer-facing quality label ("Superior", "T2", ...) surfaced on the tooltip via FRolledAffix::TierLabel.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    FText TierLabel;
};

USTRUCT(BlueprintType)
struct FMythicTieredAffixDef {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    TEnumAsByte<EGameplayModOp::Type> ModOp = EGameplayModOp::Additive;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    EMythicAffixGroup Group = EMythicAffixGroup::Prefix;

    // Matched over the item's GetTypeProbe tags. An EMPTY query applies to ALL items (see FMythicAffixTierMath::DefApplies).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    FGameplayTagQuery Applicability;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Affixes")
    TArray<FMythicAffixTier> Tiers;
};

struct FMythicAffixBudget {
    int32 PrefixCap = 0;
    int32 SuffixCap = 0;
};

struct FMythicAffixTierMath {
    static int32 WeightedPickDef(TConstArrayView<float> Weights, float Roll01) {
        const int32 N = Weights.Num();
        if (N == 0) {
            return -1;
        }
        float Total = 0.0f;
        for (const float W : Weights) {
            Total += FMath::Max(0.0f, W);
        }
        if (Total <= 0.0f) {
            return -1;
        }
        const float Target = FMath::Clamp(Roll01, 0.0f, 1.0f) * Total;
        float Cumulative = 0.0f;
        for (int32 i = 0; i < N; ++i) {
            Cumulative += FMath::Max(0.0f, Weights[i]);
            if (Target < Cumulative) {
                return i;
            }
        }
        for (int32 i = N - 1; i >= 0; --i) {
            if (Weights[i] > 0.0f) {
                return i;
            }
        }
        return -1;
    }

    static int32 SelectTierIndex(int32 ItemLevel, TConstArrayView<FMythicAffixTier> Tiers, float Roll01) {
        const int32 N = Tiers.Num();
        if (N == 0) {
            return -1;
        }
        TArray<float, TInlineAllocator<8>> EligibleWeights;
        TArray<int32, TInlineAllocator<8>> OriginalIndex;
        for (int32 i = 0; i < N; ++i) {
            if (Tiers[i].MinItemLevel <= ItemLevel) {
                EligibleWeights.Add(FMath::Max(0.0f, Tiers[i].Weight));
                OriginalIndex.Add(i);
            }
        }
        if (EligibleWeights.Num() == 0) {
            return -1;
        }
        const int32 Picked = WeightedPickDef(EligibleWeights, Roll01);
        return (Picked >= 0) ? OriginalIndex[Picked] : -1;
    }

    static float RollValueInTier(const FMythicAffixTier &Tier, int32 ItemLevel, float Roll01) {
        const float ScaledMin = Tier.Min + static_cast<float>(ItemLevel) * Tier.LevelScaling;
        const float ScaledMax = Tier.Max + static_cast<float>(ItemLevel) * Tier.LevelScaling;
        return FMath::Lerp(ScaledMin, ScaledMax, FMath::Clamp(Roll01, 0.0f, 1.0f));
    }

    static float SumEligibleTierWeight(int32 ItemLevel, TConstArrayView<FMythicAffixTier> Tiers) {
        float Sum = 0.0f;
        for (const FMythicAffixTier &T : Tiers) {
            if (T.MinItemLevel <= ItemLevel) {
                Sum += FMath::Max(0.0f, T.Weight);
            }
        }
        return Sum;
    }

    static FMythicAffixBudget ComputeAffixBudget(int32 TotalAffixCount) {
        const int32 Total = FMath::Max(0, TotalAffixCount);
        FMythicAffixBudget Budget;
        Budget.PrefixCap = (Total + 1) / 2;
        Budget.SuffixCap = Total / 2;
        return Budget;
    }

    static bool BudgetAllows(EMythicAffixGroup Group, int32 PrefixAdded, int32 SuffixAdded, const FMythicAffixBudget &Budget) {
        const int32 TotalCap = Budget.PrefixCap + Budget.SuffixCap;
        if (PrefixAdded + SuffixAdded >= TotalCap) {
            return false;
        }
        if (Group == EMythicAffixGroup::Prefix) {
            return PrefixAdded < Budget.PrefixCap;
        }
        return SuffixAdded < Budget.SuffixCap;
    }
};
