// 

#include "ConsumableActionFragment.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Mythic/Mythic.h"

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
        *CachedASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Inventory->GetOwner());
    }
    else {
        *CachedASC = nullptr;
    }
}

void UConsumableActionFragment::OnInstanced(UMythicItemInstance *ItemInstance) {
    Super::OnInstanced(ItemInstance);

    this->ConsumableActionRuntimeReplicatedData.ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner());
}

void UConsumableActionFragment::OnClientActionEnd(UMythicItemInstance *ItemInstance) {
    // ASC is cached by the server and replicated
    auto ASC = this->ConsumableActionRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UConsumableActionFragment::OnClientActionEnd: Invalid ASC."));
        return;
    }
    // 1. Loose tags
    ServerHandleTags(ItemInstance);

    // 2. Ability
    if (this->ConsumableActionConfig.GameplayAbility) {
        // Remove the ability
        if (this->ConsumableActionConfig.RemoveAbility) {
            ServerHandleInHandRemoveAbility(ItemInstance);
        }
        // Grant the ability
        else {
            ServerHandleGrantAbility(ItemInstance);
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
    if (auto Inventory = this->GetOwningInventoryComponent()) {
        Inventory->ServerRemoveItem(this->ParentItemInstance, 1);
    }
}

void UConsumableActionFragment::ServerHandleGrantAbility_Implementation(UMythicItemInstance *ItemInstance) {
    if (this->ConsumableActionConfig.GameplayAbility == nullptr) {
        return;
    }

    ConsumableActionRuntimeReplicatedData.ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ItemInstance->GetInventoryOwner());
    auto ASC = ConsumableActionRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::ServerGrantAbility_Implementation: OwningASC is null"));
        return;
    }

    // Give the ability to the player
    auto AbilitySpec = FGameplayAbilitySpec(this->ConsumableActionConfig.GameplayAbility.LoadSynchronous(), 1, INDEX_NONE, this);
    auto AbilityHandle = ASC->GiveAbility(AbilitySpec);

    if (AbilityHandle.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("UConsumableFragment::ServerGrantAbility_Implementation: Granted Ability"));
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::ServerGrantAbility_Implementation: Failed to grant ability"));
    }
}

void UConsumableActionFragment::ServerHandleInHandRemoveAbility_Implementation(UMythicItemInstance *ItemInstance) {
    auto ASC = ConsumableActionRuntimeReplicatedData.ASC;
    if (!ASC) {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::ServerRemoveAbility_Implementation: OwningASC is null"));
    }

    auto Spec = ASC->FindAbilitySpecFromClass(this->ConsumableActionConfig.GameplayAbility.LoadSynchronous());
    if (Spec->Handle.IsValid()) {
        ASC->ClearAbility(Spec->Handle);
        UE_LOG(Myth, Warning, TEXT("UConsumableFragment::OnInactiveItem: Removed Granted Ability"));
    }
    else {
        UE_LOG(Myth, Error, TEXT("UConsumableFragment::OnInactiveItem: Failed to remove Granted Ability"));
    }
}

void UConsumableActionFragment::ServerHandleTags_Implementation(UMythicItemInstance *ItemInstance) {
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
