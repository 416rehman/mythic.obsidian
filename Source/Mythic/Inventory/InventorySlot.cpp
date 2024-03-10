#include "InventorySlot.h"

#include "InventoryComponent.h"
#include "ItemInstance.h"
#include "Mythic/Mythic.h"


UItemInstance* UInventorySlot::GetItemInstance() const {
	return ItemInstance;
}

bool UInventorySlot::SetItemInstance(UItemInstance* NewItemInstance) {
	if (!NewItemInstance) {
		if (ItemInstance) {
			ItemInstance->SetSlot(nullptr);
		}
		ItemInstance = nullptr;
		return true;
	}

	if (ItemInstance) {
		UE_LOG(Mythic, Warning, TEXT("There is already an item in this slot"));
		return false;
	}

	// check if PermittedItemTypes is empty
	if (ItemTypeWhitelist.Num() > 0) {
		// check if the item type doesn't match any of the permitted item types
		if (!NewItemInstance->GetItemDefinitionObject()->ItemType.MatchesAny(ItemTypeWhitelist)) {
			UE_LOG(Mythic, Warning, TEXT("Item type doesn't match any of the permitted item types"));
			return false;
		}
	}
	
	ItemInstance = NewItemInstance;
	ItemInstance->SetOwner(this->OwningInventory);
	if (ItemInstance) {
		ItemInstance->SetSlot(this);
	}
	
	this->GetInventory()->OnSlotUpdated.Broadcast(this);

	UE_LOG(Mythic, Warning, TEXT("Item added to slot successfully"));
	return true;
}

void UInventorySlot::OnSlotActivated() {
	if (ItemInstance) {
		ItemInstance->OnActiveItem();
	}
}

void UInventorySlot::OnSlotDeactivated() {
	if (ItemInstance) {
		ItemInstance->OnInactiveItem();
	}
}

void UInventorySlot::Initialize(TObjectPtr<UInventoryComponent> InventoryComponent, FGameplayTag newSlotType, FGameplayTagContainer newItemTypeWhitelist) {
	if (OwningInventory != InventoryComponent) {
		this->OwningInventory = InventoryComponent;
		if (OwningInventory != nullptr) {
			SlotType = newSlotType;
			ItemTypeWhitelist = newItemTypeWhitelist;

			this->OwningInventory->OnSlotUpdated.Broadcast(this);
		}
		else {
			SlotType = TAG_Inventory_Slot_Default;
			ItemTypeWhitelist = FGameplayTagContainer();
		}
	}
}
