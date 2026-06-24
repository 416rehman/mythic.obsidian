// 


#include "MythicInventoryComponent.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "MythicItemInstance.h"
#include "Mythic/Mythic.h"
#include "Mythic/Itemization/Loot/MythicLootManagerSubsystem.h"
#include "ViewModels/InventoryVM.h"
#include "ItemDefinition.h"
#include "Mythic/Player/MythicPlayerController.h"
#include "Mythic/Player/MythicCharacter.h"
#include "AbilitySystemGlobals.h"      // equip-requirement: resolve the owner's ASC
#include "AbilitySystemComponent.h"    // ...to read its owned gameplay tags
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Fragments/ActionableItemFragment.h"
#include "Mythic/Itemization/MythicTags_Inventory.h"

UMythicInventoryComponent::UMythicInventoryComponent(const FObjectInitializer &OI) :
    Super(OI) {
    SetIsReplicatedByDefault(true);
    SetIsReplicated(true);
    this->bReplicateUsingRegisteredSubObjectList = true;

    // Set owner on the FastArray for client-side callbacks
    Slots.SetOwningInventory(this);
}

void FMythicInventorySlotEntry::ClientUpdateActiveState(UMythicInventoryComponent* Owner) {
    bool bChanged = ClientLastKnownItem != SlottedItemInstance;
    UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Slot Item: %s, LastKnown: %s, bEquipmentSlot: %d, Changed: %d"),
           SlottedItemInstance ? *SlottedItemInstance->GetName() : TEXT("Null"),
           ClientLastKnownItem ? *ClientLastKnownItem->GetName() : TEXT("Null"),
           bEquipmentSlot,
           bChanged);

    // handle visual mesh deactivation and activation if item slot content changed
    if (bChanged) {
        // deactivate old item and remove its local visual mesh from the character
        if (IsValid(ClientLastKnownItem)) {
            UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Deactivating Old Item: %s"), *ClientLastKnownItem->GetName());
            ClientLastKnownItem->OnClientInactiveItem();

            if (bEquipmentSlot && Owner && Owner->GetOwner()) {
                if (AMythicCharacter* CharOwner = Cast<AMythicCharacter>(Owner->GetOwner())) {
                    if (SlotDefinition) {
                        CharOwner->RemoveLocalEquipmentMesh(SlotDefinition->SlotType);
                    }
                }
            }
        }

        // activate new item and spawn its local visual mesh on the character if slotted as equipment
        if (bEquipmentSlot && IsValid(SlottedItemInstance)) {
            UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Activating New Item: %s"), *SlottedItemInstance->GetName());
            SlottedItemInstance->OnClientActiveItem();

            if (Owner && Owner->GetOwner()) {
                if (AMythicCharacter* CharOwner = Cast<AMythicCharacter>(Owner->GetOwner())) {
                    if (SlotDefinition) {
                        if (UItemDefinition* ItemDef = SlottedItemInstance->GetItemDefinition()) {
                            if (USkeletalMesh* EquipMesh = ItemDef->EquippedMesh.LoadSynchronous()) {
                                CharOwner->ApplyLocalEquipmentMesh(EquipMesh, SlotDefinition->SlotType);
                            }
                        }
                    }
                }
            }
        }

        // keep track of the slotted item for the next change evaluation
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

        // Re-sync the replicated SlotIndex back-pointer of every entry shifted down by the stable RemoveAt. Each slotted
        // item caches its index in the replicated SlotIndex (set via SetInventory on insertion); a mid-array removal
        // (e.g. RemoveSlot dropping an EMPTY slot that sits before occupied ones) would otherwise leave trailing items
        // pointing at the WRONG slot — GetSlot()/ReleaseFromSlot/OnDestroyed would then clear or release the wrong slot.
        for (int32 i = Index; i < Items.Num(); ++i) {
            if (IsValid(Items[i].SlottedItemInstance)) {
                Items[i].SlottedItemInstance->SetInventory(Owner, i);
            }
            MarkItemDirty(Items[i]);
        }
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
        UE_LOG(Myth, Warning, TEXT("[InitializeSlots] No profile assigned to inventory component on %s"), *lOwner->GetName());
        return;
    }

    // Create slots from groups
    for (const auto &GroupPair : InventoryProfile->SlotGroups) {
        const FGameplayTag &GroupTag = GroupPair.Key;
        const FInventorySlotGroup &Group = GroupPair.Value;

        int32 EntryIndex = 0;
        for (const FInventoryProfileEntry &Entry : Group.Slots) {
            if (!Entry.SlotDefinition) {
                UE_LOG(Myth, Error, TEXT("InitializeSlots: SlotDefinition is null in group %s for owner %s"),
                       *GroupTag.ToString(), *lOwner->GetName());
                ++EntryIndex;
                continue;
            }

            for (int32 i = 0; i < Entry.Count; ++i) {
                FMythicInventorySlotEntry SlotEntry;
                SlotEntry.SlotDefinition = Entry.SlotDefinition;
                SlotEntry.bEquipmentSlot = Group.bIsEquipmentGroup;
                SlotEntry.GroupTag = GroupTag;
                SlotEntry.EntryIndex = EntryIndex;
                SlotEntry.bRequireUniqueInEntry = Entry.bRequireUniqueItems;
                SlotEntry.bCanPlayerTake = Group.bCanPlayerTake;
                SlotEntry.bCanPlayerPut = Group.bCanPlayerPut;

                Slots.AddSlot(SlotEntry);
            }
            ++EntryIndex;
        }
    }

    const int32 NewSlotsSize = Slots.Num();
    UE_LOG(Myth, Verbose, TEXT("Initialized Inventory from %d to %d slots"), OldSlotsSize, NewSlotsSize);

    if (OldSlotsSize != NewSlotsSize) {
        OnInventorySizeChanged.Broadcast(NewSlotsSize, OldSlotsSize);
    }
    else {
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

bool UMythicInventoryComponent::CanSlotAcceptItem(int32 SlotIndex, UMythicItemInstance *ItemInstance, bool bFromPlayer) const {
    if (!Slots.IsValidIndex(SlotIndex) || !ItemInstance) {
        return false;
    }
    const FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    if (bFromPlayer && !Slot.bCanPlayerPut) {
        return false;
    }

    if (!SlotWhitelistAccepts(SlotIndex, ItemInstance)) {
        return false;
    }

    // Equip-requirement: a player equipping into an equipment slot must own the item's RequiredEquipTag (if any).
    // Empty tag → always passes (default). Only gates player-driven equips — the loot/transfer systems are unaffected.
    if (bFromPlayer && Slot.bEquipmentSlot) {
        if (const UItemDefinition *Def = ItemInstance->GetItemDefinition()) {
            if (Def->RequiredEquipTag.IsValid()) {
                FGameplayTagContainer OwnerTags;
                if (UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner())) {
                    ASC->GetOwnedGameplayTags(OwnerTags);
                }
                if (!MeetsEquipRequirement(Def->RequiredEquipTag, OwnerTags)) {
                    return false;
                }
            }
        }
    }

    if (Slot.bRequireUniqueInEntry && ItemInstance->GetItemDefinition()) {
        for (int32 i = 0; i < Slots.Num(); ++i) {
            if (i == SlotIndex) {
                continue;
            }
            const FMythicInventorySlotEntry &OtherSlot = Slots.Items[i];
            if (OtherSlot.GroupTag == Slot.GroupTag && OtherSlot.EntryIndex == Slot.EntryIndex && OtherSlot.SlottedItemInstance && OtherSlot.SlottedItemInstance
                ->GetItemDefinition() == ItemInstance->GetItemDefinition()) {
                return false;
            }
        }
    }

    return true;
}

bool UMythicInventoryComponent::MeetsEquipRequirement(const FGameplayTag &RequiredTag, const FGameplayTagContainer &OwnerTags) {
    // No requirement → always equippable (the default). Otherwise the owner must own the required tag.
    return !RequiredTag.IsValid() || OwnerTags.HasTag(RequiredTag);
}

bool UMythicInventoryComponent::SlotWhitelistAccepts(int32 SlotIndex, const UMythicItemInstance *Inst) const {
    if (!Slots.IsValidIndex(SlotIndex) || !Inst) {
        return false;
    }
    const FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    // No definition / no whitelist => accepts all.
    if (!Slot.SlotDefinition || Slot.SlotDefinition->WhitelistedItemTypes.Num() == 0) {
        return true;
    }

    FGameplayTagContainer Probe;
    Inst->GetTypeProbe(Probe);
    return Probe.HasAny(Slot.SlotDefinition->WhitelistedItemTypes);
}

UMythicItemInstance *UMythicInventoryComponent::ReleaseFromSlot(int32 SlotIndex) {
    checkf(GetOwner() && GetOwner()->HasAuthority(), TEXT("ReleaseFromSlot is server-only."));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return nullptr;
    }

    UMythicItemInstance *Inst = Slots.Items[SlotIndex].SlottedItemInstance;
    if (!Inst) {
        return nullptr;
    }

    // Clear the slot (deactivates equipment, fires the UI notify) without touching the instance object,
    // then detach the instance's back-pointers so it is owned by the caller / loot graph, NOT destroyed.
    SetItemInSlot(SlotIndex, nullptr);
    Inst->SetInventory(nullptr, INDEX_NONE);
    return Inst;
}

UMythicItemInstance *UMythicInventoryComponent::GetItem(int32 SlotIndex) {
    return Slots.GetItemInSlot(SlotIndex);
}

bool UMythicInventoryComponent::TryTransferToSlot(UMythicItemInstance *ItemInstance, int32 TargetSlotIndex) {
    if (!ItemInstance) {
        // Mirrors the null guards on the sibling add paths (AddToSlot / AddToAnySlot). Current callers all guard, so
        // this is defensive against a future/BP caller passing null rather than crashing the server on the derefs below.
        return false;
    }
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

    // Whitelist check (effective type: {def ItemType} ∪ ItemTags).
    if (Slot.SlotDefinition && (Slot.SlotDefinition->WhitelistedItemTypes.Num() > 0)) {
        if (!NewItemInstance->GetItemDefinition()) {
            UE_LOG(Myth, Error, TEXT("SetItemInSlotInternal: ItemDefinition is null for item %s"), *NewItemInstance->GetName());
            return false;
        }
        if (!SlotWhitelistAccepts(SlotIndex, NewItemInstance)) {
            UE_LOG(Myth, Verbose, TEXT("SetItemInSlotInternal: Item %s of type %s is not whitelisted for slot %d"),
                   *NewItemInstance->GetName(),
                   *NewItemInstance->GetItemDefinition()->ItemType.ToString(),
                   SlotIndex);
            return false;
        }
    }

    // Uniqueness check for slots requiring unique items within the same entry
    if (Slot.bRequireUniqueInEntry && NewItemInstance->GetItemDefinition()) {
        for (int32 i = 0; i < Slots.Num(); ++i) {
            if (i == SlotIndex) {
                continue;
            }
            const FMythicInventorySlotEntry &OtherSlot = Slots.Items[i];
            // Check if same group and same entry index
            if (OtherSlot.GroupTag == Slot.GroupTag &&
                OtherSlot.EntryIndex == Slot.EntryIndex &&
                OtherSlot.SlottedItemInstance &&
                OtherSlot.SlottedItemInstance->GetItemDefinition() == NewItemInstance->GetItemDefinition()) {
                UE_LOG(Myth, Warning, TEXT("SetItemInSlotInternal: Item %s already exists in entry (uniqueness required)"),
                       *NewItemInstance->GetName());
                return false;
            }
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
    // Capture name + rarity BEFORE the add: AddToAnySlot can Destroy() ItemInstance on a full stack-merge.
    auto PickupDef = ItemInstance->GetItemDefinition();
    auto AmountAdded = AddToAnySlot(ItemInstance);

    // Genuine acquisition callout: fire ONLY when stacks were actually gained (AmountAdded > 0), player-owned inventory
    // only (the guarded Cast no-ops for container/merchant inventories). This entry is a true grant (quest/loot/craft) —
    // inventory MOVES go through SendItem, not here, so they never fire a pickup callout.
    if (AmountAdded > 0 && PickupDef) {
        if (AMythicPlayerController *OwningPC = Cast<AMythicPlayerController>(GetOwner())) {
            OwningPC->ClientNotifyLootPickup(PickupDef->Name, AmountAdded, UItemDefinition::GetRarityColor(PickupDef->Rarity));
            OwningPC->NotifyItemAcquired(PickupDef, AmountAdded); // server: drive "collect N" objectives via GAS event
        }
    }

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

int32 UMythicInventoryComponent::AddToAnySlot(UMythicItemInstance *ItemInstance, bool bFromPlayer) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    const int32 original_qty = ItemInstance->GetStacks();

    // Can be stacked? Guard GetItemDefinition() — ItemDefinition is a replicated + SaveGame UPROPERTY that can resolve
    // to null on load (an unresolvable data-asset ref); a raw deref here would crash, so treat a null def as
    // non-stackable and fall through to fresh-slot placement (mirrors the sibling add paths' null-checks).
    const UItemDefinition *StackDef = ItemInstance->GetItemDefinition();
    // Merge into existing partial stacks if this is a STACKABLE TYPE — not "if the incoming isn't full". Merging drains
    // the incoming into existing stacks, so the incoming's own fullness is irrelevant; the old `> GetStacks()` gate made
    // a full incoming stack skip merging entirely, so it never topped off partials and wasted slots. (isStackableWith below
    // still guards real compatibility.)
    if (StackDef && ShouldAttemptStackMerge(StackDef->StackSizeMax)) {
        // Add to existing stack
        for (int32 i = 0; i < Slots.Num(); ++i) {
            auto ItemInSlot = Slots.Items[i].SlottedItemInstance;
            if (bFromPlayer && !Slots.Items[i].bCanPlayerPut) {
                continue;
            }

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

    // Place the remaining quantity into empty slots, SPLITTING + CLAMPING to StackSizeMax per slot. Dumping the whole
    // remainder into one slot (the old behavior) produced an over-cap stack that SetStackSize would later silently clamp
    // — destroying the surplus — and made AddToAnySlot report a full add so AddItem / ConversionStation never spilled
    // the overflow. Any true surplus is left on ItemInstance so the caller (AddItem) spawns the overflow WorldItem.
    const int32 MaxStack = (StackDef && StackDef->StackSizeMax > 0) ? StackDef->StackSizeMax : 1;
    for (int32 i = 0; i < Slots.Num(); ++i) {
        const int32 remaining = ItemInstance->GetStacks();
        if (remaining <= 0) {
            break;
        }
        if (Slots.Items[i].SlottedItemInstance != nullptr) {
            continue;
        }
        if (!CanSlotAcceptItem(i, ItemInstance, bFromPlayer)) {
            continue;
        }
        
        if (remaining <= MaxStack) {
            // Whole remainder fits in this slot.
            if (TryTransferToSlot(ItemInstance, i)) {
                return original_qty;
            }
            continue;
        }

        // Remainder exceeds one stack: peel off a capped copy for this slot, keep the rest on ItemInstance.
        UMythicItemInstance *Split = DuplicateObject<UMythicItemInstance>(ItemInstance, this);
        Split->SetStackSize(MaxStack);
        if (TryTransferToSlot(Split, i)) {
            ItemInstance->SetStackSize(remaining - MaxStack);
        }
        else if (IsValid(Split)) {
            Split->Destroy();
        }
    }

    return original_qty - ItemInstance->GetStacks();
}

int32 UMythicInventoryComponent::AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex, bool bFromPlayer) {
    if (!ItemInstance) {
        UE_LOG(Myth, Verbose, TEXT("AddToSlot: ItemInstance is null"));
        return 0;
    }
    const int32 OriginalQty = ItemInstance->GetStacks();
    if (Slots.IsValidIndex(SlotIndex)) {
        if (bFromPlayer && !Slots.Items[SlotIndex].bCanPlayerPut) {
            return 0;
        }

        auto SlottedItem = Slots.Items[SlotIndex].SlottedItemInstance;
        // If the target slot is empty, move the entire item there
        if (SlottedItem == nullptr) {
            if (!CanSlotAcceptItem(SlotIndex, ItemInstance, bFromPlayer)) {
                return 0;
            }
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

int32 UMythicInventoryComponent::ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, int32 TargetSlotIndex, bool bFromPlayer) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    if (TargetSlotIndex == INDEX_NONE) {
        return AddToAnySlot(ItemInstance, bFromPlayer);
    }

    return AddToSlot(ItemInstance, TargetSlotIndex, bFromPlayer);
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
    int32 amountSent = TargetInventory->ReceiveItem(itemInstance, TargetSlotIndex, true);

    // Clear source slot if item was fully consumed
    if (!IsValid(itemInstance) || itemInstance->GetStacks() == 0) {
        SetItemInSlot(SlotIndex, nullptr);
    }

    return amountSent;
}

bool UMythicInventoryComponent::CanPlayerTakeFromSlot(int32 SlotIndex) const {
    return Slots.IsValidIndex(SlotIndex) && Slots.Items[SlotIndex].bCanPlayerTake;
}

bool UMythicInventoryComponent::DropItem(int32 SlotIndex, const FVector &location, const float radius, AController *TargetRecipient) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("DropItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("DropItem:: Called without Authority!"));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return false;
    }

    const FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    // Check if slot is protected
    if (!Slot.bCanPlayerTake) {
        UE_LOG(Myth, Warning, TEXT("DropItem: Cannot drop item from protected group"));
        return false;
    }

    auto item_instance = Slot.SlottedItemInstance;
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
    // Capture name + rarity BEFORE the add: AddToAnySlot can Destroy() copied_item_instance on a full pickup.
    auto PickupDef = copied_item_instance->GetItemDefinition();
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

    // Genuine world-item pickup callout: fire ONLY when stacks were actually gained, player-owned inventory only.
    if (AmountAdded > 0 && PickupDef) {
        if (AMythicPlayerController *OwningPC = Cast<AMythicPlayerController>(GetOwner())) {
            OwningPC->ClientNotifyLootPickup(PickupDef->Name, AmountAdded, UItemDefinition::GetRarityColor(PickupDef->Rarity));
            OwningPC->NotifyItemAcquired(PickupDef, AmountAdded); // server: drive "collect N" objectives via GAS event
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
            Slots.Items[idx].ClientUpdateActiveState(this);
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
            Slots.Items[idx].ClientUpdateActiveState(this);
        }

        OnSlotUpdated.Broadcast(idx);
        if (IsValid(ViewModel)) {
            ViewModel->RefreshSlotFromInventory(this, idx);
        }
    }
}

void UMythicInventoryComponent::HandleSlotsRemoved(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    // deactivate items that are about to be removed
    for (int32 idx : RemovedIndices) {
        if (Slots.IsValidIndex(idx)) {
            FMythicInventorySlotEntry &Slot = Slots.Items[idx];
            if (IsValid(Slot.ClientLastKnownItem)) {
                Slot.ClientLastKnownItem->OnClientInactiveItem();

                if (Slot.bEquipmentSlot && GetOwner()) {
                    if (AMythicCharacter* CharOwner = Cast<AMythicCharacter>(GetOwner())) {
                        if (Slot.SlotDefinition) {
                            CharOwner->RemoveLocalEquipmentMesh(Slot.SlotDefinition->SlotType);
                        }
                    }
                }

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

float UMythicInventoryComponent::ComputeSlotWeight(float UnitWeight, int32 StackCount) {
    return FMath::Max(0.0f, UnitWeight) * static_cast<float>(FMath::Max(0, StackCount));
}

float UMythicInventoryComponent::GetTotalCarriedWeight() const {
    float Total = 0.0f;
    for (const FMythicInventorySlotEntry &Entry : Slots.GetItems()) {
        const UMythicItemInstance *Inst = Entry.SlottedItemInstance;
        if (Inst && Inst->GetItemDefinition()) {
            Total += ComputeSlotWeight(Inst->GetItemDefinition()->Weight, Inst->GetStacks());
        }
    }
    return Total;
}

int32 UMythicInventoryComponent::GetTotalCurrency() const {
    int32 Total = 0;
    for (const FMythicInventorySlotEntry &Entry : Slots.GetItems()) {
        const UMythicItemInstance *Inst = Entry.SlottedItemInstance;
        if (Inst && Inst->GetItemDefinition() && Inst->GetItemDefinition()->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            Total += FMath::Max(0, Inst->GetStacks());
        }
    }
    return Total;
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

void UMythicInventoryComponent::ServerSplitStack_Implementation(int32 SourceSlotIndex, int32 SplitAmount) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!Slots.IsValidIndex(SourceSlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: invalid source slot %d"), SourceSlotIndex);
        return;
    }

    const FMythicInventorySlotEntry &SourceSlot = Slots.Items[SourceSlotIndex];

    // cannot split from equipment slots
    if (SourceSlot.bEquipmentSlot) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: cannot split from equipment slot %d"), SourceSlotIndex);
        return;
    }

    UMythicItemInstance *SourceItem = SourceSlot.SlottedItemInstance;
    if (!SourceItem) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: source slot %d is empty"), SourceSlotIndex);
        return;
    }

    if (SplitAmount <= 0 || SplitAmount >= SourceItem->GetStacks()) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: invalid split amount %d for stack of %d"), SplitAmount, SourceItem->GetStacks());
        return;
    }

    // find first empty slot in the same group
    int32 TargetSlotIndex = INDEX_NONE;
    for (int32 i = 0; i < Slots.Num(); ++i) {
        if (i == SourceSlotIndex) {
            continue;
        }
        const FMythicInventorySlotEntry &CandidateSlot = Slots.Items[i];
        if (CandidateSlot.GroupTag == SourceSlot.GroupTag && !CandidateSlot.bEquipmentSlot && !CandidateSlot.SlottedItemInstance) {
            if (SlotWhitelistAccepts(i, SourceItem)) {
                TargetSlotIndex = i;
                break;
            }
        }
    }

    if (TargetSlotIndex == INDEX_NONE) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: no empty slot in group %s for split"), *SourceSlot.GroupTag.ToString());
        return;
    }

    // create a new item instance via the loot manager with the same definition
    UMythicLootManagerSubsystem *LootManager = lOwner->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
    if (!LootManager) {
        UE_LOG(Myth, Error, TEXT("ServerSplitStack: no LootManagerSubsystem"));
        return;
    }

    UMythicItemInstance *NewItem = LootManager->Create(
        SourceItem->GetItemDefinition(),
        SplitAmount,
        Cast<AController>(lOwner),
        SourceItem->GetItemLevel()
    );

    if (!NewItem) {
        UE_LOG(Myth, Error, TEXT("ServerSplitStack: failed to create split item"));
        return;
    }

    // place the new item into the target slot
    if (!SetItemInSlot(TargetSlotIndex, NewItem)) {
        UE_LOG(Myth, Error, TEXT("ServerSplitStack: failed to place split item in slot %d"), TargetSlotIndex);
        NewItem->Destroy();
        return;
    }

    // decrement source stack
    SourceItem->SetStackSize(SourceItem->GetStacks() - SplitAmount);
}

void UMythicInventoryComponent::ServerSwapSlots_Implementation(int32 SlotA, int32 SlotB) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (SlotA == SlotB) {
        return;
    }

    if (!Slots.IsValidIndex(SlotA) || !Slots.IsValidIndex(SlotB)) {
        UE_LOG(Myth, Warning, TEXT("ServerSwapSlots: invalid slot indices %d / %d"), SlotA, SlotB);
        return;
    }

    UMythicItemInstance *ItemA = Slots.Items[SlotA].SlottedItemInstance;
    UMythicItemInstance *ItemB = Slots.Items[SlotB].SlottedItemInstance;

    // both empty, nothing to do
    if (!ItemA && !ItemB) {
        return;
    }

    // one slot empty: simple move
    if (!ItemA) {
        if (!SlotWhitelistAccepts(SlotA, ItemB)) {
            UE_LOG(Myth, Warning, TEXT("ServerSwapSlots: slot %d does not accept item from slot %d"), SlotA, SlotB);
            return;
        }

        // deactivate from equipment slot B if applicable
        if (Slots.Items[SlotB].bEquipmentSlot && IsValid(ItemB)) {
            ItemB->OnInactiveItem();
        }

        // clear slot B without deactivating again (we just did it)
        Slots.ModifySlotAtIndex(SlotB, [](FMythicInventorySlotEntry &SlotData) {
            SlotData.SlottedItemInstance = nullptr;
        });
        ItemB->SetInventory(nullptr, INDEX_NONE);
        NotifyItemInstanceUpdated(SlotB);

        // place into slot A
        SetItemInSlot(SlotA, ItemB);
        return;
    }

    if (!ItemB) {
        if (!SlotWhitelistAccepts(SlotB, ItemA)) {
            UE_LOG(Myth, Warning, TEXT("ServerSwapSlots: slot %d does not accept item from slot %d"), SlotB, SlotA);
            return;
        }

        // deactivate from equipment slot A if applicable
        if (Slots.Items[SlotA].bEquipmentSlot && IsValid(ItemA)) {
            ItemA->OnInactiveItem();
        }

        // clear slot A
        Slots.ModifySlotAtIndex(SlotA, [](FMythicInventorySlotEntry &SlotData) {
            SlotData.SlottedItemInstance = nullptr;
        });
        ItemA->SetInventory(nullptr, INDEX_NONE);
        NotifyItemInstanceUpdated(SlotA);

        // place into slot B
        SetItemInSlot(SlotB, ItemA);
        return;
    }

    // both slots occupied: verify cross-acceptance
    if (!SlotWhitelistAccepts(SlotA, ItemB) || !SlotWhitelistAccepts(SlotB, ItemA)) {
        UE_LOG(Myth, Warning, TEXT("ServerSwapSlots: whitelist rejection for swap between %d and %d"), SlotA, SlotB);
        return;
    }

    // deactivate both if in equipment slots
    if (Slots.Items[SlotA].bEquipmentSlot && IsValid(ItemA)) {
        ItemA->OnInactiveItem();
    }
    if (Slots.Items[SlotB].bEquipmentSlot && IsValid(ItemB)) {
        ItemB->OnInactiveItem();
    }

    // detach both items from their slots
    ItemA->SetInventory(nullptr, INDEX_NONE);
    ItemB->SetInventory(nullptr, INDEX_NONE);

    // swap the pointers, set inventory back-pointers, and activate equipment if needed
    Slots.ModifySlotAtIndex(SlotA, [this, ItemB, SlotA](FMythicInventorySlotEntry &Slot) {
        Slot.SlottedItemInstance = ItemB;
        ItemB->SetOwner(this);
        ItemB->SetInventory(this, SlotA);
        Slot.ServerUpdateActiveState();
    });
    NotifyItemInstanceUpdated(SlotA);

    Slots.ModifySlotAtIndex(SlotB, [this, ItemA, SlotB](FMythicInventorySlotEntry &Slot) {
        Slot.SlottedItemInstance = ItemA;
        ItemA->SetOwner(this);
        ItemA->SetInventory(this, SlotB);
        Slot.ServerUpdateActiveState();
    });
    NotifyItemInstanceUpdated(SlotB);
}

void UMythicInventoryComponent::ServerQuickMoveToInventory_Implementation(int32 SourceSlotIndex, UMythicInventoryComponent *TargetInventory) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!TargetInventory) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: null target inventory"));
        return;
    }

    if (TargetInventory == this) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: target is self"));
        return;
    }

    if (!Slots.IsValidIndex(SourceSlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: invalid source slot %d"), SourceSlotIndex);
        return;
    }

    if (!Slots.Items[SourceSlotIndex].bCanPlayerTake) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: slot %d does not allow player take"), SourceSlotIndex);
        return;
    }

    // release the item from the source slot (handles deactivation)
    UMythicItemInstance *Released = ReleaseFromSlot(SourceSlotIndex);
    if (!Released) {
        return;
    }

    int32 OriginalQty = Released->GetStacks();
    int32 Added = TargetInventory->AddToAnySlot(Released, true);

    // if nothing was added, put it back
    if (Added == 0) {
        SetItemInSlot(SourceSlotIndex, Released);
        return;
    }

    // partial add: the item is still valid with reduced stacks, put remainder back
    if (Added < OriginalQty && IsValid(Released)) {
        SetItemInSlot(SourceSlotIndex, Released);
    }
}

void UMythicInventoryComponent::ServerSortGroup_Implementation(FGameplayTag GroupTag, ESortMode Mode) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!GroupTag.IsValid()) {
        return;
    }

    TArray<int32> GroupSlotIndices;
    TArray<UMythicItemInstance *> GroupItems;

    for (int32 i = 0; i < Slots.Num(); ++i) {
        const FMythicInventorySlotEntry &Slot = Slots.Items[i];
        if (Slot.GroupTag != GroupTag) {
            continue;
        }
        if (Slot.bEquipmentSlot) {
            UE_LOG(Myth, Warning, TEXT("ServerSortGroup: cannot sort equipment group %s"), *GroupTag.ToString());
            return;
        }
        GroupSlotIndices.Add(i);
        if (Slot.SlottedItemInstance) {
            GroupItems.Add(Slot.SlottedItemInstance);
        }
    }

    if (GroupItems.Num() <= 1) {
        return;
    }

    // sort items by the specified mode
    GroupItems.Sort([Mode](const UMythicItemInstance &A, const UMythicItemInstance &B) {
        const UItemDefinition *DefA = A.GetItemDefinition();
        const UItemDefinition *DefB = B.GetItemDefinition();
        if (!DefA || !DefB) {
            return DefA != nullptr;
        }

        switch (Mode) {
            case ESortMode::ByRarity:
                return static_cast<int32>(DefA->Rarity) > static_cast<int32>(DefB->Rarity);
            case ESortMode::ByType:
                return DefA->ItemType.ToString() < DefB->ItemType.ToString();
            case ESortMode::ByName:
                return DefA->Name.ToString() < DefB->Name.ToString();
            case ESortMode::ByValue:
                return DefA->Value > DefB->Value;
            case ESortMode::ByWeight:
                return DefA->Weight > DefB->Weight;
        }
        return false;
    });

    // detach all items from their slots
    for (int32 SlotIdx : GroupSlotIndices) {
        UMythicItemInstance *Inst = Slots.Items[SlotIdx].SlottedItemInstance;
        if (Inst) {
            Inst->SetInventory(nullptr, INDEX_NONE);
            Slots.ModifySlotAtIndex(SlotIdx, [](FMythicInventorySlotEntry &SlotData) {
                SlotData.SlottedItemInstance = nullptr;
            });
        }
    }

    // reassign sorted items to group slots in order
    int32 ItemIdx = 0;
    for (int32 SlotIdx : GroupSlotIndices) {
        if (ItemIdx >= GroupItems.Num()) {
            break;
        }

        UMythicItemInstance *Item = GroupItems[ItemIdx];
        Slots.ModifySlotAtIndex(SlotIdx, [this, Item, SlotIdx](FMythicInventorySlotEntry &Slot) {
            Slot.SlottedItemInstance = Item;
            Item->SetOwner(this);
            Item->SetInventory(this, SlotIdx);
        });
        NotifyItemInstanceUpdated(SlotIdx);
        ++ItemIdx;
    }

    // notify remaining empty slots
    for (int32 i = ItemIdx; i < GroupSlotIndices.Num(); ++i) {
        NotifyItemInstanceUpdated(GroupSlotIndices[i]);
    }
}

void UMythicInventoryComponent::ServerDepositAll_Implementation(UMythicInventoryComponent *Target, FGameplayTag OptionalTypeFilter) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!Target || Target == this) {
        return;
    }

    // iterate in reverse so slot index removal doesn't invalidate later indices
    for (int32 i = Slots.Num() - 1; i >= 0; --i) {
        const FMythicInventorySlotEntry &Slot = Slots.Items[i];

        // skip equipment slots
        if (Slot.bEquipmentSlot) {
            continue;
        }

        // skip protected slots
        if (!Slot.bCanPlayerTake) {
            continue;
        }

        UMythicItemInstance *Item = Slot.SlottedItemInstance;
        if (!Item) {
            continue;
        }

        // apply optional type filter
        if (OptionalTypeFilter.IsValid()) {
            const UItemDefinition *Def = Item->GetItemDefinition();
            if (!Def || !Def->ItemType.MatchesTag(OptionalTypeFilter)) {
                continue;
            }
        }

        // release and move to target
        UMythicItemInstance *Released = ReleaseFromSlot(i);
        if (!Released) {
            continue;
        }

        int32 OriginalQty = Released->GetStacks();
        int32 Added = Target->AddToAnySlot(Released, true);

        // if nothing transferred, put it back
        if (Added == 0) {
            SetItemInSlot(i, Released);
            continue;
        }

        // partial transfer: put remainder back in source
        if (Added < OriginalQty && IsValid(Released)) {
            SetItemInSlot(i, Released);
        }
    }
}

void UMythicInventoryComponent::ServerUseItemInSlot_Implementation(int32 SlotIndex) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!Slots.IsValidIndex(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: invalid slot %d"), SlotIndex);
        return;
    }

    UMythicItemInstance *Item = Slots.Items[SlotIndex].SlottedItemInstance;
    if (!Item) {
        UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: slot %d is empty"), SlotIndex);
        return;
    }

    // the item must have the InInventory action type tag
    if (!Item->HasTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def || !Def->ItemType.MatchesTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
            UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: item in slot %d does not support in-inventory use"), SlotIndex);
            return;
        }
    }

    // find actionable fragment and execute its generic action
    // GetFragment returns const T* but ExecuteGenericAction mutates state (server authority)
    if (const auto *ActionFrag = Item->GetFragment<UActionableItemFragment>()) {
        const_cast<UActionableItemFragment *>(ActionFrag)->ExecuteGenericAction(Item);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: no actionable fragment found on item in slot %d"), SlotIndex);
    }
}

bool UMythicInventoryComponent::CanUseItemInSlot(int32 SlotIndex) const {
    if (!Slots.IsValidIndex(SlotIndex)) {
        return false;
    }

    UMythicItemInstance *Item = Slots.Items[SlotIndex].SlottedItemInstance;
    if (!Item) {
        return false;
    }

    // check for the InInventory action type tag on the item's tags or definition type
    if (!Item->HasTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def || !Def->ItemType.MatchesTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
            return false;
        }
    }

    // must have an actionable fragment
    return Item->GetFragment<UActionableItemFragment>() != nullptr;
}
