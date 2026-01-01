// 


#include "MythicInventoryComponent.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Mythic/Itemization/Loot/MythicLootManagerSubsystem.h"
#include "ViewModels/InventoryVM.h"

UMythicInventoryComponent::UMythicInventoryComponent(const FObjectInitializer &OI) :
    Super(OI) {
    SetIsReplicatedByDefault(true);
    SetIsReplicated(true);
    this->bReplicateUsingRegisteredSubObjectList = true;

    // Set owner on the FastArray for client-side callbacks
    Slots.SetOwningInventory(this);
}

void FMythicInventorySlotEntry::ClientUpdateActiveState() {
    bool bChanged = ClientLastKnownItem != SlottedItemInstance;
    UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Slot Item: %s, LastKnown: %s, bEquipmentSlot: %d, Changed: %d"), 
        SlottedItemInstance ? *SlottedItemInstance->GetName() : TEXT("Null"),
        ClientLastKnownItem ? *ClientLastKnownItem->GetName() : TEXT("Null"),
        bEquipmentSlot,
        bChanged);

    // If the item has changed (or this is the first time we're seeing it)
    if (bChanged) {
        // Deactivate old item
        if (IsValid(ClientLastKnownItem)) {
            UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Deactivating Old Item: %s"), *ClientLastKnownItem->GetName());
            ClientLastKnownItem->OnClientInactiveItem();
        }

        // Activate new item if this is an equipment slot
        if (bEquipmentSlot && IsValid(SlottedItemInstance)) {
            UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Activating New Item: %s"), *SlottedItemInstance->GetName());
            // Optional: Check whitelist if strict validation is needed on client, 
            // but usually we trust the server state in the slot.
            SlottedItemInstance->OnClientActiveItem();
        }

        // Update tracking
        ClientLastKnownItem = SlottedItemInstance;
    }
}

void FMythicInventorySlotEntry::ServerUpdateActiveState() {
    if (bEquipmentSlot && IsValid(SlottedItemInstance)) {
        SlottedItemInstance->OnActiveItem();
    }
}

void FMythicInventorySlotEntry::Clear() {
    // Clear the slot references
    if (SlottedItemInstance) {
        this->SlottedItemInstance->SetInventory(nullptr, INDEX_NONE);
        this->SlottedItemInstance = nullptr;
    }
}

// FastArray replication callbacks
void FMythicInventoryFastArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    if (Owner) {
        UE_LOG(Myth, Log, TEXT("FastArray: PostReplicatedAdd called with %d indices"), AddedIndices.Num());
        Owner->HandleSlotsAdded(AddedIndices, FinalSize);
    }
}

void FMythicInventoryFastArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {
    if (Owner) {
        UE_LOG(Myth, Log, TEXT("FastArray: PostReplicatedChange called with %d indices"), ChangedIndices.Num());
        Owner->HandleSlotsChanged(ChangedIndices, FinalSize);
    }
}

void FMythicInventoryFastArray::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    if (Owner) {
        UE_LOG(Myth, Log, TEXT("FastArray: PreReplicatedRemove called with %d indices"), RemovedIndices.Num());
        Owner->HandleSlotsRemoved(RemovedIndices, FinalSize);
    }
}

void FMythicInventoryFastArray::AddSlot(const FMythicInventorySlotEntry &NewSlot) {
    FMythicInventorySlotEntry &AddedItem = Items.Add_GetRef(NewSlot);
    MarkItemDirty(AddedItem);

    // Manually trigger callback for Listen Server (Host)
    if (Owner && Owner->GetNetMode() != NM_Client) {
        int32 AddedIndex = Items.Num() - 1;
        PostReplicatedAdd(TArrayView<int32>(&AddedIndex, 1), Items.Num());
    }
}

void FMythicInventoryFastArray::RemoveSlotAt(int32 Index) {
    if (Items.IsValidIndex(Index)) {
        // Manually trigger callback for Listen Server (Host) BEFORE removal
        if (Owner && Owner->GetNetMode() != NM_Client) {
            int32 RemovedIndex = Index;
            PreReplicatedRemove(TArrayView<int32>(&RemovedIndex, 1), Items.Num() - 1);
        }

        Items.RemoveAt(Index);
        MarkArrayDirty();
    }
}

void FMythicInventoryFastArray::ModifySlotAtIndex(int32 Index, const TFunction<void(FMythicInventorySlotEntry &SlotData)> &Modifier) {
    if (Items.IsValidIndex(Index)) {
        Modifier(Items[Index]);
        MarkItemDirty(Items[Index]);

        // Manually trigger callback for Listen Server (Host)
        if (Owner && Owner->GetNetMode() != NM_Client) {
            int32 ChangedIndex = Index;
            PostReplicatedChange(TArrayView<int32>(&ChangedIndex, 1), Items.Num());
        }
    }
}

void UMythicInventoryComponent::SetupLocalViewModel() {
    // Only create VMs where they can be used (not on dedicated server)
    if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer) {
        return;
    }

    const bool bWasNull = (ViewModel == nullptr);
    if (!IsValid(ViewModel)) {
        ViewModel = NewObject<UInventoryVM>(this);
    }
    if (IsValid(ViewModel)) {
        ViewModel->InitializeFromInventoryComponent(this);
        if (bWasNull) {
            OnViewModelCreated.Broadcast();
        }
    }
}

void UMythicInventoryComponent::BeginPlay() {
    Super::BeginPlay();

    // Ensure owner is set for callbacks
    Slots.Owner = this;

    // Use the resize function to create the initial slots
    if (GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Verbose, TEXT("Inventory Component BeginPlay: Has Authority"));
        InitializeSlots();
    }

    // Setup or refresh the local view model; subsequent updates are driven by FastArray callbacks
    SetupLocalViewModel();
}

void UMythicInventoryComponent::OnRep_Slots() {
    // Ensure owner is set for callbacks
    Slots.Owner = this;
}

void UMythicInventoryComponent::InitializeSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("InitializeSlots:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("InitializeSlots:: Called without Authority!"));

    const int32 OldSlotsSize = Slots.Num();

    // Clear out any existing slots first
    DestroyAllSlots();

    if (!InventoryProfile) {
        UE_LOG(Myth, Warning, TEXT("[InitializeSlots] No details profile assigned to inventory component on %s"), *lOwner->GetName());
        return;
    }

    // Create new slots based on the profile
    for (const FInventoryProfileEntry &Entry : InventoryProfile->Slots) {
        if (!Entry.SlotDefinition) {
            UE_LOG(Myth, Error, TEXT("Inventory InitializeSlots:: SlotDefinition is null in profile %s for owner %s"), *InventoryProfile->GetName(),
                   *GetOwner()->GetName());
            continue;
        }

        for (int32 i = 0; i < Entry.Count; ++i) {
            FMythicInventorySlotEntry SlotEntry;
            SlotEntry.SlotDefinition = Entry.SlotDefinition;
            SlotEntry.bEquipmentSlot = Entry.bIsActiveSlot;

            Slots.AddSlot(SlotEntry);
        }
    }

    const int32 NewSlotsSize = Slots.Num();
    UE_LOG(Myth, Verbose, TEXT("Initialized Inventory from %d to %d slots"), OldSlotsSize, NewSlotsSize);

    if (OldSlotsSize != NewSlotsSize) {
        // Server-side local update for any UI (listen server)
        // SetupLocalViewModel();
        OnInventorySizeChanged.Broadcast(NewSlotsSize, OldSlotsSize);
    }
    else {
        // If the size is the same, update the view model items
        if (IsValid(ViewModel)) {
            ViewModel->RefreshAllItemsFromInventory(this);
        }
    }
}

bool UMythicInventoryComponent::CanAcceptItemType(const FGameplayTag &ItemType) const {
    for (const auto &Slot : Slots.Items) {
        // If a slot has an empty whitelist, it accepts all types.
        // Otherwise, check if the item type matches the whitelist.
        if (Slot.SlotDefinition) {
            if (Slot.SlotDefinition->WhitelistedItemTypes.Num() == 0 || ItemType.MatchesAny(Slot.SlotDefinition->WhitelistedItemTypes)) {
                return true;
            }
        }
    }
    return false;
}

UMythicItemInstance *UMythicInventoryComponent::GetItem(int32 SlotIndex) {
    return Slots.GetItemInSlot(SlotIndex);
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

    if (OldItemSlot != INDEX_NONE && OldInventory) {
        // if we failed to add the item to the new slot, put it back in the old one
        UE_LOG(Myth, Verbose, TEXT("Failed to add item to slot, reverting to previous"));
        OldInventory->SetItemInSlot(OldItemSlot, ItemInstance);
    }

    return false;
}

bool UMythicInventoryComponent::SetItemInSlot(int32 SlotIndex, UMythicItemInstance *NewItemInstance) {
    // Authority Check
    checkf(GetOwner()->HasAuthority(), TEXT("This function is server-only."));

    return SetItemInSlotInternal(SlotIndex, NewItemInstance);
}

bool UMythicInventoryComponent::SetItemInSlotInternal(int32 SlotIndex, UMythicItemInstance *NewItemInstance) {
    if (!Slots.IsValidIndex(SlotIndex)) {
        return false; // Invalid index
    }

    if (!NewItemInstance) {
        // Deactivate old item if applicable
        if (Slots.Items[SlotIndex].SlottedItemInstance && Slots.Items[SlotIndex].bEquipmentSlot) {
             Slots.Items[SlotIndex].SlottedItemInstance->OnInactiveItem();
        }

        Slots.ModifySlotAtIndex(SlotIndex, [](FMythicInventorySlotEntry &SlotData) {
            SlotData.Clear();
        });

        NotifyItemInstanceUpdated(SlotIndex);
        return true;
    }

    // Get a mutable reference to the slot data
    FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    // If slot has an item already, reject the new item
    if (Slot.SlottedItemInstance) {
        UE_LOG(Myth, Warning, TEXT("There is already an item in this slot"));
        return false;
    }

    // Whitelist check
    if (Slot.SlotDefinition && (Slot.SlotDefinition->WhitelistedItemTypes.Num() > 0)) {
        if (!NewItemInstance->GetItemDefinition()) {
            UE_LOG(Myth, Error, TEXT("SetItemInSlotInternal: ItemDefinition is null for item %s"), *NewItemInstance->GetName());
            return false;
        }
        if (!NewItemInstance->GetItemDefinition()->ItemType.MatchesAny(Slot.SlotDefinition->WhitelistedItemTypes)) {
            return false;
        }
    }

    // Deactivate old item if applicable
    if (Slot.SlottedItemInstance && Slot.bEquipmentSlot) {
        Slot.SlottedItemInstance->OnInactiveItem();
    }

    // Update the data in the replicated struct and mark dirty
    Slots.ModifySlotAtIndex(SlotIndex, [this, NewItemInstance, SlotIndex](FMythicInventorySlotEntry &Slot) {
        Slot.SlottedItemInstance = NewItemInstance;
        if (IsValid(Slot.SlottedItemInstance)) {
            Slot.SlottedItemInstance->SetOwner(this);
            Slot.SlottedItemInstance->SetInventory(this, SlotIndex);
            
            // Activate new item if applicable
            Slot.ServerUpdateActiveState();
        }
    });

    NotifyItemInstanceUpdated(SlotIndex);

    UE_LOG(Myth, Verbose, TEXT("Item added to slot %d successfully"), SlotIndex);
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
        UE_LOG(Myth, Verbose, TEXT("AddToSlot: ItemInstance is null"));
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
    if (item_instance == nullptr) {
        UE_LOG(Myth, Verbose, TEXT("DropItem:: No item in slot %d"), SlotIndex);
        return false;
    }

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
        UE_LOG(Myth, Verbose, TEXT("PickupItem:: Not the TargetRecipient!"));
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

// FastArray -> Component handlers
void UMythicInventoryComponent::HandleSlotsAdded(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    const int32 NumAdded = AddedIndices.Num();
    const int32 OldSize = FinalSize - NumAdded;

    // Structural change: rebuild VM once, then refresh changed slots (no-cost if rebuild already covers)
    SetupLocalViewModel();

    if (NumAdded > 0) {
        OnInventorySizeChanged.Broadcast(FinalSize, OldSize);
    }

    for (int32 idx : AddedIndices) {
        if (Slots.IsValidIndex(idx)) {
            Slots.Items[idx].ClientUpdateActiveState();
        }
        
        OnSlotUpdated.Broadcast(idx);
        if (IsValid(ViewModel)) {
            ViewModel->RefreshSlotFromInventory(this, idx);
        }
    }
}

void UMythicInventoryComponent::HandleSlotsChanged(const TArrayView<int32> &ChangedIndices, int32 /*FinalSize*/) {
    for (int32 idx : ChangedIndices) {
        if (Slots.IsValidIndex(idx)) {
            Slots.Items[idx].ClientUpdateActiveState();
        }

        OnSlotUpdated.Broadcast(idx);
        if (IsValid(ViewModel)) {
            ViewModel->RefreshSlotFromInventory(this, idx);
        }
    }
}

void UMythicInventoryComponent::HandleSlotsRemoved(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    // Deactivate items that are about to be removed
    for (int32 idx : RemovedIndices) {
        if (Slots.IsValidIndex(idx)) {
            FMythicInventorySlotEntry& Slot = Slots.Items[idx];
            if (IsValid(Slot.ClientLastKnownItem)) {
                Slot.ClientLastKnownItem->OnClientInactiveItem();
                Slot.ClientLastKnownItem = nullptr;
            }
        }
    }

    const int32 NumRemoved = RemovedIndices.Num();
    const int32 OldSize = FinalSize + NumRemoved;

    // Structural change: rebuild VM
    SetupLocalViewModel();

    if (NumRemoved > 0) {
        OnInventorySizeChanged.Broadcast(FinalSize, OldSize);
    }
}

UInventoryVM *UMythicInventoryComponent::GetViewModel() const {
    return ViewModel;
}

int32 UMythicInventoryComponent::GetItemCount(UItemDefinition *RequiredItem) const {
    if (!RequiredItem) {
        return 0; // Early return for null definition
    }

    int32 count = 0;
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

    int32 count = 0;

    if (ItemInstance->GetStacks() <= Amount) {
        count += ItemInstance->GetStacks();
        ItemInstance->Destroy();
    }
    else {
        ItemInstance->SetStackSize(ItemInstance->GetStacks() - Amount);
        count += Amount;
    }

    UE_LOG(Myth, Verbose, TEXT("Removed %d items from inventory"), count);
}

void UMythicInventoryComponent::ServerRemoveItemByDefinition_Implementation(UItemDefinition *ItemDef, int32 Amount) {
    // Remove the given amount of items from the inventory that match the given item definition. Returns the amount of items that were removed.
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("RemoveItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("RemoveItem:: Called without Authority!"));
    checkf(ItemDef != nullptr, TEXT("RemoveItem:: Invalid ItemDef!"));
    checkf(Amount > 0, TEXT("RemoveItem:: Invalid Amount!"));

    int32 RemovedSoFar = 0;
    for (int32 i = 0; i < Slots.Items.Num(); ++i) {
        FMythicInventorySlotEntry &Slot = Slots.Items[i];
        UMythicItemInstance *item = Slot.SlottedItemInstance;
        if (!item) {
            continue;
        }

        if (item->GetItemDefinition() == ItemDef) {
            const int32 ItemStacks = item->GetStacks();
            const int32 RemainingToRemove = Amount - RemovedSoFar;

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

    UE_LOG(Myth, Verbose, TEXT("Removed %d items from inventory"), RemovedSoFar);
}

bool UMythicInventoryComponent::DestroySlot(int32 SlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("Called without Authority!"));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return false; // Invalid index
    }

    FMythicInventorySlotEntry &InSlot = Slots.Items[SlotIndex];
    
    // Server Deactivate
    if (InSlot.SlottedItemInstance && InSlot.bEquipmentSlot) {
        InSlot.SlottedItemInstance->OnInactiveItem();
    }

    if (InSlot.SlottedItemInstance) {
        InSlot.SlottedItemInstance->Destroy();
    }

    Slots.RemoveSlotAt(SlotIndex);

    return true;
}

void UMythicInventoryComponent::DestroyAllSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("DestroySlot:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("DestroySlot:: Called without Authority!"));

    for (int32 i = Slots.Num() - 1; i >= 0; --i) {
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

bool UMythicInventoryComponent::GetSlotEntry(int32 Index, FMythicInventorySlotEntry &OutEntry) const {
    if (Slots.IsValidIndex(Index)) {
        OutEntry = Slots.Items[Index];
        return true;
    }
    return false;
}

void UMythicInventoryComponent::NotifyItemInstanceUpdated(int32 SlotIndex) {
    if (IsValid(ViewModel)) {
        ViewModel->RefreshSlotFromInventory(this, SlotIndex);
    }
    OnSlotUpdated.Broadcast(SlotIndex);
}

void UMythicInventoryComponent::AddSlot(UInventorySlotDefinition *SlotDefinition, int32 Count) {
    if (!SlotDefinition || Count <= 0) {
        return;
    }

    // Authority check
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Warning, TEXT("AddSlot called without authority!"));
        return;
    }

    for (int32 i = 0; i < Count; ++i) {
        FMythicInventorySlotEntry NewSlot;
        NewSlot.SlotDefinition = SlotDefinition;
        NewSlot.bEquipmentSlot = false;

        Slots.AddSlot(NewSlot);
    }

    UE_LOG(Myth, Verbose, TEXT("Added %d slots of type %s"), Count, *SlotDefinition->GetName());
}

bool UMythicInventoryComponent::RemoveSlot(UInventorySlotDefinition *SlotDefinition, int32 Count, bool bDropItems) {
    if (!SlotDefinition || Count <= 0) {
        return false;
    }

    // Authority check
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Warning, TEXT("RemoveSlot called without authority!"));
        return false;
    }

    int32 RemovedCount = 0;

    // Prioritize removal of slots with no items
    for (int32 i = Slots.Items.Num() - 1; i >= 0; --i) {
        if (RemovedCount >= Count) {
            break;
        }

        if (Slots.Items[i].SlotDefinition == SlotDefinition && Slots.Items[i].SlottedItemInstance == nullptr) {
            Slots.RemoveSlotAt(i);
            RemovedCount++;
        }
    }

    for (int32 i = Slots.Items.Num() - 1; i >= 0; --i) {
        if (RemovedCount >= Count) {
            break;
        }

        if (Slots.Items[i].SlotDefinition == SlotDefinition) {
            // Drop or destroy any item in the slot
            if (Slots.Items[i].SlottedItemInstance) {
                if (bDropItems) {
                    DropItem(i, GetOwner()->GetActorLocation());
                }
                else {
                    Slots.Items[i].SlottedItemInstance->Destroy();
                }
            }

            Slots.RemoveSlotAt(i);
            RemovedCount++;
        }
    }

    return RemovedCount > 0;
}
