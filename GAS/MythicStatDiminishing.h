#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Containers/ArrayView.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "MythicStatDiminishing.generated.h"

/**
 * The diminishing curve for one stat. Stacking below the soft cap is worth face value; past it each further point
 * buys less than the last and the total approaches the ceiling without reaching it.
 *
 * Both numbers are bonus fractions, so SoftCap 1.0 is +100% and Ceiling 4.0 caps the stat just under +400%.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatDiminishing {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Diminishing")
    FGameplayAttribute Attribute;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Diminishing", meta = (ClampMin = "0.0"))
    float SoftCapBonus = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Diminishing", meta = (ClampMin = "0.0"))
    float CeilingBonus = 4.0f;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatDiminishingConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Diminishing")
    TArray<FMythicStatDiminishing> Stats;

    /**
     * Curve for any multiplier stat not named above. A ceiling of 0 leaves unnamed stats uncurved, so adding a
     * stat to the game never silently caps it; naming it here is a deliberate act.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Diminishing", meta = (ClampMin = "0.0"))
    float DefaultSoftCapBonus = 1.0f;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Diminishing", meta = (ClampMin = "0.0"))
    float DefaultCeilingBonus = 0.0f;
};

struct FMythicStatDiminishingRules {
    // The authored curve for a stat, or the default pair when it is not named.
    static void FindCurve(const FMythicStatDiminishingConfig &Config, const FGameplayAttribute &Attribute,
                          float &OutSoftCapBonus, float &OutCeilingBonus) {
        for (const FMythicStatDiminishing &Row : Config.Stats) {
            if (Row.Attribute.IsValid() && Row.Attribute == Attribute) {
                OutSoftCapBonus = Row.SoftCapBonus;
                OutCeilingBonus = Row.CeilingBonus;
                return;
            }
        }
        OutSoftCapBonus = Config.DefaultSoftCapBonus;
        OutCeilingBonus = Config.DefaultCeilingBonus;
    }

    /**
     * A raw 0.0-based bonus stat bent by its authored curve, returned as a ready-to-multiply scale. A stat named
     * Bonus* or Increased* is a fraction where 0 means no bonus, so 0.4 comes back as 1.4.
     */
    static float ApplyToBonus(const FMythicStatDiminishingConfig &Config, const FGameplayAttribute &Attribute, float RawBonus) {
        float Soft = 0.0f;
        float Ceiling = 0.0f;
        FindCurve(Config, Attribute, Soft, Ceiling);
        const float Bonus = FMath::Max(0.0f, RawBonus);
        return 1.0f + (Ceiling > 0.0f ? MythicCombat::Diminish(Bonus, Soft, Ceiling) : Bonus);
    }

    // A raw 1.0-based stat bent by its authored curve. Ready to multiply.
    static float Apply(const FMythicStatDiminishingConfig &Config, const FGameplayAttribute &Attribute, float RawMultiplier) {
        float Soft = 0.0f;
        float Ceiling = 0.0f;
        FindCurve(Config, Attribute, Soft, Ceiling);
        return MythicCombat::DiminishMultiplier(RawMultiplier, Soft, Ceiling);
    }
};
