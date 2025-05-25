// 

#pragma once

#include "CoreMinimal.h"
#include "MythicDamageContainer.h"
#include "Abilities/GameplayAbility.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "MythicGameplayAbility.generated.h"

class AMythicPlayerController;
/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicGameplayAbility : public UGameplayAbility {
    GENERATED_BODY()

private:
    void SendEvent(FGameplayAbilityTargetDataHandle TargetData, FGameplayEffectContextHandle EffectContextHandle, FGameplayTag EventTag);

public:
    // Whether this ability is a passive one (i.e. it is always active), it will be automatically activated when granted
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "MythicAbility")
    bool bIsPassive = false;

    // Override OnAvatarSet
    virtual void OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) override;

    /** Creates gameplay effect container spec to be applied later via ApplyDamageContainerSpec */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility", meta=(AutoCreateRefTerm = "EventData"))
    virtual FMythicDamageContainerSpec MakeDamageContainerSpec(const FMythicDamageContainer &Container, int32 OverrideGameplayLevel = -1);

    /** Applies a gameplay effect container spec that was previously created */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility")
    virtual TArray<FActiveGameplayEffectHandle> ApplyDamageContainerSpec(const FMythicDamageContainerSpec &ContainerSpec);

    /** Applies a gameplay effect container, by creating and applying the spec */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility", meta = (AutoCreateRefTerm = "EventData"))
    virtual TArray<FActiveGameplayEffectHandle> ApplyDamageContainer(const FMythicDamageContainer &Container, const TArray<FHitResult> &HitResults,
                                                                     const TArray<AActor *> &TargetActors, int32 OverrideGameplayLevel = -1);
    /** Add targets to a damage container spec */
    UFUNCTION(BlueprintCallable, Category = "MythicAbility")
    virtual void AddTargetsToDamageContainerSpec(UPARAM(ref) FMythicDamageContainerSpec &ContainerSpec, const TArray<FHitResult> &HitResults,
                                                 const TArray<AActor *> &TargetActors);
};
