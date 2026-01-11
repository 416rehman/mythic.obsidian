// 

#include "ConsumableActionFragment.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Abilities/MythicAbilityCost_ItemStack.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "System/MythicAssetManager.h"

void UConsumableActionFragment::OnItemActivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemActivated(ItemInstance);
}

void UConsumableActionFragment::OnItemDeactivated(UMythicItemInstance *ItemInstance) {
    Super::OnItemDeactivated(ItemInstance);
}

void UConsumableActionFragment::OnInventorySlotChanged(UMythicInventoryComponent *Inventory, int32 SlotIndex) {
    Super::OnInventorySlotChanged(Inventory, SlotIndex);
    auto CachedASC = &this->ConsumableActionRuntimeReplicatedData.ASC;
    if (Inventory) {
        *CachedASC = Cast<UMythicAbilitySystemComponent>(UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Inventory->GetOwner()));
    }
    else {
        *CachedASC = nullptr;
    }
}

void UConsumableActionFragment::OnInstanced(UMythicItemInstance *ItemInstance) {
    Super::OnInstanced(ItemInstance);

    this->ConsumableActionRuntimeReplicatedData.ASC = Cast<UMythicAbilitySystemComponent>(
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner()));

    // Precache the ability class (LoadAsync handles null case)
    UMythicAssetManager::LoadAsync(this, this->ConsumableActionConfig.GameplayAbility,
                                   [](TSubclassOf<UGameplayAbility>) {});
}

void UConsumableActionFragment::OnClientActionEnd(UMythicItemInstance *ItemInstance) {
    ServerHandleAction_Implementation(ItemInstance);
}

void UConsumableActionFragment::ServerHandleAction_Implementation(UMythicItemInstance *ItemInstance) {
    if (!ItemInstance) {
        UE_LOG(Myth, Error, TEXT("UConsumableActionFragment::OnClientActionEnd: Invalid ItemInstance."));
        return;
    }

    // ASC is cached by the server and replicated
    auto ASC = Cast<UMythicAbilitySystemComponent>(
        UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner()));
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UConsumableActionFragment::OnClientActionEnd: Invalid ASC."));
        return;
    }

    auto bShouldRemoveItem = true;

    // 1. Loose tags
    HandleTags(ItemInstance);

    // 2. Ability
    if (this->ConsumableActionConfig.GameplayAbility) {
        // Remove the ability
        if (this->ConsumableActionConfig.RemoveAbility) {
            HandleInHandRemoveAbility(ASC);
        }
        // Grant the ability
        else {
            if (HandleGrantAbility(ASC, ItemInstance)) {
                // If we granted an ability, don't remove the item. They handle removal internally
                bShouldRemoveItem = false;
            }
        }
    }

    // 3. If event tag is valid, trigger the event
    if (ConsumableActionConfig.EventTag.IsValid()) {
        auto Payload = FGameplayEventData();
        Payload.EventTag = ConsumableActionConfig.EventTag;
        Payload.Instigator = ItemInstance->GetInventoryOwner();
        Payload.OptionalObject = this;

        auto TriggeredAbilities = ASC->HandleGameplayEvent(ConsumableActionConfig.EventTag, &Payload);
        UE_LOG(Myth, Warning, TEXT("UConsumableFragment: Triggered %d abilities"), TriggeredAbilities);
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::Action: GameplayEvent is invalid"));
    }

    // FINALLY. Consume the item
    if (bShouldRemoveItem) {
        ItemInstance->ConsumeItem(1);
    }
}

bool UConsumableActionFragment::HandleGrantAbility(UMythicAbilitySystemComponent *ASC, UMythicItemInstance *ItemInstance) {
    if (this->ConsumableActionConfig.GameplayAbility == nullptr) {
        return false;
    }

    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::ServerGrantAbility_Implementation: ASC is null"));
        return false;
    }

    // Get the ability class - should be pre-loaded, use .Get()
    UClass *AbilityClass = this->ConsumableActionConfig.GameplayAbility.Get();
    if (!AbilityClass) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::GrantAbility: Ability class not loaded. Ensure it's pre-loaded."));
        return false;
    }

    auto AbilityCDO = Cast<UMythicGameplayAbility>(AbilityClass->GetDefaultObject());
    if (!AbilityCDO) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::GrantAbility: Ability is not a MythicGameplayAbility"));
        return false;
    }

    // This cost will consume item stacks when the ability is activated
    AbilityCDO->AdditionalCosts.AddUnique(NewObject<UMythicAbilityCost_ItemTagStack>(AbilityCDO));

    // Give the ability to the player
    auto AbilitySpec = FGameplayAbilitySpec(AbilityCDO, 1, INDEX_NONE, ItemInstance);
    auto AbilityHandle = ASC->GiveAbility(AbilitySpec);

    if (!AbilityHandle.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::GrantAbility: Failed to grant ability"));
        return false;
    }

    UE_LOG(Myth, Warning, TEXT("UConsumableFragment::GrantAbility: Granted Ability"));
    return true;
}

void UConsumableActionFragment::HandleInHandRemoveAbility(UMythicAbilitySystemComponent *ASC) {
    UClass *AbilityClass = this->ConsumableActionConfig.GameplayAbility.Get();
    if (!AbilityClass) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::HandleInHandRemoveAbility: Ability class not loaded."));
        return;
    }

    auto Spec = ASC->FindAbilitySpecFromClass(AbilityClass);
    if (Spec && Spec->Handle.IsValid()) {
        ASC->ClearAbility(Spec->Handle);
        UE_LOG(Myth, Warning, TEXT("UConsumableFragment::OnInactiveItem: Removed Granted Ability"));
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::OnInactiveItem: Failed to remove Granted Ability"));
    }
}

void UConsumableActionFragment::HandleTags(UMythicItemInstance *ItemInstance) {
    // Tags to add and remove
    auto InventoryOwner = ItemInstance->GetInventoryOwner();
    // Check authority
    if (InventoryOwner->GetLocalRole() != ROLE_Authority) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::HandleTags: Not authority"));
        return;
    }

    auto ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(InventoryOwner);
    if (ASC) {
        if (ConsumableActionConfig.TagsToAdd.IsValid()) {
            UE_LOG(Myth, Warning, TEXT("UConsumableFragment::HandleTags: Adding Tag %s to ASC %s"), *ConsumableActionConfig.TagsToAdd.ToString(),
                   *ASC->GetName());
            ASC->AddLooseGameplayTags(ConsumableActionConfig.TagsToAdd, 1, EGameplayTagReplicationState::TagAndCountToAll);
        }

        if (ConsumableActionConfig.TagsToRemove.IsValid()) {
            ASC->RemoveLooseGameplayTags(ConsumableActionConfig.TagsToRemove, 1, EGameplayTagReplicationState::TagAndCountToAll);
        }
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::HandleTags: OwningASC is null"));
    }
}

bool UConsumableActionFragment::CanBeStackedWith(const UItemFragment *Other) const {
    auto OtherConsumable = Cast<UConsumableActionFragment>(Other);
    if (!OtherConsumable) {
        return false;
    }

    return Super::CanBeStackedWith(Other) &&
        this->ConsumableActionConfig.TagsToAdd == OtherConsumable->ConsumableActionConfig.TagsToAdd &&
        this->ConsumableActionConfig.TagsToRemove == OtherConsumable->ConsumableActionConfig.TagsToRemove &&
        this->ConsumableActionConfig.GameplayAbility == OtherConsumable->ConsumableActionConfig.GameplayAbility &&
        this->ConsumableActionConfig.RemoveAbility == OtherConsumable->ConsumableActionConfig.RemoveAbility &&
        this->ConsumableActionConfig.EventTag == OtherConsumable->ConsumableActionConfig.EventTag;
}
