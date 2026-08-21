// Copyright Mythic Games. All Rights Reserved.

#include "GA_GenericConsumable.h"
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

    if (CommitAbility(Handle, ActorInfo, ActivationInfo)) {
        if (UActionableItemFragment *Fragment = Cast<UActionableItemFragment>(GetCurrentSourceObject())) {
            if (UMythicItemInstance *ItemInstance = Fragment->GetOwningItemInstance()) {
                const UItemDefinition *UsedDef = ItemInstance->GetItemDefinition();
                Fragment->ExecuteGenericAction(ItemInstance);
                if (ActorInfo && ActorInfo->PlayerController.IsValid()) {
                    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(ActorInfo->PlayerController.Get())) {
                        PC->NotifyItemUsed(UsedDef, 1);
                    }
                }
            }
            else {
                UE_LOG(Myth, Error, TEXT("GA_GenericConsumable: Fragment has no owning MythicItemInstance! SourceObject: %s"),
                       *GetNameSafe(GetCurrentSourceObject()));
            }
        }
        else {
            UE_LOG(Myth, Error, TEXT("GA_GenericConsumable: Ability SourceObject is NOT a UActionableItemFragment! Cast Failed. SourceObject: %s"),
                   *GetNameSafe(GetCurrentSourceObject()));
        }

        EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
    }
    else {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
    }
}
