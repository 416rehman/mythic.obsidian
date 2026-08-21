
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MythicDamageApplication.generated.h"

UCLASS()
class MYTHIC_API UMythicDamageApplication : public UGameplayEffectExecutionCalculation {
    GENERATED_BODY()
public:
    UMythicDamageApplication();

    static bool ShouldNegateFriendlyFire(bool bSourceIsPlayer, bool bTargetIsPlayer, bool bSameActor, bool bFriendlyFireEnabled);

    static float ApplySkillDamageBonus(float Damage, bool bIsSkillHit, float BonusSkillDamage);

    // Buildup one landed proc contributes. Never negative: a hit must never drain the meter it is filling.
    // Overflow is proc chance stacked past certainty, spent here so it is not simply discarded.
    static float ComputeBuildupPerProc(float BasePerProc, float SourceMultiplier, float ChanceOverflow = 0.0f);

    // Armour must not reduce a hit below the chip floor, but a hit already nullified stays nullified —
    // otherwise immunity through IncomingDamageMultiplier cannot be expressed.
    static float ApplyChipFloor(float Damage, float MinChipDamage);

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
    TArray<FGameplayEffectAttributeCaptureDefinition> IncreasedComposeCaptures;
    TArray<FGameplayEffectAttributeCaptureDefinition> MoreComposeCaptures;
};
