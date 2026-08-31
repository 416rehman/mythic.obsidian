#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MythicMMC_StatContribution.generated.h"

/**
 * The magnitude of one primary stat's contribution to one derived attribute, read from the authored rows.
 *
 * Attach it to a MultiplyAdditive modifier and the derived attribute becomes Base * (1 + contribution), which
 * is the same additive-into-one-multiplier shape the damage path uses. Two stats feeding the same target
 * therefore sum rather than multiply, and no pair of independent terms can go quadratic.
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
