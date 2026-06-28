// Copyright Mythic Games. All Rights Reserved.

#include "GA_GenericConsumable.h"
#include "Mythic/Mythic.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Itemization/Inventory/Fragments/ActionableItemFragment.h"
#include "Player/MythicPlayerController.h" // NotifyItemUsed -> "use N <type>" objectives

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
                // Snapshot the definition BEFORE the action — ExecuteGenericAction may consume + destroy the instance on
                // its last stack, so reading it afterward is a use-after-consume (the iter-20 gotcha). The data asset persists.
                const UItemDefinition *UsedDef = ItemInstance->GetItemDefinition();
                Fragment->ExecuteGenericAction(ItemInstance);
                // Drive "use N <type>" objectives: the item was actioned/consumed. ServerOnly ability → authoritative;
                // NotifyItemUsed re-gates on authority + a valid item type. The PC owns the ObjectiveTracker's ASC.
                if (ActorInfo && ActorInfo->PlayerController.IsValid()) {
                    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(ActorInfo->PlayerController.Get())) {
                        PC->NotifyItemUsed(UsedDef, 1);
                    }
                }
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
