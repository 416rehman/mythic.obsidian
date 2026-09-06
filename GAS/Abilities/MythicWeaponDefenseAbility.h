#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "MythicWeaponDefenseAbility.generated.h"
class UAbilityTask_WaitInputRelease;
class UWeaponBlockFragment;
/** Canonical exact-item held guard; it never removes effects by tag or class. */
UCLASS(NotBlueprintable)
class MYTHIC_API UMythicWeaponDefenseAbility final : public UMythicGameplayAbility {
    GENERATED_BODY()
public:
    UMythicWeaponDefenseAbility();
    virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
        const FGameplayTagContainer* SourceTags=nullptr,const FGameplayTagContainer* TargetTags=nullptr,FGameplayTagContainer* FailureTags=nullptr) const override;
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
        FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) override;
    virtual void EndAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
        FGameplayAbilityActivationInfo Activation,bool bReplicateEndAbility,bool bWasCancelled) override;
private:
    const UWeaponBlockFragment* Source(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info) const;
    UFUNCTION() void Released(float HeldSeconds);
    void ValidateSource();
    FActiveGameplayEffectHandle GuardHandle;
    FTimerHandle SourceTimer;
    UPROPERTY(Transient) TObjectPtr<UAbilityTask_WaitInputRelease> ReleaseTask;
    bool bEnding=false;
    int32 CallbackDepth=0;
};
