// 


#include "InventoryVM.h"

#include "Mythic.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Engine/Texture2D.h"

// ---------------- UInventoryTabVM -----------------
void UInventoryTabVM::SetTabName(FText InTabName) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TabName, InTabName)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabName);
        UE_LOG(Myth, Verbose, TEXT("Tab name set to: %s"), *InTabName.ToString());
    }
}

FText UInventoryTabVM::GetTabName() const {
    return TabName;
}

void UInventoryTabVM::SetTabIcon(UTexture2D *InTabIcon) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TabIcon, InTabIcon)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabIcon);
        UE_LOG(Myth, Verbose, TEXT("Tab icon set: %s"), InTabIcon ? *InTabIcon->GetName() : TEXT("None"));
    }
}

UTexture2D *UInventoryTabVM::GetTabIcon() const {
    return TabIcon;
}

void UInventoryTabVM::SetGroupTag(FGameplayTag InGroupTag) {
    if (UE_MVVM_SET_PROPERTY_VALUE(GroupTag, InGroupTag)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(GroupTag);
    }
}

FGameplayTag UInventoryTabVM::GetGroupTag() const {
    return GroupTag;
}

void UInventoryTabVM::SetIsProtected(bool bInIsProtected) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsProtected, bInIsProtected)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsProtected);
    }
}

bool UInventoryTabVM::GetIsProtected() const {
    return IsProtected;
}

void UInventoryTabVM::SetSlots(TArray<TObjectPtr<UItemSlotVM>> InSlots) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Slots, InSlots)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
        UE_LOG(Myth, Verbose, TEXT("Tab slots updated. Slot count: %d"), InSlots.Num());
    }
}

TArray<TObjectPtr<UItemSlotVM>> UInventoryTabVM::GetSlots() const {
    return Slots;
}

void UInventoryTabVM::Initialize(FText InTabName, UTexture2D *InTabIcon, FGameplayTag InGroupTag, bool bInIsProtected,
                                 TArray<TObjectPtr<UItemSlotVM>> InSlots) {
    SetTabName(InTabName);
    SetTabIcon(InTabIcon);
    SetGroupTag(InGroupTag);
    SetIsProtected(bInIsProtected);
    SetSlots(InSlots);
}

// ---------------- UInventoryVM -----------------
void UInventoryVM::SetInventoryTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs) {
    if (UE_MVVM_SET_PROPERTY_VALUE(InventoryTabs, InTabs)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(InventoryTabs);
        UE_LOG(Myth, Verbose, TEXT("Inventory tabs updated. Tab count: %d"), InTabs.Num());
    }
}

TArray<TObjectPtr<UInventoryTabVM>> UInventoryVM::GetInventoryTabs() const {
    return InventoryTabs;
}

void UInventoryVM::SetEquipmentTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs) {
    if (UE_MVVM_SET_PROPERTY_VALUE(EquipmentTabs, InTabs)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EquipmentTabs);
        UE_LOG(Myth, Verbose, TEXT("Equipment tabs updated. Tab count: %d"), InTabs.Num());
    }
}

TArray<TObjectPtr<UInventoryTabVM>> UInventoryVM::GetEquipmentTabs() const {
    return EquipmentTabs;
}

void UInventoryVM::SetSelectionVM(UInventorySelectionVM *InSelectionVM) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectionVM, InSelectionVM)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectionVM);
        UE_LOG(Myth, Verbose, TEXT("Selection VM set: %s"), InSelectionVM ? *InSelectionVM->GetName() : TEXT("None"));
    }
}

UInventorySelectionVM *UInventoryVM::GetSelectionVM() const {
    return SelectionVM;
}

void UInventoryVM::SetOwningInventoryComponent(UMythicInventoryComponent *InOwningInventoryComponent) {
    if (UE_MVVM_SET_PROPERTY_VALUE(OwningInventoryComponent, InOwningInventoryComponent)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(OwningInventoryComponent);
        UE_LOG(Myth, Verbose, TEXT("Owning inventory component set: %s"), InOwningInventoryComponent ? *InOwningInventoryComponent->GetName() : TEXT("None"));
    }
}

UMythicInventoryComponent *UInventoryVM::GetOwningInventoryComponent() const {
    return OwningInventoryComponent;
}

void UInventoryVM::Clear() {
    if (SelectionVM != nullptr) {
        SelectionVM->ClearSelection();
    }

    AbsoluteIndexToSlotVM.Reset();
    EquipmentTabs.Reset();
    InventoryTabs.Reset();
    UE_LOG(Myth, Log, TEXT("InventoryVM cleared."));
}

TArray<TObjectPtr<UInventoryTabVM>> UInventoryVM::CreateVMs(const TArray<FMythicInventorySlotEntry> &allSlots, TSet<int32> SlotIndices) {
    // Map GroupTag -> TabVM
    TMap<FGameplayTag, TObjectPtr<UInventoryTabVM>> GroupToTabMap;

    auto AsArray = SlotIndices.Array();
    for (int32 i = 0; i < AsArray.Num(); ++i) {
        int32 AbsIndex = AsArray[i];
        const FMythicInventorySlotEntry &Entry = allSlots[AbsIndex];

        if (!Entry.SlotDefinition) {
            continue;
        }

        // Get or create tab for this group
        TObjectPtr<UInventoryTabVM> *FoundTab = GroupToTabMap.Find(Entry.GroupTag);
        if (FoundTab == nullptr) {
            auto NewTabVM = NewObject<UInventoryTabVM>(this);

            // Get group info from Profile if available
            FText GroupName = FText::FromString(Entry.GroupTag.ToString());
            UTexture2D *GroupIcon = nullptr;
            bool bProtected = Entry.bProtectedGroup;

            if (OwningInventoryComponent && OwningInventoryComponent->InventoryProfile) {
                const FInventorySlotGroup *Group = OwningInventoryComponent->InventoryProfile->SlotGroups.Find(Entry.GroupTag);
                if (Group) {
                    GroupName = Group->GroupName.IsEmpty() ? GroupName : Group->GroupName;
                    GroupIcon = Group->Icon;
                    bProtected = Group->bProtectedItems;
                }
            }

            NewTabVM->Initialize(GroupName, GroupIcon, Entry.GroupTag, bProtected, TArray<TObjectPtr<UItemSlotVM>>());
            GroupToTabMap.Add(Entry.GroupTag, NewTabVM);
            FoundTab = GroupToTabMap.Find(Entry.GroupTag);
        }

        // Create slot VM and add to tab
        if (FoundTab && *FoundTab) {
            auto NewSlotVM = NewObject<UItemSlotVM>(this);
            NewSlotVM->Initialize(Entry.SlottedItemInstance, this, Entry.SlotDefinition, AbsIndex);
            (*FoundTab)->Slots.Add(NewSlotVM);

            if (AbsoluteIndexToSlotVM.Num() <= AbsIndex) {
                AbsoluteIndexToSlotVM.SetNum(AbsIndex + 1);
            }
            AbsoluteIndexToSlotVM[AbsIndex] = NewSlotVM;
        }
    }

    // Convert to array and sort by DisplayOrder
    TArray<TObjectPtr<UInventoryTabVM>> TabsArray;
    GroupToTabMap.GenerateValueArray(TabsArray);

    // Sort by DisplayOrder from Profile
    if (OwningInventoryComponent && OwningInventoryComponent->InventoryProfile) {
        Algo::SortBy(TabsArray, [this](const UInventoryTabVM *Tab) -> int32 {
            const FInventorySlotGroup *Group = OwningInventoryComponent->InventoryProfile->SlotGroups.Find(Tab->GroupTag);
            return Group ? Group->DisplayOrder : 0;
        });
    }

    return TabsArray;
}

void UInventoryVM::InitializeFromInventoryComponent(UMythicInventoryComponent *InInventoryComponent) {
    Clear();
    SetOwningInventoryComponent(InInventoryComponent);

    if (InInventoryComponent == nullptr) {
        return;
    }

    const auto AllSlots = InInventoryComponent->GetAllSlots();

    TSet<int32> InventoryIndices;
    TSet<int32> EquipmentIndices;
    for (int32 SlotIdx = 0; SlotIdx < AllSlots.Num(); ++SlotIdx) {
        const FMythicInventorySlotEntry &Entry = AllSlots[SlotIdx];
        if (Entry.SlotDefinition) {
            if (Entry.bEquipmentSlot) {
                EquipmentIndices.Add(SlotIdx);
            }
            else {
                InventoryIndices.Add(SlotIdx);
            }
        }
    }

    SetInventoryTabs(CreateVMs(AllSlots, InventoryIndices));
    SetEquipmentTabs(CreateVMs(AllSlots, EquipmentIndices));

    if (!SelectionVM) {
        SelectionVM = NewObject<UInventorySelectionVM>(this);
    }

    UE_LOG(Myth, Log, TEXT("InventoryVM initialized from inventory component: %s"), *InInventoryComponent->GetName());

    if (InventoryTabs.Num() > 0) {
        auto FirstInvTab = InventoryTabs[0];
        SelectionVM->SetSelectedTabVM(FirstInvTab);
        if (FirstInvTab->Slots.Num() > 0) {
            SelectionVM->SetSelectedSlotVM(FirstInvTab->Slots[0]);
        }
    }
}

void UInventoryVM::RefreshSlotFromInventory(UMythicInventoryComponent *Inventory, int32 AbsoluteIndex) {
    if (!Inventory) {
        return;
    }
    if (!AbsoluteIndexToSlotVM.IsValidIndex(AbsoluteIndex)) {
        return;
    }
    UItemSlotVM *SlotVM = AbsoluteIndexToSlotVM[AbsoluteIndex];
    if (!IsValid(SlotVM)) {
        return;
    }

    FMythicInventorySlotEntry Entry;
    if (!Inventory->GetSlotEntry(AbsoluteIndex, Entry)) {
        return;
    }

    // Update slot VM from current inventory data
    if (Entry.SlotDefinition) {
        SlotVM->Initialize(Entry.SlottedItemInstance, this, Entry.SlotDefinition, AbsoluteIndex);
        UE_LOG(Myth, Verbose, TEXT("Slot %d refreshed from inventory."), AbsoluteIndex);
    }
    else {
        auto Owner = Inventory->GetOwner();
        auto OwnerName = Owner ? Owner->GetName() : TEXT("NoOwner");
        UE_LOG(Myth, Error, TEXT("Slot %d has no definition for %s's inventory"), AbsoluteIndex, *OwnerName);
        SlotVM->Initialize(Entry.SlottedItemInstance, this, nullptr, AbsoluteIndex);
    }
}

void UInventoryVM::RefreshAllItemsFromInventory(UMythicInventoryComponent *Inventory) {
    if (!Inventory) {
        return;
    }
    const int32 Total = Inventory->GetNumSlots();
    for (int32 AbsIdx = 0; AbsIdx < Total; ++AbsIdx) {
        if (AbsoluteIndexToSlotVM.IsValidIndex(AbsIdx) && AbsoluteIndexToSlotVM[AbsIdx] != nullptr) {
            RefreshSlotFromInventory(Inventory, AbsIdx);
        }
    }
    UE_LOG(Myth, Log, TEXT("All inventory slots refreshed from inventory."));
}

// ---------------- UInventorySelectionVM -----------------
// tab
void UInventorySelectionVM::SetSelectedTabVM(UInventoryTabVM *InSelectedTabVM) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedTabVM, InSelectedTabVM)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedTabVM);
        UE_LOG(Myth, Verbose, TEXT("InventorySelectionVM: Selected tab VM set to: %s"),
               InSelectedTabVM ? *InSelectedTabVM->GetTabName().ToString() : TEXT("None"));
    }
}

UInventoryTabVM *UInventorySelectionVM::GetSelectedTabVM() const {
    return SelectedTabVM;
}

// slot vm
void UInventorySelectionVM::SetSelectedSlotVM(UItemSlotVM *InSelectedSlotVM) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedSlotVM, InSelectedSlotVM)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedSlotVM);
        UE_LOG(Myth, Verbose, TEXT("InventorySelectionVM: Selected slot VM set to index: %d"),
               InSelectedSlotVM ? InSelectedSlotVM->GetAbsoluteIndex() : -1);
    }
}

UItemSlotVM *UInventorySelectionVM::GetSelectedSlotVM() const {
    return SelectedSlotVM;
}
