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

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
