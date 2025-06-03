// 


#include "MythicInventoryComponent.h"

#include "MVVMGameSubsystem.h"
#include "MVVMSubsystem.h"
#include "MythicInventoryComponent_ViewModel.h"
#include "MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Mythic/Itemization/Loot/MythicLootManagerSubsystem.h"

UMythicInventoryComponent::UMythicInventoryComponent(const FObjectInitializer &OI):
    Super(OI) {
    SetIsReplicatedByDefault(true);
    SetIsReplicated(true);
    this->bReplicateUsingRegisteredSubObjectList = true;
}

void UMythicInventoryComponent::ClientSetupViewModel_Implementation() {
    if (this->ViewModelIdentifier.IsNone()) {
        UE_LOG(Mythic, Error, TEXT("Inventory Component Begin Play:: ViewModelIdentifier is empty"));
        return;
    }

    // Add the view model to the view model subsystem
    auto ViewModelSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UMVVMGameSubsystem>();
    auto collection = ViewModelSubsystem->GetViewModelCollection();

    // Create the view model
    this->ViewModel = NewObject<UMythicInventoryComponent_ViewModel>(GetOwner());
    this->ViewModel->Initialize(ActiveSlotIndex, InitialSize, Slots);
    FMVVMViewModelContext ViewModelContext;
    ViewModelContext.ContextClass = this->ViewModel->GetClass();
    ViewModelContext.ContextName = this->ViewModelIdentifier;

    if (collection->AddViewModelInstance(ViewModelContext, this->ViewModel)) {
        UE_LOG(Mythic, Warning, TEXT("Inventory Component Begin Play:: Added ViewModel %s to ViewModelCollection on Role: %s"),
               *this->ViewModelIdentifier.ToString(), *FString::FromInt(GetOwnerRole()));

        OnViewModelCreated.Broadcast();
    }
}

void UMythicInventoryComponent::BeginPlay() {
    Super::BeginPlay();

    // Use the resize function to create the initial slots
    if (GetOwner()->HasAuthority()) {
        UE_LOG(Mythic, Warning, TEXT("Inventory Component Begin Play:: Has Authority"));
        Resize(InitialSize);

        // Make the initial item activated
        if (Slots.IsValidIndex(ActiveSlotIndex)) {
            Slots[ActiveSlotIndex]->ActivateSlot();
        }
    }
    // We need to setup the view model for both client and server
    ClientSetupViewModel();
}

/*
* Item Activation Flow - Server Side
* ------------------------------
* 1. User/Game calls SetActiveSlotIndex
* 2. Server validates and processes the change
* 3. Server activates new slot and deactivates old slot
* 4. Server notifies clients via ClientOnActiveSlotChangedDelegate
*/
void UMythicInventoryComponent::SetActiveSlotIndex(int32 NewIndex) {
    checkf(GetOwner()->HasAuthority(), TEXT("SetActiveSlotIndex:: Called without Authority!"));

    if (Slots.IsValidIndex(NewIndex) && NewIndex != ActiveSlotIndex) {
        auto OldIndex = ActiveSlotIndex;
        Slots[ActiveSlotIndex]->DeactivateSlot();
        ActiveSlotIndex = NewIndex;
        Slots[ActiveSlotIndex]->ActivateSlot();

        // Client RPC
        ClientOnActiveSlotChangedDelegate(ActiveSlotIndex, OldIndex);
    }
}

int32 UMythicInventoryComponent::GetActiveSlotIndex() const {
    return ActiveSlotIndex;
}

FGameplayTag UMythicInventoryComponent::GetSlotType() const {
    if (SlotTypeRow.IsNull()) {
        UE_LOG(Mythic, Error, TEXT("UMythicInventoryComponent::GetSlotType:: SlotTypeRow is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTag();
    }
    const FSlotType *SlotType = SlotTypeRow.GetRow<FSlotType>(SlotTypeRow.RowName.ToString());
    if (!SlotType) {
        UE_LOG(Mythic, Error, TEXT("UMythicInventoryComponent::GetSlotType:: SlotType is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTag();
    }
    return SlotType->SlotType;
}

FGameplayTagContainer UMythicInventoryComponent::GetSlotWhitelistedTypes() const {
    if (SlotTypeRow.IsNull()) {
        UE_LOG(Mythic, Error, TEXT("UMythicInventoryComponent::GetSlotWhitelistedTypes:: SlotTypeRow is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTagContainer();
    }

    const FSlotType *SlotType = SlotTypeRow.GetRow<FSlotType>(SlotTypeRow.RowName.ToString());
    if (!SlotType) {
        UE_LOG(Mythic, Error, TEXT("UMythicInventoryComponent::GetSlotWhitelistedTypes:: SlotType is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTagContainer();
    }
    return SlotType->WhitelistedItemTypes;
}

bool UMythicInventoryComponent::DestroySlot(UMythicInventorySlot *InSlot) {
    if (InSlot) {
        AActor *lOwner = GetOwner();
        checkf(lOwner != nullptr, TEXT("Invalid Inventory Owner"));
        checkf(lOwner->HasAuthority(), TEXT("Called without Authority!"));
        const int32 RemovedIndex = Slots.Remove(InSlot);
        if (RemovedIndex != INDEX_NONE) {
            if (InSlot->SlottedItemInstance) {
                InSlot->SlottedItemInstance->Destroy();
            }

            InSlot->Destroy();
            return true;
        }
    }

    return false;
}

void UMythicInventoryComponent::DestroyAllSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("DestroySlot:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("DestroySlot:: Called without Authority!"));
    for (int i = 0; i < Slots.Num(); ++i) {
        if (IsValid(Slots[i])) {
            DestroySlot(Slots[i]);
        }
    }

    Slots.Empty();
}

int32 UMythicInventoryComponent::GetSlotCount() const {
    return Slots.Num();
}

void UMythicInventoryComponent::Resize(int32 NewSize) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("Resize:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("Resize:: Called without Authority!"));
    checkf(NewSize >= 0, TEXT("Resize:: Invalid Size!"));

    const int32 CurrentSize = Slots.Num();
    auto slotType = GetSlotType();
    auto slotWhitelistedTypes = GetSlotWhitelistedTypes();
    if (NewSize > CurrentSize) {
        // Add new slots
        for (int32 i = CurrentSize; i < NewSize; ++i) {
            // Create a slot with this as its outer (when this component is garbage collected, the slot will be too)
            UMythicInventorySlot *NewSlot = NewObject<UMythicInventorySlot>(this->GetOwner());
            NewSlot->SetOwner(this);
            NewSlot->Initialize(this, slotType, slotWhitelistedTypes);

            Slots.Add(NewSlot);
        }
    }
    else if (NewSize < CurrentSize) {
        // Remove slots
        for (int32 i = CurrentSize - 1; i >= NewSize; --i) {
            DestroySlot(Slots[i]);
        }
    }

    UE_LOG(Mythic, Warning, TEXT("Resized Inventory from %d to %d"), CurrentSize, NewSize);

    // Client RPC
    ClientOnInventorySizeChangedDelegate(NewSize, CurrentSize);
}

UMythicInventorySlot *UMythicInventoryComponent::GetSlot(int32 SlotIndex) {
    if (Slots.IsValidIndex(SlotIndex)) {
        return Slots[SlotIndex];
    }

    return nullptr;
}

bool UMythicInventoryComponent::TryTransferToSlot(UMythicItemInstance *ItemInstance, UMythicInventorySlot *TargetSlot) {
    auto OldItemSlot = ItemInstance->GetSlot();

    // remove from old slot
    if (OldItemSlot) {
        ItemInstance->GetSlot()->SetItemInstance(nullptr);
    }
    if (TargetSlot->SetItemInstance(ItemInstance)) {
        return true;
    }

    if (OldItemSlot) {
        // if we failed to add the item to the new slot, put it back in the old one
        UE_LOG(Mythic, Warning, TEXT("Failed to add item to slot, putting it back in the old one"));
        OldItemSlot->SetItemInstance(ItemInstance);
    }

    return false;
}

AMythicWorldItem *UMythicInventoryComponent::AddItem(UMythicItemInstance *ItemInstance, AController *TargetRecipient) {
    auto OriginalQty = ItemInstance->GetStacks();
    auto AmountAdded = AddToAnySlot(ItemInstance);

    if (AmountAdded != OriginalQty) {
        // Create a world item and return it
        UMythicLootManagerSubsystem *LootManager = GetOwner()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
        // If the ItemOwner is a player controller, use the player controller's pawn's location
        AMythicWorldItem *WorldItem = LootManager->Spawn(ItemInstance, GetOwner()->GetActorLocation(), 100, TargetRecipient);
        if (WorldItem) {
            return WorldItem;
        }
    }

    return nullptr;
}

int32 UMythicInventoryComponent::AddToAnySlot(UMythicItemInstance *ItemInstance) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    const int32 original_qty = ItemInstance->GetStacks();

    // Can be stacked?
    if (ItemInstance->GetItemDefinition()->StackSizeMax > ItemInstance->GetStacks()) {
        // Add to existing stack
        for (int32 i = 0; i < Slots.Num(); ++i) {
            auto ItemInSlot = Slots[i]->GetItemInstance();
            // Check if the item can be stacked with the existing item
            if (ItemInSlot != nullptr && ItemInSlot->GetItemDefinition() == ItemInstance->GetItemDefinition() && ItemInstance->isStackableWith(ItemInSlot)) {
                // Update existing stack
                const int32 availableSpace = ItemInSlot->GetItemDefinition()->StackSizeMax - ItemInSlot->GetStacks();
                const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

                ItemInSlot->SetStackSize(ItemInSlot->GetStacks() + QuantityToAdd);
                ItemInstance->SetStackSize(ItemInstance->GetStacks() - QuantityToAdd);

                // Destroy the item if it's remaining quantity is 0 and return the amount added
                if (ItemInstance->GetStacks() == 0) {
                    const int32 amountAdded = original_qty - ItemInstance->GetStacks();

                    ItemInstance->Destroy();
                    return amountAdded;
                }
            }
        }
    }

    // Find the first empty slot and add the rest of the item there
    for (int32 i = 0; i < Slots.Num(); ++i) {
        if (Slots[i]->GetItemInstance() == nullptr) {
            if (TryTransferToSlot(ItemInstance, Slots[i])) {
                return original_qty; // The entire item was moved.
            }
        }
    }

    return original_qty - ItemInstance->GetStacks();
}

int32 UMythicInventoryComponent::AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex) {
    if (!ItemInstance) {
        UE_LOG(Mythic, Warning, TEXT("AddToSlot: ItemInstance is null"));
        return 0;
    }
    const int32 OriginalQty = ItemInstance->GetStacks();
    if (Slots.IsValidIndex(SlotIndex)) {
        auto SlottedItem = Slots[SlotIndex]->SlottedItemInstance;
        // If the target slot is empty, move the entire item there
        if (SlottedItem == nullptr) {
            if (TryTransferToSlot(ItemInstance, Slots[SlotIndex])) {
                return OriginalQty;
            }
        }
        // If there is an item in the slot and it is stackable with the item being added, add as much as possible
        else if (SlottedItem->GetItemDefinition() == ItemInstance->GetItemDefinition() && SlottedItem->isStackableWith(ItemInstance)) {
            const int32 availableSpace = SlottedItem->GetItemDefinition()->StackSizeMax - SlottedItem->GetStacks();
            const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

            SlottedItem->SetStackSize(SlottedItem->GetStacks() + QuantityToAdd);
            ItemInstance->SetStackSize(ItemInstance->GetStacks() - QuantityToAdd);

            if (ItemInstance->GetStacks() == 0) { // If there are no more stacks left, destroy the item and return the amount added
                ItemInstance->Destroy();
                return OriginalQty;
            }
        }
    }

    return OriginalQty - ItemInstance->GetStacks();
}

int32 UMythicInventoryComponent::ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, UMythicInventorySlot *TargetSlot) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    if (TargetSlot == nullptr) {
        return AddToAnySlot(ItemInstance);
    }
    else {
        const int32 TargetSlotIndex = Slots.Find(TargetSlot);
        if (TargetSlotIndex == INDEX_NONE) {
            // log error
            UE_LOG(Mythic, Error, TEXT("ReceiveItem:: TargetSlot not found in Slots!"));
            return 0;
        }
        return AddToSlot(ItemInstance, TargetSlotIndex);
    }
}

int32 UMythicInventoryComponent::SendItem(UMythicInventorySlot *ItemSlot, UMythicInventoryComponent *TargetInventory,
                                          UMythicInventorySlot *TargetSlot) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(TargetInventory != nullptr, TEXT("SendItem:: Invalid TargetInventory!"));
    checkf(ItemSlot != nullptr, TEXT("SendItem:: Invalid ItemSlot!"));
    checkf(ItemSlot->GetItemInstance() != nullptr, TEXT("SendItem:: Invalid ItemInstance!"));

    // If same slot, return 0
    if (ItemSlot == TargetSlot) {
        return ItemSlot->GetItemInstance()->GetStacks();
    }

    // ItemInstance could be destroyed by the ReceiveItem function if it is fully consumed so we need to store the quantity before calling it.
    int32 amountSent = TargetInventory->ReceiveItem(ItemSlot->SlottedItemInstance, TargetSlot);

    return amountSent;
}

bool UMythicInventoryComponent::DropItem(UMythicInventorySlot *item_slot, const FVector &location, const float radius, AController *TargetRecipient) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(item_slot != nullptr, TEXT("DropItem:: Invalid ItemSlot!"));
    checkf(item_slot->GetItemInstance() != nullptr, TEXT("DropItem:: Invalid ItemInstance!"));
    
    // Get LootManager
    UMythicLootManagerSubsystem *loot_manager = lOwner->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();

    // Spawn the item
    AMythicWorldItem *world_item = loot_manager->Spawn(item_slot->GetItemInstance(), location, radius, TargetRecipient);
    if (world_item == nullptr) {
        return false;
    }

    // Set the item slot's item instance to null
    item_slot->SetItemInstance(nullptr);

    this->OnItemDropped.Broadcast(item_slot, world_item);

    return true;
}

void UMythicInventoryComponent::PickupItem(AMythicWorldItem *world_item) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(world_item != nullptr, TEXT("PickupItem:: Invalid WorldItem!"));

    // Make sure we are the TargetRecipient if one is set
    auto Recipient = world_item->GetTargetRecipient();
    if (Recipient && world_item->GetTargetRecipient() != lOwner) {
        UE_LOG(Mythic, Warning, TEXT("PickupItem:: Not the TargetRecipient!"));
        return;
    }

    // Get the item instance from the world item
    UMythicItemInstance *item_instance = world_item->ItemInstance;
    if (item_instance == nullptr) {
        return;
    }

    // Copy of the item instance. Doing this so the world item's item instance can be modified without affecting the inventory's item instance
    // in case the item is partially picked up.
    auto copied_item_instance = DuplicateObject<UMythicItemInstance>(item_instance, this);

    // If not all items could be added, AddItem will return a pointer to the dropped item
    auto OriginalQty = copied_item_instance->GetStacks();
    auto AmountAdded = AddToAnySlot(copied_item_instance);

    auto RemainingQty = OriginalQty - AmountAdded;

    if (RemainingQty > 0) {
        // Update the world item's item instance
        world_item->ItemInstance->SetStackSize(RemainingQty);
    }
    else {
        // Destroy the world item
        world_item->Destroy();
    }
}

void UMythicInventoryComponent::ClientUpdateViewModelSlots_Implementation() {
    if (this->ViewModel) {
        this->ViewModel->SetSlots(this->Slots);
    }
}

/*
* Item Activation Flow - Client Side
* ------------------------------
* 1. Receives activation change from server
* 2. Activates/Deactivates appropriate slots on client
* 3. Updates UI/ViewModel
* 4. Broadcasts events for client-side systems
* 
* Note: This is called automatically by the server's SetActiveSlotIndex
*/
void UMythicInventoryComponent::ClientOnActiveSlotChangedDelegate_Implementation(int32 NewIndex, int32 OldIndex) {
    // Handle client-side activation
    if (Slots.IsValidIndex(NewIndex)) {
        Slots[NewIndex]->ClientActivateSlot();
    }
    if (Slots.IsValidIndex(OldIndex)) {
        Slots[OldIndex]->ClientDeactivateSlot();
    }

    ClientUpdateViewModelActiveSlot(NewIndex);
    this->OnActiveSlotChanged.Broadcast(NewIndex, OldIndex);
}

void UMythicInventoryComponent::ClientUpdateViewModelActiveSlot_Implementation(int32 NewIndex) {
    if (this->ViewModel) {
        this->ViewModel->SetActiveSlotIndex(NewIndex);
    }
}

void UMythicInventoryComponent::ClientOnSlotUpdatedDelegate_Implementation(UMythicInventorySlot *Slot) {
    ClientUpdateViewModelSlots();

    this->OnSlotUpdated.Broadcast(Slot);
}

void UMythicInventoryComponent::ClientUpdateViewModeInventorySize_Implementation(int32 NewSize) {
    if (this->ViewModel) {
        this->ViewModel->SetDefaultSize(NewSize);
        this->ViewModel->SetSlots(this->Slots);
    }
}

void UMythicInventoryComponent::ClientOnInventorySizeChangedDelegate_Implementation(int32 NewSize, int32 OldSize) {
    ClientUpdateViewModeInventorySize(NewSize);

    this->OnInventorySizeChanged.Broadcast(NewSize, OldSize);
}

int32 UMythicInventoryComponent::GetItemCount(UItemDefinition *RequiredItem) const {
    auto count = 0;
    for (auto &Slot : Slots) {
        if (Slot->SlottedItemInstance && Slot->SlottedItemInstance->GetItemDefinition() == RequiredItem) {
            count += Slot->SlottedItemInstance->GetStacks();
        }
    }

    return count;
}

void UMythicInventoryComponent::ServerRemoveItem_Implementation(UMythicItemInstance *ItemInstance, int32 Amount) {
    // Remove the given amount of items from the inventory that match the given item definition. Returns the amount of items that were removed.
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("RemoveItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("RemoveItem:: Called without Authority!"));
    checkf(Amount > 0, TEXT("RemoveItem:: Invalid Amount!"));
    checkf(ItemInstance != nullptr, TEXT("RemoveItem:: Invalid ItemInstance!"));

    auto count = 0;

    if (ItemInstance->GetStacks() <= Amount) {
        count += ItemInstance->GetStacks();
        ItemInstance->GetSlot()->SetItemInstance(nullptr);
        ItemInstance->Destroy();
    }
    else {
        ItemInstance->SetStackSize(ItemInstance->GetStacks() - Amount);
        count += Amount;
    }

    UE_LOG(Mythic, Warning, TEXT("Removed %d items from inventory"), count);
}

void UMythicInventoryComponent::ServerRemoveItemByDefinition_Implementation(UItemDefinition *ItemDef, int32 Amount) {
    // Remove the given amount of items from the inventory that match the given item definition. Returns the amount of items that were removed.
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("RemoveItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("RemoveItem:: Called without Authority!"));
    checkf(ItemDef != nullptr, TEXT("RemoveItem:: Invalid ItemDef!"));
    checkf(Amount > 0, TEXT("RemoveItem:: Invalid Amount!"));

    auto count = 0;
    for (auto &Slot : Slots) {
        auto item = Slot->SlottedItemInstance;
        if (!item) {
            continue;
        }

        auto item_def = item->GetItemDefinition();
        if (item && item_def == ItemDef) {
            auto item_stacks = item->GetStacks();
            if (item_stacks <= Amount) {
                count += item_stacks;
                Slot->SetItemInstance(nullptr);
                Slot->SlottedItemInstance->Destroy();
            }
            else {
                Slot->SlottedItemInstance->SetStackSize(item_stacks - Amount);
                count += Amount;
            }

            if (count >= Amount) {
                break;
            }
        }
    }

    UE_LOG(Mythic, Warning, TEXT("Removed %d items from inventory"), count);
}

void UMythicInventoryComponent::OnUnregister() {
    const AActor *lOwner = GetOwner();
    if (lOwner && lOwner->HasAuthority()) {
        DestroyAllSlots();
    }

    Super::OnUnregister();
}
