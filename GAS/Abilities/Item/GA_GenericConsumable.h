// Copyright Mythic Games. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "GA_GenericConsumable.generated.h"

UCLASS()
class MYTHIC_API UGA_GenericConsumable : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UGA_GenericConsumable(const FObjectInitializer &ObjectInitializer);

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;
};
