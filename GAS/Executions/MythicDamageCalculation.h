
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExecutionCalculation.h"
#include "MythicDamageCalculation.generated.h"

UCLASS()
class MYTHIC_API UMythicDamageCalculation : public UGameplayEffectExecutionCalculation {
    GENERATED_BODY()

public:
    UMythicDamageCalculation();
    static void ResolveActionIntents(struct FMythicGameplayEffectContext& Context,const FGameplayTagContainer* TargetTags);
    void CaptureActionAttributes(const UAbilitySystemComponent* ASC,struct FMythicAbilityActionSource& Out) const;
    bool IsSourceAttribute(const FGameplayAttribute& Attribute) const;

protected:
    virtual void Execute_Implementation(const FGameplayEffectCustomExecutionParameters& ExecutionParams, FGameplayEffectCustomExecutionOutput& OutExecutionOutput) const override;
};
