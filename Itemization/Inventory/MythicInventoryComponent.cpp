// 


#include "MythicInventoryComponent.h"
#include "MVVMGameSubsystem.h"
#include "MVVMSubsystem.h"
#include "MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Mythic/Itemization/Loot/MythicLootManagerSubsystem.h"

UMythicInventoryComponent::UMythicInventoryComponent(const FObjectInitializer &OI) :
    Super(OI) {
    SetIsReplicatedByDefault(true);
    SetIsReplicated(true);
    this->bReplicateUsingRegisteredSubObjectList = true;
}

void FMythicInventorySlotEntry::ActivateSlot() {
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

void FMythicInventorySlotEntry::DeactivateSlot() {
    // If an item is slotted and active, deactivate it
    if (SlottedItemInstance && bIsActive) {
        SlottedItemInstance->OnInactiveItem();
    }

    this->bIsActive = false;
}

void FMythicInventorySlotEntry::Clear() {
    // Clear the slot
    if (SlottedItemInstance) {
        if (bIsActive) {
            SlottedItemInstance->OnInactiveItem();
        }

        this->SlottedItemInstance->SetInventory(nullptr, INDEX_NONE);
        this->SlottedItemInstance = nullptr;
    }
}

void FMythicInventoryFastArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {}
void FMythicInventoryFastArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {}
void FMythicInventoryFastArray::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {}

void UMythicInventoryComponent::ClientSetupViewModel_Implementation() {
    if (this->ViewModelIdentifier.IsNone()) {
        UE_LOG(Myth, Error, TEXT("Inventory Component Begin Play:: ViewModelIdentifier is empty"));
        return;
    }

    // Add the view model to the view model subsystem
    auto ViewModelSubsystem = GetOwner()->GetGameInstance()->GetSubsystem<UMVVMGameSubsystem>();
    auto collection = ViewModelSubsystem->GetViewModelCollection();

    // Create the view model
    // this->ViewModel = NewObject<UInventoryVM>(GetOwner());
    // this->ViewModel->Initialize(SlotConfigurations, Slots);
    // FMVVMViewModelContext ViewModelContext;
    // ViewModelContext.ContextClass = this->ViewModel->GetClass();
    // ViewModelContext.ContextName = this->ViewModelIdentifier;

    // if (collection->AddViewModelInstance(ViewModelContext, this->ViewModel)) {
    //     UE_LOG(Myth, Warning, TEXT("Inventory Component Begin Play:: Added ViewModel %s to ViewModelCollection on Role: %s"),
    //            *this->ViewModelIdentifier.ToString(), *FString::FromInt(GetOwnerRole()));
    //
    //     OnViewModelCreated.Broadcast();
    // }
}

void UMythicInventoryComponent::BeginPlay() {
    Super::BeginPlay();

    // Use the resize function to create the initial slots
    if (GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Warning, TEXT("Inventory Component Begin Play:: Has Authority"));
        InitializeSlots();
    }
    // We need to setup the view model for both client and server
    ClientSetupViewModel();
}

void UMythicInventoryComponent::InitializeSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("InitializeSlots:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("InitializeSlots:: Called without Authority!"));

    const int32 OldSlotsSize = Slots.Num();

    // Clear out any existing slots first
    DestroyAllSlots();

    // Create new slots based on the configurations
    for (const FInventorySlotConfiguration &Config : SlotConfigurations) {
        if (Config.SlotTypeRow.IsNull()) {
            UE_LOG(Myth, Error, TEXT("Inventory Component InitializeSlots:: SlotTypeRow is null in a configuration for owner %s"), *GetOwner()->GetName());
            continue;
        }

        const FSlotType *SlotTypeData = Config.SlotTypeRow.GetRow<FSlotType>(FString());
        if (!SlotTypeData) {
            UE_LOG(Myth, Error, TEXT("Inventory Component InitializeSlots:: Failed to find SlotType for row %s in owner %s"),
                   *Config.SlotTypeRow.RowName.ToString(), *GetOwner()->GetName());
            continue;
        }

        for (int32 i = 0; i < Config.NumSlots; ++i) {
            auto SlotEntry = FMythicInventorySlotEntry();
            SlotEntry.SlotType = SlotTypeData->SlotType;
            SlotEntry.ItemTypeWhitelist = SlotTypeData->WhitelistedItemTypes;
            SlotEntry.bIsActive = Config.bIsEquipable;
            SlotEntry.SlotTypeRow = Config.SlotTypeRow;

            Slots.AddSlot(SlotEntry);
        }
    }

    const int32 NewSlotsSize = Slots.Num();
    UE_LOG(Myth, Warning, TEXT("Initialized Inventory from %d to %d slots"), OldSlotsSize, NewSlotsSize);

    if (OldSlotsSize != NewSlotsSize) {
        ClientOnInventorySizeChangedDelegate(NewSlotsSize, OldSlotsSize);
    }
    else {
        // If the size is the same, we still need to update the view model with the new slot instances
        ClientUpdateViewModelSlots();
    }
}

bool UMythicInventoryComponent::CanAcceptItemType(const FGameplayTag &ItemType) const {
    for (auto Slot : Slots.Items) {
        // If a slot has an empty whitelist, it accepts all types.
        // Otherwise, check if the item type matches the whitelist.
        if (Slot.ItemTypeWhitelist.Num() == 0 || ItemType.MatchesAny(Slot.ItemTypeWhitelist)) {
            return true;
        }

    }
    return false;
}

UMythicItemInstance *UMythicInventoryComponent::GetItem(int32 SlotIndex) {
    if (Slots.IsValidIndex(SlotIndex)) {
        return Slots.Items[SlotIndex].SlottedItemInstance;
    }

    return nullptr;
}

bool UMythicInventoryComponent::TryTransferToSlot(UMythicItemInstance *ItemInstance, int32 TargetSlotIndex) {
    auto OldInventory = ItemInstance->GetInventoryComponent();
    auto OldItemSlot = ItemInstance->GetSlot();

    // remove from old slot
    if (OldInventory) {
        OldInventory->SetItemInSlot(OldItemSlot, nullptr);
    }
    if (this->SetItemInSlot(TargetSlotIndex, ItemInstance)) {
        return true;
    }

    if (OldItemSlot) {
        // if we failed to add the item to the new slot, put it back in the old one
        UE_LOG(Myth, Warning, TEXT("Failed to add item to slot, putting it back in the old one"));
        OldInventory->SetItemInSlot(OldItemSlot, ItemInstance);
    }

    return false;
}

bool UMythicInventoryComponent::SetItemInSlot(int32 SlotIndex, UMythicItemInstance *NewItemInstance) {
    // Authority Check
    checkf(GetOwner()->HasAuthority(), TEXT("This function is server-only."));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return false; // Invalid index
    }

    if (!NewItemInstance) {
        Slots.ModifySlotAtIndex(SlotIndex, [](FMythicInventorySlotEntry &SlotData) {
            SlotData.Clear();
        });

        this->ClientOnSlotUpdatedDelegate(SlotIndex);
        return true;
    }

    // Get a mutable reference to the slot data
    FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    // If slot has an item already, reject the new item
    if (Slot.SlottedItemInstance) {
        UE_LOG(Myth, Warning, TEXT("There is already an item in this slot"));
        return false;
    }

    // Whitelist check (logic moved from the old UObject)
    if (Slot.ItemTypeWhitelist.Num() > 0) {
        if (!NewItemInstance->GetItemDefinition()->ItemType.MatchesAny(Slot.ItemTypeWhitelist)) {
            return false;
        }
    }

    // Update the data in the replicated struct
    Slot.SlottedItemInstance = NewItemInstance;
    Slot.SlottedItemInstance->SetOwner(this);
    Slot.SlottedItemInstance->SetInventory(this, SlotIndex);

    // If the slot is equipable, handle activation logic
    if (Slot.bIsActive) {
        Slot.ActivateSlot();
    }

    // Mark this specific item in the array as dirty to replicate the change
    Slots.MarkItemDirty(Slot);

    this->ClientOnSlotUpdatedDelegate(SlotIndex);

    UE_LOG(Myth, Warning, TEXT("Item added to slot successfully"));
    return true;
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
            auto ItemInSlot = Slots.Items[i].SlottedItemInstance;
            // Check if the item can be stacked with the existing item
            if (ItemInSlot != nullptr && ItemInSlot->GetItemDefinition() == ItemInstance->GetItemDefinition() && ItemInstance->isStackableWith(ItemInSlot)) {
                // Update existing stack
                const int32 availableSpace = ItemInSlot->GetItemDefinition()->StackSizeMax - ItemInSlot->GetStacks();
                const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

                ItemInSlot->SetStackSize(ItemInSlot->GetStacks() + QuantityToAdd);
                ItemInstance->SetStackSize(ItemInstance->GetStacks() - QuantityToAdd);

                // Destroy the item if it's remaining quantity is 0 and return the amount added
                if (ItemInstance->GetStacks() == 0) {
                    ItemInstance->Destroy();
                    return original_qty;
                }
            }
        }
    }

    // Find the first empty slot and add the rest of the item there
    for (int32 i = 0; i < Slots.Num(); ++i) {
        if (Slots.Items[i].SlottedItemInstance == nullptr) {
            if (TryTransferToSlot(ItemInstance, i)) {
                return original_qty; // The entire item was moved.
            }
        }
    }

    return original_qty - ItemInstance->GetStacks();
}

int32 UMythicInventoryComponent::AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex) {
    if (!ItemInstance) {
        UE_LOG(Myth, Warning, TEXT("AddToSlot: ItemInstance is null"));
        return 0;
    }
    const int32 OriginalQty = ItemInstance->GetStacks();
    if (Slots.IsValidIndex(SlotIndex)) {
        auto SlottedItem = Slots.Items[SlotIndex].SlottedItemInstance;
        // If the target slot is empty, move the entire item there
        if (SlottedItem == nullptr) {
            if (TryTransferToSlot(ItemInstance, SlotIndex)) {
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

int32 UMythicInventoryComponent::ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, int32 TargetSlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    if (TargetSlotIndex == INDEX_NONE) {
        return AddToAnySlot(ItemInstance);
    }

    return AddToSlot(ItemInstance, TargetSlotIndex);
}

int32 UMythicInventoryComponent::SendItem(int32 SlotIndex, UMythicInventoryComponent *TargetInventory, int32 TargetSlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(TargetInventory != nullptr, TEXT("SendItem:: Invalid TargetInventory!"));

    // If same slot, return 0
    if (this == TargetInventory && SlotIndex == TargetSlotIndex) {
        return 0;
    }

    auto itemInstance = Slots.GetItemInSlot(SlotIndex);
    if (!itemInstance) {
        return 0; // No item in the slot
    }

    // ItemInstance could be destroyed by the ReceiveItem function if it is fully consumed so we need to store the quantity before calling it.
    int32 amountSent = TargetInventory->ReceiveItem(itemInstance, TargetSlotIndex);

    // Clear source slot if item was fully consumed
    if (!IsValid(itemInstance) || itemInstance->GetStacks() == 0) {
        SetItemInSlot(SlotIndex, nullptr);
    }

    return amountSent;
}

bool UMythicInventoryComponent::DropItem(int32 SlotIndex, const FVector &location, const float radius, AController *TargetRecipient) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));

    auto item_instance = Slots.GetItemInSlot(SlotIndex);
    checkf(item_instance != nullptr, TEXT("DropItem:: Invalid ItemInstance!"));

    // Get LootManager
    UMythicLootManagerSubsystem *loot_manager = lOwner->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();

    // Spawn the item
    AMythicWorldItem *world_item = loot_manager->Spawn(item_instance, location, radius, TargetRecipient);
    if (world_item == nullptr) {
        return false;
    }

    // Verify the world item actually has our item instance
    if (world_item->ItemInstance != item_instance) {
        UE_LOG(Myth, Error, TEXT("DropItem:: World item failed to take ownership of item instance"));
        world_item->Destroy(); // Clean up the failed world item
        return false;
    }

    // Set the item slot's item instance to null
    SetItemInSlot(SlotIndex, nullptr);

    this->OnItemDropped.Broadcast(SlotIndex, world_item);

    return true;
}

void UMythicInventoryComponent::PickupItem_Implementation(AMythicWorldItem *world_item) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(world_item != nullptr, TEXT("PickupItem:: Invalid WorldItem!"));

    // Make sure we are the TargetRecipient if one is set
    auto Recipient = world_item->GetTargetRecipient();
    if (Recipient && Recipient != lOwner) {
        UE_LOG(Myth, Warning, TEXT("PickupItem:: Not the TargetRecipient!"));
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

    if (AmountAdded >= OriginalQty) {
        // All items were picked up
        world_item->Destroy();
    }
    else if (AmountAdded > 0) {
        // Partial pickup - update the world item's remaining quantity
        auto RemainingQty = OriginalQty - AmountAdded;
        world_item->ItemInstance->SetStackSize(RemainingQty);
    }
    else {
        // Nothing was picked up - clean up the copied instance
        if (IsValid(copied_item_instance)) {
            copied_item_instance->Destroy();
        }
    }
}

void UMythicInventoryComponent::ClientUpdateViewModelSlots_Implementation() {
    // if (this->ViewModel) {
        // this->ViewModel->Initialize(SlotConfigurations, Slots);
    // }
}

void UMythicInventoryComponent::ClientOnSlotUpdatedDelegate_Implementation(int32 SlotIndex) {
    ClientUpdateViewModelSlots();

    this->OnSlotUpdated.Broadcast(SlotIndex);
}

void UMythicInventoryComponent::ClientUpdateViewModeInventorySize_Implementation(int32 NewSize) {
    // if (this->ViewModel) {
        // this->ViewModel->SetSlots(this->Slots);
    // }
}

void UMythicInventoryComponent::ClientOnInventorySizeChangedDelegate_Implementation(int32 NewSize, int32 OldSize) {
    ClientUpdateViewModeInventorySize(NewSize);

    this->OnInventorySizeChanged.Broadcast(NewSize, OldSize);
}

int32 UMythicInventoryComponent::GetItemCount(UItemDefinition *RequiredItem) const {
    if (!RequiredItem) {
        return 0; // Early return for null definition
    }

    auto count = 0;
    for (const FMythicInventorySlotEntry &Slot : Slots.Items) {
        if (Slot.SlottedItemInstance && Slot.SlottedItemInstance->GetItemDefinition() == RequiredItem) {
            count += Slot.SlottedItemInstance->GetStacks();
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

    // Check item is from this inventory
    if (ItemInstance->GetInventoryComponent() != this) {
        UE_LOG(Myth, Error, TEXT("RemoveItem:: ItemInstance is not from this inventory!"));
        return;
    }

    auto count = 0;

    if (ItemInstance->GetStacks() <= Amount) {
        count += ItemInstance->GetStacks();
        ItemInstance->Destroy();
    }
    else {
        ItemInstance->SetStackSize(ItemInstance->GetStacks() - Amount);
        count += Amount;
    }

    UE_LOG(Myth, Warning, TEXT("Removed %d items from inventory"), count);
}

void UMythicInventoryComponent::ServerRemoveItemByDefinition_Implementation(UItemDefinition *ItemDef, int32 Amount) {
    // Remove the given amount of items from the inventory that match the given item definition. Returns the amount of items that were removed.
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("RemoveItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("RemoveItem:: Called without Authority!"));
    checkf(ItemDef != nullptr, TEXT("RemoveItem:: Invalid ItemDef!"));
    checkf(Amount > 0, TEXT("RemoveItem:: Invalid Amount!"));

    auto RemovedSoFar = 0;
    for (const FMythicInventorySlotEntry &Slot : Slots.Items) {
        auto item = Slot.SlottedItemInstance;
        if (!item) {

            continue;
        }

        auto item_def = item->GetItemDefinition();
        if (item && item_def == ItemDef) {
            auto ItemStacks = item->GetStacks();
            auto RemainingToRemove = Amount - RemovedSoFar;

            if (ItemStacks <= RemainingToRemove) {
                RemovedSoFar += ItemStacks;
                item->Destroy();
            }
            else {
                Slot.SlottedItemInstance->SetStackSize(ItemStacks - RemainingToRemove);
                RemovedSoFar += RemainingToRemove;
            }

            if (RemovedSoFar >= Amount) {
                break;
            }
        }
    }

    UE_LOG(Myth, Warning, TEXT("Removed %d items from inventory"), RemovedSoFar);
}

bool UMythicInventoryComponent::DestroySlot(int32 SlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("Called without Authority!"));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return false; // Invalid index
    }

    auto InSlot = Slots.Items[SlotIndex];
    InSlot.DeactivateSlot();

    Slots.Items.RemoveAt(SlotIndex);
    if (InSlot.SlottedItemInstance) {
        InSlot.SlottedItemInstance->Destroy();
    }

    return true;
}

void UMythicInventoryComponent::DestroyAllSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("DestroySlot:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("DestroySlot:: Called without Authority!"));
    for (int i = 0; i < Slots.Num(); ++i) {
        DestroySlot(i);
    }

    Slots.Items.Empty();
}

void UMythicInventoryComponent::OnUnregister() {
    const AActor *lOwner = GetOwner();
    if (lOwner && lOwner->HasAuthority()) {
        DestroyAllSlots();
    }

    Super::OnUnregister();
}
