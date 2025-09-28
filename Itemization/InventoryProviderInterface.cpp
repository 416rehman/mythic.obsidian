#include "InventoryProviderInterface.h"

#include "GameplayTagContainer.h"
#include "Inventory/MythicInventoryComponent.h"
#include "Loot/MythicWorldItem.h"

UInventoryProviderInterface::UInventoryProviderInterface(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}


UMythicInventoryComponent *IInventoryProviderInterface::GetInventoryForItemType(const FGameplayTag &ItemType) const {
    for (UMythicInventoryComponent *Inventory : GetAllInventoryComponents()) {
        if (Inventory && Inventory->CanAcceptItemType(ItemType)) {
            return Inventory;
        }
    }

    return nullptr;
}

UMythicInventoryComponent *IInventoryProviderInterface::GetInventoryForItemDefinition(const UItemDefinition *ItemDefinition) const {
    if (ItemDefinition) {
        auto ItemType = ItemDefinition->ItemType;
        return GetInventoryForItemType(ItemType);
    }
    return nullptr;
}

UMythicInventoryComponent *IInventoryProviderInterface::GetInventoryForItemInstance(const UMythicItemInstance *ItemInstance) const {
    if (ItemInstance) {
        return GetInventoryForItemDefinition(ItemInstance->GetItemDefinition());
    }
    return nullptr;
}

UMythicInventoryComponent *IInventoryProviderInterface::GetInventoryForWorldItem(const AMythicWorldItem *WorldItem) const {
    if (WorldItem) {
        auto ItemInstance = WorldItem->ItemInstance;
        if (ItemInstance) {
            return GetInventoryForItemInstance(ItemInstance);
        }
    }
    return nullptr;
}
