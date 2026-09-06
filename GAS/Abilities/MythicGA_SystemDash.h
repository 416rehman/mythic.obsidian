#pragma once
#include "CoreMinimal.h"
#include "GAS/Abilities/MythicGA_Skill.h"
#include "GAS/Effects/MythicCombatBuffs.h"
#include "MythicGA_SystemDash.generated.h"
class UAbilityTask_ApplyRootMotionMoveToForce;

UCLASS()
class MYTHIC_API UMythicGE_SystemDashCooldown : public UMythicBuffGameplayEffect {
    GENERATED_BODY()
public: UMythicGE_SystemDashCooldown();
};

/** A baseline action; inherited skill type exists only for the established rune movement callback ABI. */
UCLASS(NotBlueprintable)
class MYTHIC_API UMythicGA_SystemDash final : public UMythicGA_Skill {
    GENERATED_BODY()
public:
    UMythicGA_SystemDash();
    virtual bool CanActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
        const FGameplayTagContainer* SourceTags=nullptr,const FGameplayTagContainer* TargetTags=nullptr,
        FGameplayTagContainer* FailureTags=nullptr) const override;
    virtual bool CheckCost(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,FGameplayTagContainer* FailureTags=nullptr) const override;
    virtual void ActivateAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
        FGameplayAbilityActivationInfo Activation,const FGameplayEventData* Event) override;
    virtual void EndAbility(FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* Info,
        FGameplayAbilityActivationInfo Activation,bool bReplicateEndAbility,bool bWasCancelled) override;
private:
    void ReceiveIntent(const FGameplayAbilityTargetDataHandle& Data,FGameplayTag Tag);
    void StartDash(FVector Aim,FVector MovementIntent);
    UFUNCTION() void Reached();
    UFUNCTION() void Blocked();
    void ValidateMotion();
    UPROPERTY(Transient) TObjectPtr<UAbilityTask_ApplyRootMotionMoveToForce> Motion;
    TWeakObjectPtr<AActor> DashAvatar;
    FDelegateHandle IntentHandle;
    FTimerHandle IntentTimeout, MotionGuard;
    bool bEnding=false, bMovementStarted=false;
    int32 CallbackDepth=0;
};
