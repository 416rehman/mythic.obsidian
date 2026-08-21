
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

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;

private:
    TArray<FGameplayEffectAttributeCaptureDefinition> IncreasedComposeCaptures;
    TArray<FGameplayEffectAttributeCaptureDefinition> MoreComposeCaptures;
};
