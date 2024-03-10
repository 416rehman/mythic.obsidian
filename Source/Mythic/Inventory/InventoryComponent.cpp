// 


#include "InventoryComponent.h"

#include "NavigationSystem.h"
#include "WorldItem.h"
#include "Components/ListView.h"
#include "Mythic/Mythic.h"
#include "Mythic/Loot/LootManagerSubsystem.h"

UInventoryComponent::UInventoryComponent(const FObjectInitializer &OI):
    Super(OI) {
    SetIsReplicatedByDefault(true);
    SetIsReplicated(true);
    this->bReplicateUsingRegisteredSubObjectList = true;
}

void UInventoryComponent::BeginPlay() {
    Super::BeginPlay();

    // Use the resize function to create the initial slots
    if (GetOwner()->HasAuthority()) {
        UE_LOG(Mythic, Warning, TEXT("Inventory Component Begin Play:: Has Authority"));
        // Crashes if this is not delayed
        // Resize(DefaultSize);
        FTimerHandle ResizeTimer;
        GetWorld()->GetTimerManager().SetTimer(ResizeTimer, [this]() {
            Resize(DefaultSize);
        }, 2.f, false);
    }
}

void UInventoryComponent::SetActiveSlotIndex(int32 NewIndex) {
    if (NewIndex >= 0 && NewIndex < Slots.Num()) {
        Slots[ActiveSlotIndex]->OnSlotDeactivated();
        OnActiveSlotChanged.Broadcast(NewIndex, ActiveSlotIndex);
        ActiveSlotIndex = NewIndex;
        Slots[ActiveSlotIndex]->OnSlotActivated();
    }
}

int32 UInventoryComponent::GetActiveSlotIndex() const {
    return ActiveSlotIndex;
}

FGameplayTag UInventoryComponent::GetSlotType() const {
    if (SlotTypeRow.IsNull()) {
        UE_LOG(Mythic, Error, TEXT("UInventoryComponent::GetSlotType:: SlotTypeRow is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTag();
    }
    const FSlotType *SlotType = SlotTypeRow.GetRow<FSlotType>(SlotTypeRow.RowName.ToString());
    if (!SlotType) {
        UE_LOG(Mythic, Error, TEXT("UInventoryComponent::GetSlotType:: SlotType is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTag();
    }
    return SlotType->SlotType;
}

FGameplayTagContainer UInventoryComponent::GetSlotWhitelistedTypes() const {
    if (SlotTypeRow.IsNull()) {
        UE_LOG(Mythic, Error, TEXT("UInventoryComponent::GetSlotWhitelistedTypes:: SlotTypeRow is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTagContainer();
    }

    const FSlotType *SlotType = SlotTypeRow.GetRow<FSlotType>(SlotTypeRow.RowName.ToString());
    if (!SlotType) {
        UE_LOG(Mythic, Error, TEXT("UInventoryComponent::GetSlotWhitelistedTypes:: SlotType is null in %s"),
               *GetOwner()->GetName());
        return FGameplayTagContainer();
    }
    return SlotType->WhitelistedItemTypes;
}

bool UInventoryComponent::DestroySlot(UInventorySlot *InSlot) {
    if (InSlot) {
        AActor *lOwner = GetOwner();
        checkf(lOwner != nullptr, TEXT("Invalid Inventory Owner"));
        checkf(lOwner->HasAuthority(), TEXT("Called without Authority!"));
        const int32 RemovedIndex = Slots.Remove(InSlot);
        if (RemovedIndex != INDEX_NONE) {
            if (InSlot->ItemInstance) {
                InSlot->ItemInstance->Destroy();
            }

            InSlot->Destroy();
            return true;
        }
    }

    return false;
}

void UInventoryComponent::DestroyAllSlots() {
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

int32 UInventoryComponent::GetSlotCount() const {
    return Slots.Num();
}

void UInventoryComponent::PopulateListView(UListView *SlotListView) {
    if (SlotListView) {
        SlotListView->ClearListItems();
        for (int32 i = 0; i < Slots.Num(); ++i) {
            SlotListView->AddItem(Slots[i]);
            UE_LOG(Mythic, Warning, TEXT("Added Slot %d to ListView %s"), i, *SlotListView->GetName());
        }
    }
}

void UInventoryComponent::Resize(int32 NewSize) {
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
            UInventorySlot *NewSlot = NewObject<UInventorySlot>(this->GetOwner());
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
    OnInventorySizeChanged.Broadcast(NewSize, CurrentSize);
}

UInventorySlot *UInventoryComponent::GetSlot(int32 SlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("GetItem:: Called without Authority!"));

    if (Slots.IsValidIndex(SlotIndex)) {
        return Slots[SlotIndex];
    }

    return nullptr;
}

bool UInventoryComponent::TryTransferToSlot(UItemInstance *ItemInstance, UInventorySlot *TargetSlot) {
    auto old_slot_of_item = ItemInstance->GetSlot();

    // remove from old slot
    if (old_slot_of_item) {
        ItemInstance->GetSlot()->SetItemInstance(nullptr);
    }
    if (TargetSlot->SetItemInstance(ItemInstance)) {
        return true;
    }

    if (old_slot_of_item) {
        // if we failed to add the item to the new slot, put it back in the old one
        UE_LOG(Mythic, Warning, TEXT("Failed to add item to slot, putting it back in the old one"));
        old_slot_of_item->SetItemInstance(ItemInstance);
    }

    return false;
}

AWorldItem * UInventoryComponent::AddItem(UItemInstance *ItemInstance, bool is_private) {
    auto original_qty = ItemInstance->GetStacks();
    auto amount_added = AddToAnySlot(ItemInstance);

    if (amount_added != original_qty) {
        // Create a world item and return it
        ULootManagerSubsystem *loot_manager = GetOwner()->GetGameInstance()->GetSubsystem<ULootManagerSubsystem>();
        AWorldItem *world_item = loot_manager->Spawn(ItemInstance, GetOwner()->GetActorLocation(), 100, GetOwner(), is_private);
        if (world_item) {
            return world_item;
        }
    }

    return nullptr;
}

int32 UInventoryComponent::AddToAnySlot(UItemInstance *ItemInstance) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    const int32 original_qty = ItemInstance->GetStacks();

    // Can be stacked?
    if (ItemInstance->GetItemDefinitionObject()->StackSizeMax > ItemInstance->GetStacks()) {
        // Add to existing stack
        for (int32 i = 0; i < Slots.Num(); ++i) {
            if (Slots[i]->GetItemInstance() != nullptr && Slots[i]->GetItemInstance()->GetItemDefinitionObject() ==
                ItemInstance->GetItemDefinitionObject()) {
                // Update existing stack
                const auto currentSlotQty = Slots[i]->GetItemInstance()->GetStacks();
                const int32 availableSpace = Slots[i]->GetItemInstance()->GetItemDefinitionObject()->StackSizeMax -
                    currentSlotQty;
                const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

                Slots[i]->GetItemInstance()->SetStackSize(currentSlotQty + QuantityToAdd);
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

int32 UInventoryComponent::AddToSlot(UItemInstance *ItemInstance, int32 SlotIndex) {
    const int32 original_qty = ItemInstance->GetStacks();
    if (Slots.IsValidIndex(SlotIndex)) {
        // If the target slot is empty, move the entire item there
        if (Slots[SlotIndex]->ItemInstance == nullptr) {
            if (TryTransferToSlot(ItemInstance, Slots[SlotIndex])) {
                return original_qty;
            }
        }

        // If the target slot is NOT empty, and the item in the slot is the same as the item we're trying to add, stack it
        else if (Slots[SlotIndex]->ItemInstance->GetItemDefinitionObject() == ItemInstance->GetItemDefinitionObject()) {
            const int32 availableSpace = Slots[SlotIndex]->ItemInstance->GetItemDefinitionObject()->StackSizeMax - Slots[
                SlotIndex]->ItemInstance->GetStacks();
            const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

            Slots[SlotIndex]->ItemInstance->SetStackSize(Slots[SlotIndex]->ItemInstance->GetStacks() + QuantityToAdd);
            ItemInstance->SetStackSize(ItemInstance->GetStacks() - QuantityToAdd);

            if (ItemInstance->GetStacks() == 0) { // If there are no more stacks left, destroy the item and return the amount added
                ItemInstance->Destroy();
                return original_qty;
            }
        }
    }

    return original_qty - ItemInstance->GetStacks();
}

int32 UInventoryComponent::ReceiveItem(TObjectPtr<UItemInstance> ItemInstance, UInventorySlot *TargetSlot) {
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

int32 UInventoryComponent::SendItem(UInventorySlot *ItemSlot, UInventoryComponent *TargetInventory,
                                    UInventorySlot *TargetSlot) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(TargetInventory != nullptr, TEXT("SendItem:: Invalid TargetInventory!"));
    checkf(ItemSlot != nullptr, TEXT("SendItem:: Invalid ItemSlot!"));
    checkf(ItemSlot->GetItemInstance() != nullptr, TEXT("SendItem:: Invalid ItemInstance!"));

    // ItemInstance could be destroyed by the ReceiveItem function if it is fully consumed so we need to store the quantity before calling it.
    int32 amountSent = TargetInventory->ReceiveItem(ItemSlot->ItemInstance, TargetSlot);

    return amountSent;
}

bool UInventoryComponent::DropItem(UInventorySlot *item_slot, const FVector &location, const float radius) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(item_slot != nullptr, TEXT("DropItem:: Invalid ItemSlot!"));
    checkf(item_slot->GetItemInstance() != nullptr, TEXT("DropItem:: Invalid ItemInstance!"));

    // Get LootManager
    ULootManagerSubsystem *loot_manager = lOwner->GetGameInstance()->GetSubsystem<ULootManagerSubsystem>();

    AWorldItem *world_item = loot_manager->Spawn(item_slot->GetItemInstance(), location, radius, GetOwner(), true);
    if (world_item == nullptr) {
        return false;
    }

    // Set the item slot's item instance to null
    item_slot->SetItemInstance(nullptr);

    this->OnItemDropped.Broadcast(item_slot, world_item);

    return true;
}

// int32 UInventoryComponent::PickupItem(AWorldItem *world_item) {
    // AActor *lOwner = GetOwner();
    // checkf(lOwner != nullptr, TEXT("PickupItem:: Invalid Inventory Owner"));
    // checkf(lOwner->HasAuthority(), TEXT("PickupItem:: Called without Authority!"));
    // checkf(world_item != nullptr, TEXT("PickupItem:: Invalid WorldItem!"));
    // checkf(world_item->ItemInstance != nullptr, TEXT("PickupItem:: Invalid ItemInstance!"));
    //
    // const int32 prePickupQty = world_item->ItemInstance->GetStacks();
    //
    // // Try to add the item to the inventory
    // int32 amount_added = AddToAnySlot(world_item->ItemInstance);
    //
    // // If the item was fully added to the inventory, destroy the world item
    // if (amount_added >= prePickupQty) {
    //     world_item->Destroy();
    // }
    //
    // return amount_added;
// }


void UInventoryComponent::OnUnregister() {
    const AActor *lOwner = GetOwner();
    if (lOwner && lOwner->HasAuthority()) {
        DestroyAllSlots();
    }

    Super::OnUnregister();
}
