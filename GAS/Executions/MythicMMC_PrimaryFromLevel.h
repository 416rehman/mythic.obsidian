// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "MythicMMC_PrimaryFromLevel.generated.h"

/**
 * The level-growth half of a primary stat: the authored player-growth curve sampled at the character's
 * live Level, minus its level-1 value, so it composes additively with the seeded base and with gear.
 *
 * Level is captured non-snapshot from the target: levelling up moves the primary the same frame, and
 * everything derived from it (weapon damage, skill damage, health, armor) follows through the existing
 * contribution effect without another mechanism.
 */
UCLASS(Abstract)
class MYTHIC_API UMythicMMC_PrimaryFromLevel : public UGameplayModMagnitudeCalculation {
    GENERATED_BODY()

public:
    UMythicMMC_PrimaryFromLevel();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec &Spec) const override;

protected:
    /** The authored growth curve this primary rides. Each subclass answers with its settings row. */
    virtual const struct FCurveTableRowHandle *GetGrowthCurve() const PURE_VIRTUAL(UMythicMMC_PrimaryFromLevel::GetGrowthCurve, return nullptr;);

    FGameplayEffectAttributeCaptureDefinition XpDef;
    FGameplayEffectAttributeCaptureDefinition XpMaxDef;
};

UCLASS()
class MYTHIC_API UMythicMMC_PowerFromLevel : public UMythicMMC_PrimaryFromLevel {
    GENERATED_BODY()

protected:
    virtual const FCurveTableRowHandle *GetGrowthCurve() const override;
};

UCLASS()
class MYTHIC_API UMythicMMC_StrengthFromLevel : public UMythicMMC_PrimaryFromLevel {
    GENERATED_BODY()

protected:
    virtual const FCurveTableRowHandle *GetGrowthCurve() const override;
};
