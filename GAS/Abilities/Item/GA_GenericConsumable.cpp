#include "GA_GenericConsumable.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
// Copyright Mythic Games. All Rights Reserved.

#include "Mythic/Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "Player/MythicPlayerController.h"

UGA_GenericConsumable::UGA_GenericConsumable(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_GenericConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    bool bSucceeded=false;
    if (ActorInfo && ActorInfo->IsNetAuthority()) if (auto* Fragment=Cast<UActionableItemFragment>(GetCurrentSourceObject())) if (auto* Item=Fragment->GetOwningItemInstance()) if (auto* Inventory=Item->GetInventoryComponent()) {
        FMythicInventorySourceLocator Source;
        Source.Inventory=Inventory; Source.SlotIndex=Item->GetSlot(); Source.ExpectedItemGuid=Item->GetItemInstanceGuid(); Source.ExpectedQuantity=Item->GetStacks();
        int32 Consumed=0;
        bSucceeded=Inventory->TryUseItemInSlot(Source,Consumed)==EMythicInventoryActionResult::Succeeded;
    }
    EndAbility(Handle,ActorInfo,ActivationInfo,true,!bSucceeded);
}
