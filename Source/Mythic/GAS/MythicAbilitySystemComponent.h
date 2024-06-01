#pragma once
#include "AbilitySystemComponent.h"
#include "MythicAbilitySystemComponent.generated.h"

UCLASS()
class MYTHIC_API UMythicAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable)
    void AddAttributeSet(UAttributeSet* AttributeSet);

    UFUNCTION(BlueprintCallable)
    void RemoveAttributeSet(UAttributeSet* AttributeSet);
};