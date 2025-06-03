#include "MythicInventorySlot.h"

#include "MythicInventoryComponent.h"
#include "MythicItemInstance.h"
#include "Mythic/Mythic.h"


UMythicItemInstance *UMythicInventorySlot::GetItemInstance() const {
    return SlottedItemInstance;
}

void UMythicInventorySlot::Clear() {
    // authority check
    auto owner = this->GetOwningActor();
    checkf(owner && owner->HasAuthority(), TEXT("Only the server can clear an item from a slot"));

    if (SlottedItemInstance) {
        // if the slot is active, deactivate the item only (NOT THE SLOT)
        if (bIsActive) {
            SlottedItemInstance->OnInactiveItem();
        }

        // Set the item's reference to this slot to null
        SlottedItemInstance->SetSlot(nullptr);
    }

    // Set our slotted item to null
    SlottedItemInstance = nullptr;

    this->GetInventory()->ClientOnSlotUpdatedDelegate(this);
}

bool UMythicInventorySlot::SetItemInstance(UMythicItemInstance *NewItemInstance) {
    // If no new item is being added, clear this slot instead
    if (!NewItemInstance) {
        Clear();
        return true;
    }

    if (SlottedItemInstance) {
        UE_LOG(Mythic, Warning, TEXT("There is already an item in this slot"));
        return false;
    }

    // check if PermittedItemTypes is empty
    if (ItemTypeWhitelist.Num() > 0) {
        // check if the item type doesn't match any of the permitted item types so we fail
        if (!NewItemInstance->GetItemDefinition()->ItemType.MatchesAny(ItemTypeWhitelist)) {
            UE_LOG(Mythic, Warning, TEXT("Item type doesn't match any of the permitted item types"));
            return false;
        }
    }

    SlottedItemInstance = NewItemInstance;
    SlottedItemInstance->SetOwner(this->OwningInventory);
    if (SlottedItemInstance) {
        SlottedItemInstance->SetSlot(this);

        // if the slot is active, activate the item
        if (bIsActive) {
            ActivateSlot();
        }
    }

    this->GetInventory()->ClientOnSlotUpdatedDelegate(this);

    UE_LOG(Mythic, Warning, TEXT("Item added to slot successfully"));
    return true;
}

/*
* Slot Activation Flow - Server Side
* ------------------------------
* 1. Called by InventoryComponent when this slot becomes active
* 2. Validates server authority
* 3. Sets slot as active
* 4. If slot has an item:
*    - Validates item type whitelist
*    - Calls OnActiveItem() on the item instance
*    - This triggers server-side fragment activation
*/
void UMythicInventorySlot::ActivateSlot() {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can activate a slot"));

    // Mark this slot as active
    this->bIsActive = true;

    // if there is no item in the slot, we are done
    if (!SlottedItemInstance) {
        return;
    }

    // if the item type whitelist is not empty, check if the item type matches any of the permitted item types
    if (this->ItemTypeWhitelist.Num() > 0) {
        if (!SlottedItemInstance->GetItemDefinition()->ItemType.MatchesAny(ItemTypeWhitelist)) {
            return;
        }
    }

    // if there IS an item in the slot, activate it
    SlottedItemInstance->OnActiveItem();
}

void UMythicInventorySlot::DeactivateSlot() {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can deactivate a slot"));

    // If an item is slotted and active, deactivate it
    if (SlottedItemInstance && bIsActive) {
        SlottedItemInstance->OnInactiveItem();
    }

    this->bIsActive = false;
}

void UMythicInventorySlot::Initialize(TObjectPtr<UMythicInventoryComponent> InventoryComponent, FGameplayTag newSlotType,
                                      FGameplayTagContainer newItemTypeWhitelist) {
    // If the owning inventory is not set, set it
    if (OwningInventory == nullptr) {
        // Set the slot type and item type whitelist
        SlotType = newSlotType;
        ItemTypeWhitelist = newItemTypeWhitelist;

        // Set the owning inventory
        if (InventoryComponent != nullptr) {
            this->OwningInventory = InventoryComponent;
            this->OwningInventory->ClientOnSlotUpdatedDelegate(this);
        }
        else {
            UE_LOG(Mythic, Error, TEXT("UMythicInventorySlot::Initialize: InventoryComponent is null"));
        }
    }
}

/*
* Slot Activation Flow - Client Side
* ------------------------------
* 1. Called via ClientOnActiveSlotChangedDelegate
* 2. No authority check needed (client-side only)
* 3. If slot has an item:
*    - Calls OnClientActiveItem() on the item instance
*    - This triggers client-side fragment activation
*    - Used for visual effects, animations, etc.
*/
void UMythicInventorySlot::ClientActivateSlot() {
    if (SlottedItemInstance && SlottedItemInstance->GetItemDefinition()) {
        SlottedItemInstance->OnClientActiveItem();
    }
}

/*
* Slot Deactivation Flow - Client Side
* ------------------------------
* 1. Called via ClientOnActiveSlotChangedDelegate
* 2. No authority check needed (client-side only)
* 3. If slot has an item:
*    - Calls OnClientInactiveItem() on the item instance
*    - This triggers client-side fragment deactivation
*    - Used for cleaning up visual effects, animations, etc.
*/
void UMythicInventorySlot::ClientDeactivateSlot() {
    if (SlottedItemInstance && SlottedItemInstance->GetItemDefinition()) {
        SlottedItemInstance->OnClientInactiveItem();
    }
}
