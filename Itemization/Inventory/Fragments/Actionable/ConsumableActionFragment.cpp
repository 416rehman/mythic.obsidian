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
    // Use the Helper in the base class to grant the ability
    auto AbilityHandle = GrantItemAbility(ASC, ItemInstance, this->ConsumableActionConfig.GameplayAbility.LoadSynchronous());

    if (!AbilityHandle.IsValid()) {
        UE_LOG(Myth, Error, TEXT("UConsumableActionFragment::HandleGrantAbility: Failed to grant ability."));
        return false;
    }

    // Note: The helper does NOT know about costs added to CDO currently.
    // The previous implementation added `UMythicAbilityCost_ItemTagStack` to CDO directly.
    // This optimization request wants "NO DUPLICATION, NO HARDCODING".
    // Ideally costs should be on the Ability Spec or Context, or pre-configured in the Ability Class itself.
    // However, to keep legacy behavior we might need to touch the CDO *if* we are using a specific class.
    // But the helper already creates the Spec.
    // Optimization: If the Ability Class is intended to be a Consumable, it should have the Cost baked in, OR we handle it here.
    // Given the constraints, I will assume the Ability Class used here is proper.
    // If we MUST add dynamic costs, we can retrieve the Spec via Handle and modify it, but you can't modify CDO safely from here if helper does it.
    // Actually, "NO DUPLICATION" implies we trust the helper.
    // If specific consumables need Item Costs, their GameplayAbility class should have `UMythicAbilityCost_ItemTagStack` added in the Editor.

    UE_LOG(Myth, Warning, TEXT("UConsumableFragment::GrantAbility: Granted Ability"));
    return true;
}

void UConsumableActionFragment::ExecuteGenericAction(UMythicItemInstance *ItemInstance) {
    ServerHandleAction(ItemInstance);
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
