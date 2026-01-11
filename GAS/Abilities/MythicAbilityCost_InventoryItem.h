// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "MythicAbilityCost.h"
#include "ScalableFloat.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Templates/SubclassOf.h"

#include "MythicAbilityCost_InventoryItem.generated.h"

struct FGameplayAbilityActivationInfo;
struct FGameplayAbilitySpecHandle;

class UMythicGameplayAbility;
class UObject;
struct FGameplayAbilityActorInfo;
struct FGameplayTagContainer;

/**
 * Represents a cost that requires expending a quantity of an arbitrary inventory item
 * 
 */
UCLASS(meta=(DisplayName="Inventory Item"))
class UMythicAbilityCost_InventoryItem : public UMythicAbilityCost {
    GENERATED_BODY()

public:
    UMythicAbilityCost_InventoryItem();

    //~UMythicAbilityCost interface
    virtual bool CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           FGameplayTagContainer *OptionalRelevantTags) const override;
    virtual void ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo) override;
    //~End of UMythicAbilityCost interface

protected:
    /** How much of the item to spend (keyed on ability level) */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AbilityCost)
    FScalableFloat Quantity;

    /** Which item to consume */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=AbilityCost)
    UItemDefinition *ItemDefinition;
};
