// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "GameplayTagContainer.h"
#include "MythicAbilityCost.h"
#include "ScalableFloat.h"
#include "MythicAbilityCost_ItemStack.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpecHandle;

class UMythicGameplayAbility;
class UObject;
struct FGameplayAbilityActorInfo;

/**
 * Represents a cost that requires expending a quantity of a stack
 * on the associated item instance
 */
UCLASS(meta=(DisplayName="Item Stack"))
class UMythicAbilityCost_ItemTagStack : public UMythicAbilityCost {
    GENERATED_BODY()

public:
    UMythicAbilityCost_ItemTagStack();

    //~UMythicAbilityCost interface
    virtual bool CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           FGameplayTagContainer *OptionalRelevantTags) const override;
    virtual void ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo) override;
    //~End of UMythicAbilityCost interface

protected:
    /** How much of the tag to spend (keyed on ability level) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
    FScalableFloat Quantity;

    /** Which tag to send back as a response if this cost cannot be applied */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
    FGameplayTag FailureTag;
};
