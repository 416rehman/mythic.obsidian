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
            // Resolve the owning ItemInstance via the canonical owner pointer (single source of truth), NOT the UObject
            // outer chain — GetOwningItemInstance() returns UItemFragment::ParentItemInstance, set on both the live
            // (AddFragment) and deserialize paths, and is what every other fragment consumer reads.
            if (UMythicItemInstance *ItemInstance = Fragment->GetOwningItemInstance()) {
                Fragment->ExecuteGenericAction(ItemInstance);
            }
            else {
                // Owner pointer unset — the fragment was not instanced/owned properly (should never happen in Mythic).
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
