// Copyright Mythic Games. All Rights Reserved.

#include "GA_GenericConsumable.h"
#include "Mythic/Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"

UGA_GenericConsumable::UGA_GenericConsumable(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
}

void UGA_GenericConsumable::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                            const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (CommitAbility(Handle, ActorInfo, ActivationInfo)) {
        // Get the Fragment from Source Object (GrantItemAbility sets 'this' as SourceObject)
        if (UActionableItemFragment *Fragment = Cast<UActionableItemFragment>(GetCurrentSourceObject())) {
            // We need the ItemInstance. Assuming Fragment is owned by ItemInstance.
            if (UMythicItemInstance *ItemInstance = Cast<UMythicItemInstance>(Fragment->GetOuter())) {
                Fragment->ExecuteGenericAction(ItemInstance);
            }
            else {
                // Fallback: Try to get it from the Avator or Owner if GetOuter fails (e.g. if Fragments are not instanced properly)
                // But in Mythic, Fragments SHOULD be instanced.
                UE_LOG(Myth, Error, TEXT("GA_GenericConsumable: Fragment Outer is not MythicItemInstance! SourceObject: %s"),
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
