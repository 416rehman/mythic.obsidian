
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Containers/ArrayView.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "MythicStatContribution.generated.h"

/**
 * One primary stat feeding one derived value, with scaling that belongs to this pairing alone.
 *
 * Power feeding weapon damage and Power feeding skill damage are two rows, not one coefficient used twice, so a
 * designer can make skills scale differently from weapons off the same stat without touching code.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatContribution {
    GENERATED_BODY()

    // The primary stat read from the character, e.g. Power or Strength.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contribution")
    FGameplayAttribute SourceStat;

    // The derived value it feeds, e.g. BaseWeaponDamage or MaxHealth.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contribution")
    FGameplayAttribute TargetAttribute;

    /**
     * Fraction contributed per point of the source stat, before diminishing. 0.01 means each point is worth +1%.
     * This is the number a designer reaches for first, so it is deliberately the simplest one in the row.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contribution", meta = (ClampMin = "0.0"))
    float PerPoint = 0.01f;

    /**
     * Where this pairing starts bending. Below it every point is worth face value, so early levels feel honest.
     * Expressed as a total bonus fraction: 1.0 means the first +100% is undiminished.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contribution", meta = (ClampMin = "0.0"))
    float SoftCapBonus = 1.0f;

    /**
     * The total bonus this pairing approaches and never reaches. **Zero means no curve at all** - the contribution
     * grows without bound, which is correct for a value that should keep pace with level forever (health) and
     * wrong for one that should not (a percentage bonus). Choosing this per row is the point of the row.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contribution", meta = (ClampMin = "0.0"))
    float CeilingBonus = 0.0f;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicStatContributionConfig {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Contribution")
    TArray<FMythicStatContribution> Contributions;

    bool IsConfigured() const { return Contributions.Num() > 0; }
};

struct FMythicStatContributionRules {
    // Whether a row can do anything. A row missing either end is authored and inert.
    static bool IsRowLive(const FMythicStatContribution &Row) {
        return Row.SourceStat.IsValid() && Row.TargetAttribute.IsValid() && Row.PerPoint > 0.0f;
    }

    // What one row contributes at a given value of its source stat, as a fraction.
    static float ResolveRow(const FMythicStatContribution &Row, float StatValue) {
        if (!IsRowLive(Row)) {
            return 0.0f;
        }
        const float Raw = FMath::Max(0.0f, StatValue) * Row.PerPoint;
        if (Row.CeilingBonus <= 0.0f) {
            return Raw;
        }
        return MythicCombat::Diminish(Raw, Row.SoftCapBonus, Row.CeilingBonus);
    }

    /**
     * Everything feeding one derived value, summed. Rows sum before nothing else, so two stats feeding the same
     * target compose additively rather than multiplying - which is what keeps total growth from going quadratic.
     */
    static float ResolveTarget(TConstArrayView<FMythicStatContribution> Rows, const FGameplayAttribute &Target,
                               TFunctionRef<float(const FGameplayAttribute &)> ReadStat) {
        float Total = 0.0f;
        for (const FMythicStatContribution &Row : Rows) {
            if (IsRowLive(Row) && Row.TargetAttribute == Target) {
                Total += ResolveRow(Row, ReadStat(Row.SourceStat));
            }
        }
        return Total;
    }

    /**
     * A base value lifted by everything feeding it: Base * (1 + sum).
     *
     * The one place this shape lives, so the damage execution and the tests that police it cannot drift.
     * Additive-into-one-multiplier is the whole point: it is what stops weapon damage (which rises with item
     * level) and a primary stat (which rises with character level) multiplying into quadratic growth.
     */
    static float ApplyToBase(TConstArrayView<FMythicStatContribution> Rows, const FGameplayAttribute &Target,
                             float BaseValue, TFunctionRef<float(const FGameplayAttribute &)> ReadStat) {
        return BaseValue * (1.0f + ResolveTarget(Rows, Target, ReadStat));
    }

    // Every derived value any row feeds, for UI enumeration and for the recompute pass.
    static void GatherTargets(TConstArrayView<FMythicStatContribution> Rows, TArray<FGameplayAttribute> &OutTargets) {
        OutTargets.Reset();
        for (const FMythicStatContribution &Row : Rows) {
            if (IsRowLive(Row)) {
                OutTargets.AddUnique(Row.TargetAttribute);
            }
        }
    }
};
