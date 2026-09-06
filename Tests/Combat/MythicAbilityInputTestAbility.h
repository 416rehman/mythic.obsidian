#pragma once
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "MythicAbilityInputTestAbility.generated.h"

/** Native probe for real GAS input dispatch; never granted by shipping content. */
UCLASS(NotBlueprintable, Transient)
class UMythicAbilityInputTestAbility : public UMythicGameplayAbility {
    GENERATED_BODY()
public:
    UMythicAbilityInputTestAbility();
    TArray<FName> Trace;
    bool bCancelDuringActivation=false;
    bool bClearAllDuringActivation=false;
    bool bAllowActivation=true;
    virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,const FGameplayTagContainer* SourceTags=nullptr,const FGameplayTagContainer* TargetTags=nullptr,FGameplayTagContainer* RelevantTags=nullptr) const override;
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) override;
    virtual void InputPressed(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation) override;
    virtual void InputReleased(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation) override;
    virtual void CancelAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayAbilityActivationInfo Activation,bool bReplicate) override;
};
