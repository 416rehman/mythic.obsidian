
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Containers/ArrayView.h"
#include "MythicDamageCompose.generated.h"

USTRUCT(BlueprintType)
struct FMythicDamageComposeConfig {
    GENERATED_BODY()

    // Source attributes whose magnitudes are SUMMED into the additive "increased" bucket ((1 + Σ)). Each value is a
    // fraction (0.20 = +20% increased). Empty = no increased bucket.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Compose")
    TArray<FGameplayAttribute> IncreasedBucketAttributes;

    // Source attributes whose magnitudes each contribute a multiplicative "more" factor (Π(1 + value)). Each value is a
    // fraction (0.20 = +20% more). Empty = no more bucket.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Compose")
    TArray<FGameplayAttribute> MoreBucketAttributes;

    // Upper clamp on the total MULTIPLICATIVE "more" product (mirrors how armor mitigation clamps its fraction). <= 0
    // means UNCAPPED. e.g. 5.0 caps the stacked "more" multiplier at 5x no matter how many sources stack.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Damage Compose")
    float MoreStackCap = 0.0f;

    bool IsConfigured() const {
        return IncreasedBucketAttributes.Num() > 0 || MoreBucketAttributes.Num() > 0;
    }
};

struct FMythicDamageComposer {
    static float ComposeDamage(float BaseDamage, float SumIncreasedPercent, TConstArrayView<float> MoreMultipliers, float MoreStackCap) {
        const float IncreasedFactor = 1.0f + SumIncreasedPercent;

        float MoreProduct = 1.0f;
        for (const float More : MoreMultipliers) {
            MoreProduct *= (1.0f + More);
        }
        if (MoreStackCap > 0.0f) {
            MoreProduct = FMath::Min(MoreProduct, MoreStackCap);
        }

        return BaseDamage * IncreasedFactor * MoreProduct;
    }
};
