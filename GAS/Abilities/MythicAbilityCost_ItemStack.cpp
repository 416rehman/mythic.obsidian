// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicAbilityCost_ItemStack.h"

#include "MythicGameplayAbility.h"
#include "NativeGameplayTags.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/MythicItemInstance.h"

UMythicAbilityCost_ItemTagStack::UMythicAbilityCost_ItemTagStack() {
    Quantity.SetValue(1.0f);
    FailureTag = NOTIFY_ABILITY_ACTIVATION_FAILED_COST;
}

bool UMythicAbilityCost_ItemTagStack::CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo *ActorInfo, FGameplayTagContainer *OptionalRelevantTags) const {
    if (!Ability) {
        return false;
    }

    // Cast source object to item instance
    auto ItemInstance = Cast<UMythicItemInstance>(Ability->GetCurrentSourceObject());
    if (!ItemInstance) {
        return false;
    }

    const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);

    const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
    const int32 NumStacks = FMath::TruncToInt(NumStacksReal);
    const bool bCanApplyCost = ItemInstance->GetStacks() >= NumStacks;

    // Inform other abilities why this cost cannot be applied
    if (!bCanApplyCost && OptionalRelevantTags && FailureTag.IsValid()) {
        OptionalRelevantTags->AddTag(FailureTag);
    }

    return bCanApplyCost;
}

void UMythicAbilityCost_ItemTagStack::ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle,
                                                const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo) {
    if (!ActorInfo->IsNetAuthority()) {
        return;
    }
    if (!Ability) {
        return;
    }
    auto ItemInstance = Cast<UMythicItemInstance>(Ability->GetCurrentSourceObject());
    if (!ItemInstance) {
        return;
    }

    const int32 AbilityLevel = Ability->GetAbilityLevel(Handle, ActorInfo);
    const float NumStacksReal = Quantity.GetValueAtLevel(AbilityLevel);
    const int32 NumStacks = FMath::TruncToInt(NumStacksReal);

    ItemInstance->ConsumeItem(NumStacks);
}
