// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicAbilityCost_InventoryItem.h"
#include "GameplayAbilitySpec.h"
#include "GameplayAbilitySpecHandle.h"
#include "MythicGameplayAbility.h"
#include "NativeGameplayTags.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/InventoryProviderInterface.h"
#include "Itemization/ItemizationSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicAbilityCost_InventoryItem)

UMythicAbilityCost_InventoryItem::UMythicAbilityCost_InventoryItem() {
    Quantity.SetValue(1.0f);
    FailureTag = NOTIFY_ABILITY_ACTIVATION_FAILED_COST;
}

bool UMythicAbilityCost_InventoryItem::CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                                 const FGameplayAbilityActorInfo *ActorInfo, FGameplayTagContainer *OptionalRelevantTags) const {
    if (AController *PC = Ability->GetControllerFromActorInfo()) {
        if (IInventoryProviderInterface *InvProvider = Cast<IInventoryProviderInterface>(PC)) {
            auto Inventories = InvProvider->GetAllInventoryComponents();

            const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
            const float NumItemsToConsumeReal = Quantity.GetValueAtLevel(AbilityLevel);
            const int32 NumItemsToConsume = FMath::TruncToInt(NumItemsToConsumeReal);

            // Affordability is the SUM across all the player's inventories — matching ApplyCost (which pays summed) and
            // CraftingComponent::VerifyRequirements. The old per-inventory test wrongly blocked a cost the player could
            // actually pay when the items were split across multiple inventory components.
            int32 TotalAvailable = 0;
            for (auto &InventoryComponent : Inventories) {
                TotalAvailable += InventoryComponent->GetItemCount(ItemDefinition);
                if (TotalAvailable >= NumItemsToConsume) {
                    return true;
                }
            }
        }
    }

    // Inform other systems why activation was blocked (e.g. an "out of item" UI cue), matching the Stamina/ItemStack
    // sibling costs + the documented base contract — previously this cost gave the player zero out-of-item feedback.
    if (OptionalRelevantTags && FailureTag.IsValid()) {
        OptionalRelevantTags->AddTag(FailureTag);
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
