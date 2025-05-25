// 


#include "MythicAbilitySystemGlobals.h"

#include "MythicGameplayEffectContext.h"

UMythicAbilitySystemGlobals::UMythicAbilitySystemGlobals(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {
}

FGameplayEffectContext *UMythicAbilitySystemGlobals::AllocGameplayEffectContext() const {
    return new FMythicGameplayEffectContext();
}
