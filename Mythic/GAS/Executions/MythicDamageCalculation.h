// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MythicDamageCalculation.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicDamageCalculation : public UGameplayEffectExecutionCalculation {
    GENERATED_BODY()

public:
    UMythicDamageCalculation();

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
