// 


#include "InventoryVM.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Engine/Texture2D.h"

// ---------------- UInventoryTabVM -----------------
void UInventoryTabVM::SetTabName(FText InTabName) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TabName, InTabName)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabName);
    }
}

FText UInventoryTabVM::GetTabName() const {
    return TabName;
}

void UInventoryTabVM::SetTabIcon(UTexture2D *InTabIcon) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TabIcon, InTabIcon)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TabIcon);
    }
}

UTexture2D *UInventoryTabVM::GetTabIcon() const {
    return TabIcon;
}

void UInventoryTabVM::SetSlots(TArray<TObjectPtr<UItemSlotVM>> InSlots) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Slots, InSlots)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
    }
}

TArray<TObjectPtr<UItemSlotVM>> UInventoryTabVM::GetSlots() const {
    return Slots;
}

void UInventoryTabVM::SetSelectedSlotIndex(int32 InSelectedSlotIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedSlotIndex, InSelectedSlotIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedSlotIndex);
    }
}

int32 UInventoryTabVM::GetSelectedSlotIndex() const {
    return SelectedSlotIndex;
}

// ---------------- UInventoryVM -----------------
void UInventoryVM::SetSelectedTabIndex(int32 InSelectedTabIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedTabIndex, InSelectedTabIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedTabIndex);
    }
}

int32 UInventoryVM::GetSelectedTabIndex() const {
    return SelectedTabIndex;
}

void UInventoryVM::SetTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Tabs, InTabs)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Tabs);
    }
}

TArray<TObjectPtr<UInventoryTabVM>> UInventoryVM::GetTabs() const {
    return Tabs;
}

void UInventoryVM::Clear() {
    SetSelectedTabIndex(0);
    SetTabs(TArray<TObjectPtr<UInventoryTabVM>>());
    TabStartOffsets.Reset();
    TabSlotCounts.Reset();
    TabAbsoluteIndices.Reset();
    AbsoluteToTabIndex.Reset();
    AbsoluteToSlotIndex.Reset();
    AbsoluteIndexToSlotVM.Reset();
}

void UInventoryVM::InitializeFromInventory(UMythicInventoryComponent *InInventoryComponent) {
    SetFromInventory(InInventoryComponent);
}

// Internal builder from inventory
void UInventoryVM::SetFromInventory(UMythicInventoryComponent *InInventoryComponent) {
    if (InInventoryComponent == nullptr) {
        Clear();
        return;
    }

    const int32 TotalSlots = InInventoryComponent->GetNumSlots();

    // Prepare working arrays
    struct FTabKey {
        FGameplayTag SlotType;
        int32 TabIndex;
    };

    TArray<FTabKey> TabKeys;
    TabKeys.Reserve(8);
    TArray<TArray<int32>> PerTabAbsoluteIndices;
    PerTabAbsoluteIndices.Reserve(8);
    TArray<TObjectPtr<UInventoryTabVM>> NewTabs;

    // First pass: discover tabs in first-seen order and gather absolute indices per tab
    for (int32 AbsIdx = 0; AbsIdx < TotalSlots; ++AbsIdx) {
        FMythicInventorySlotEntry Entry;
        if (!InInventoryComponent->GetSlotEntry(AbsIdx, Entry)) {
            continue;
        }

        const FGameplayTag SlotType = Entry.SlotType;

        int32 FoundTabIdx = INDEX_NONE;
        for (int32 i = 0; i < TabKeys.Num(); ++i) {
            if (TabKeys[i].SlotType == SlotType) {
                FoundTabIdx = TabKeys[i].TabIndex;
                break;
            }
        }
        if (FoundTabIdx == INDEX_NONE) {
            FoundTabIdx = NewTabs.Num();
            FTabKey NewKey;
            NewKey.SlotType = SlotType;
            NewKey.TabIndex = FoundTabIdx;
            TabKeys.Add(NewKey);

            // Create tab VM now, fill metadata
            UInventoryTabVM *TabVM = NewObject<UInventoryTabVM>(this);

            // Tab name from SlotTypeRow.RowName if valid, otherwise from tag
            FText TabDisplayName = FText();
            UTexture2D *TabIcon = nullptr;
            if (Entry.SlotTypeRow.IsNull() == false) {
                const FSlotType *SlotTypeData = Entry.SlotTypeRow.GetRow<FSlotType>(FString());
                if (SlotTypeData) {
                    TabIcon = SlotTypeData->Icon;
                }
                TabDisplayName = FText::FromName(Entry.SlotTypeRow.RowName);
            }
            if (TabDisplayName.IsEmpty()) {
                TabDisplayName = FText::FromString(SlotType.ToString());
            }
            TabVM->SetTabName(TabDisplayName);
            TabVM->SetTabIcon(TabIcon);
            TabVM->SetSelectedSlotIndex(0);

            NewTabs.Add(TabVM);
            PerTabAbsoluteIndices.AddDefaulted();
            PerTabAbsoluteIndices[FoundTabIdx].Reserve(16);
        }

        PerTabAbsoluteIndices[FoundTabIdx].Add(AbsIdx);
    }

    // Second pass: build slot VMs and mappings
    TArray<int32> NewTabStartOffsets;
    NewTabStartOffsets.Reserve(NewTabs.Num());
    TArray<int32> NewTabSlotCounts;
    NewTabSlotCounts.Reserve(NewTabs.Num());
    TArray<TArray<int32>> NewTabAbsoluteIndices = PerTabAbsoluteIndices; // copy

    // Allocate reverse lookup arrays
    TArray<int32> NewAbsToTab;
    NewAbsToTab.Init(INDEX_NONE, TotalSlots);
    TArray<int32> NewAbsToSlot;
    NewAbsToSlot.Init(INDEX_NONE, TotalSlots);
    TArray<TObjectPtr<UItemSlotVM>> NewAbsToSlotVM;
    NewAbsToSlotVM.Init(nullptr, TotalSlots);

    int32 RunningOffset = 0;
    for (int32 TabIdx = 0; TabIdx < NewTabs.Num(); ++TabIdx) {
        UInventoryTabVM *TabVM = NewTabs[TabIdx];

        const TArray<int32> &AbsList = NewTabAbsoluteIndices[TabIdx];
        NewTabStartOffsets.Add(RunningOffset);
        NewTabSlotCounts.Add(AbsList.Num());
        RunningOffset += AbsList.Num();

        // Create slot VMs for this tab
        TArray<TObjectPtr<UItemSlotVM>> SlotVMs;
        SlotVMs.Reserve(AbsList.Num());
        for (int32 LocalSlotIdx = 0; LocalSlotIdx < AbsList.Num(); ++LocalSlotIdx) {
            const int32 AbsIdx = AbsList[LocalSlotIdx];

            FMythicInventorySlotEntry Entry;
            if (!InInventoryComponent->GetSlotEntry(AbsIdx, Entry)) {
                continue;
            }

            UItemSlotVM *SlotVM = NewObject<UItemSlotVM>(this);
            SlotVM->SetAbsoluteIndex(AbsIdx);
            SlotVM->SetSlotTypeTag(Entry.SlotType);

            // Initialize from item instance (icon/qty/rarity color)
            SlotVM->SetFromItemInstance(Entry.SlottedItemInstance);

            // Clear runtime flags default
            SlotVM->SetIsLocked(false);
            SlotVM->SetIsDisabled(false);
            SlotVM->SetIsInUse(false);
            SlotVM->SetIsJunk(false);

            SlotVMs.Add(SlotVM);

            // Reverse lookups
            NewAbsToTab[AbsIdx] = TabIdx;
            NewAbsToSlot[AbsIdx] = LocalSlotIdx;
            NewAbsToSlotVM[AbsIdx] = SlotVM;
        }

        TabVM->SetSlots(SlotVMs);
    }

    // Publish
    SetTabs(NewTabs);
    SetSelectedTabIndex(0);

    // Store mappings
    TabStartOffsets = MoveTemp(NewTabStartOffsets);
    TabSlotCounts = MoveTemp(NewTabSlotCounts);
    TabAbsoluteIndices = MoveTemp(NewTabAbsoluteIndices);
    AbsoluteToTabIndex = MoveTemp(NewAbsToTab);
    AbsoluteToSlotIndex = MoveTemp(NewAbsToSlot);
    AbsoluteIndexToSlotVM = MoveTemp(NewAbsToSlotVM);
}

bool UInventoryVM::AbsoluteIndexToTabSlot(int32 AbsoluteIndex, int32 &OutTabIndex, int32 &OutSlotIndex) const {
    if (!AbsoluteToTabIndex.IsValidIndex(AbsoluteIndex)) {
        OutTabIndex = INDEX_NONE;
        OutSlotIndex = INDEX_NONE;
        return false;
    }
    const int32 TabIdx = AbsoluteToTabIndex[AbsoluteIndex];
    const int32 SlotIdx = AbsoluteToSlotIndex.IsValidIndex(AbsoluteIndex) ? AbsoluteToSlotIndex[AbsoluteIndex] : INDEX_NONE;
    if (TabIdx == INDEX_NONE || SlotIdx == INDEX_NONE) {
        OutTabIndex = INDEX_NONE;
        OutSlotIndex = INDEX_NONE;
        return false;
    }
    OutTabIndex = TabIdx;
    OutSlotIndex = SlotIdx;
    return true;
}

int32 UInventoryVM::TabSlotToAbsoluteIndex(int32 TabIndex, int32 SlotIndex) const {
    if (!TabAbsoluteIndices.IsValidIndex(TabIndex))
        return INDEX_NONE;
    const TArray<int32> &AbsList = TabAbsoluteIndices[TabIndex];
    if (!AbsList.IsValidIndex(SlotIndex))
        return INDEX_NONE;
    return AbsList[SlotIndex];
}

bool UInventoryVM::SetSlotLockedByAbsoluteIndex(int32 AbsoluteIndex, bool bLocked) {
    if (!AbsoluteIndexToSlotVM.IsValidIndex(AbsoluteIndex))
        return false;
    UItemSlotVM *SlotVM = AbsoluteIndexToSlotVM[AbsoluteIndex];
    if (!IsValid(SlotVM))
        return false;
    SlotVM->SetIsLocked(bLocked);
    return true;
}

bool UInventoryVM::SetSlotInUseByAbsoluteIndex(int32 AbsoluteIndex, bool bInUse) {
    if (!AbsoluteIndexToSlotVM.IsValidIndex(AbsoluteIndex))
        return false;
    UItemSlotVM *SlotVM = AbsoluteIndexToSlotVM[AbsoluteIndex];
    if (!IsValid(SlotVM))
        return false;
    SlotVM->SetIsInUse(bInUse);
    return true;
}

bool UInventoryVM::SetSlotDisabledByAbsoluteIndex(int32 AbsoluteIndex, bool bDisabled) {
    if (!AbsoluteIndexToSlotVM.IsValidIndex(AbsoluteIndex))
        return false;
    UItemSlotVM *SlotVM = AbsoluteIndexToSlotVM[AbsoluteIndex];
    if (!IsValid(SlotVM))
        return false;
    SlotVM->SetIsDisabled(bDisabled);
    return true;
}

void UInventoryVM::RefreshSlotFromInventory(UMythicInventoryComponent *Inventory, int32 AbsoluteIndex) {
    if (!Inventory)
        return;
    if (!AbsoluteIndexToSlotVM.IsValidIndex(AbsoluteIndex))
        return;
    UItemSlotVM *SlotVM = AbsoluteIndexToSlotVM[AbsoluteIndex];
    if (!IsValid(SlotVM))
        return;

    FMythicInventorySlotEntry Entry;
    if (!Inventory->GetSlotEntry(AbsoluteIndex, Entry))
        return;

    // Update slot VM from current inventory data
    SlotVM->SetSlotTypeTag(Entry.SlotType);
    SlotVM->SetFromItemInstance(Entry.SlottedItemInstance);
}

void UInventoryVM::RefreshAllItemsFromInventory(UMythicInventoryComponent *Inventory) {
    if (!Inventory)
        return;
    const int32 Total = Inventory->GetNumSlots();
    for (int32 AbsIdx = 0; AbsIdx < Total; ++AbsIdx) {
        if (AbsoluteIndexToSlotVM.IsValidIndex(AbsIdx) && AbsoluteIndexToSlotVM[AbsIdx] != nullptr) {
            RefreshSlotFromInventory(Inventory, AbsIdx);
        }
    }
}
