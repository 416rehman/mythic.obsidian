#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MythicMMC_StatContribution.generated.h"

/**
 * The magnitude of one primary stat's contribution to one derived attribute, read from the authored rows.
 *
 * Attach it to a MultiplyAdditive modifier. The calculation returns the complete factor (1 + contribution), so
 * the GAS aggregator receives the neutral value 1.0 rather than a bonus fraction that could erase the base value.
 * Two stats feeding the same target therefore sum inside one factor rather than multiplying into quadratic growth.
 *
 * TargetAttribute is a property rather than a hardcoded attribute so a designer covers a new derived value by
 * making a Blueprint child of this and pointing it somewhere new - no C++ change, which is the whole
 * requirement behind the primary stat model.
 */
UCLASS(Blueprintable)
class MYTHIC_API UMythicMMC_StatContribution : public UGameplayModMagnitudeCalculation {
    GENERATED_BODY()

public:
    UMythicMMC_StatContribution();

    /** Converts a resolved bonus fraction into GAS's complete multiplicative factor, with a neutral invalid fallback. */
    static float MakeMultiplicativeFactor(float Contribution);

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec &Spec) const override;

protected:
    /** Which derived value this instance feeds. Rows whose TargetAttribute differs are ignored. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic|Stats")
    FGameplayAttribute TargetAttribute;

private:
    FGameplayEffectAttributeCaptureDefinition PowerDef;
    FGameplayEffectAttributeCaptureDefinition StrengthDef;
    FGameplayEffectAttributeCaptureDefinition ResolveDef;
};
