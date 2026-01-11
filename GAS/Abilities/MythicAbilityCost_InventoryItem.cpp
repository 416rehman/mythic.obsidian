// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicAbilityCost_InventoryItem.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySpecHandle.h"
#include "MythicGameplayAbility.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/ItemizationSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicAbilityCost_InventoryItem)

UMythicAbilityCost_InventoryItem::UMythicAbilityCost_InventoryItem() {
    Quantity.SetValue(1.0f);
}

bool UMythicAbilityCost_InventoryItem::CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo *ActorInfo, FGameplayTagContainer *OptionalRelevantTags) const {
    if (AController *PC = Ability->GetControllerFromActorInfo()) {
        if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(PC)) {
            auto Inventories = InvProvider->GetAllInventoryComponents();

            for (auto &InventoryComponent : Inventories) {
                const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

                const float NumItemsToConsumeReal = Quantity.GetValueAtLevel(AbilityLevel);
                const int32 NumItemsToConsume = FMath::TruncToInt(NumItemsToConsumeReal);

                if (InventoryComponent->GetItemCount(ItemDefinition) >= NumItemsToConsume) {
                    return true;
                }
            }
        }
    }
    return false;
}

void UMythicAbilityCost_InventoryItem::ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) {
    if (!ActorInfo->IsNetAuthority()) {
        return;
    }

    if (AController *PC = Ability->GetControllerFromActorInfo()) {
        if (auto InvProvider = Cast<IInventoryProviderInterface>(PC)) {
            auto Inventories = InvProvider->GetAllInventoryComponents();

            const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
            const float NumItemsToConsumeReal = Quantity.GetValueAtLevel(AbilityLevel);
            const int32 NumItemsToConsume = FMath::TruncToInt(NumItemsToConsumeReal);

            auto RemainingToConsume = NumItemsToConsume;
            for (auto &InventoryComponent : Inventories) {
                const int32 AvailableInThisInventory = InventoryComponent->GetItemCount(ItemDefinition);
                const int32 ToConsumeFromThisInventory = FMath::Min(AvailableInThisInventory, RemainingToConsume);
                if (ToConsumeFromThisInventory > 0) {
                    InventoryComponent->ServerRemoveItemByDefinition(ItemDefinition, ToConsumeFromThisInventory);
                    RemainingToConsume -= ToConsumeFromThisInventory;
                    if (RemainingToConsume <= 0) {
                        break;
                    }
                }
            }
        }
    }
}
