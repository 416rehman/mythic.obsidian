// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MythicDamageApplication.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicDamageApplication : public UGameplayEffectExecutionCalculation {
    GENERATED_BODY()
public:
    UMythicDamageApplication();

    // Pure: should this combat hit be negated as friendly fire? True only for a DISTINCT player-vs-player hit while
    // friendly fire is disabled — i.e. both source and target are player-controlled, they are NOT the same actor
    // (so self-damage: fall damage, env hazards, self-DoT is never negated), and bFriendlyFireEnabled is false.
    // Static + no engine state so the co-op policy gate is unit-testable.
    static bool ShouldNegateFriendlyFire(bool bSourceIsPlayer, bool bTargetIsPlayer, bool bSameActor, bool bFriendlyFireEnabled);

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
