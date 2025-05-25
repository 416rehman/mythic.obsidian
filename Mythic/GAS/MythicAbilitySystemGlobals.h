// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemGlobals.h"
#include "MythicAbilitySystemGlobals.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicAbilitySystemGlobals : public UAbilitySystemGlobals {
    GENERATED_BODY()

    UMythicAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer);
    
    //~UAbilitySystemGlobals interface
    virtual FGameplayEffectContext* AllocGameplayEffectContext() const override;
    //~End of UAbilitySystemGlobals interface
};
