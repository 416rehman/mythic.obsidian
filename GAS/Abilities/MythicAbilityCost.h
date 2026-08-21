// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "MythicAbilityCost.generated.h"

class UMythicGameplayAbility;

UCLASS(MinimalAPI, DefaultToInstanced, EditInlineNew, Abstract)
class UMythicAbilityCost : public UObject {
    GENERATED_BODY()

public:
    UMythicAbilityCost() {}

    virtual bool CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           FGameplayTagContainer *OptionalRelevantTags) const {
        return true;
    }

    virtual void ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo) {}

    bool ShouldOnlyApplyCostOnHit() const { return bOnlyApplyCostOnHit; }

protected:
    /** If true, this cost should only be applied if this ability hits successfully */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category=Costs)
    bool bOnlyApplyCostOnHit = false;
};
