// 


#include "InventoryVM.h"
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

void UInventoryTabVM::SetSlots(TArray<TObjectPtr<UItemSlotVM>> InSlots) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Slots, InSlots)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Slots);
        UE_LOG(Myth, Verbose, TEXT("Tab slots updated. Slot count: %d"), InSlots.Num());
    }
}

TArray<TObjectPtr<UItemSlotVM>> UInventoryTabVM::GetSlots() const {
    return Slots;
}

void UInventoryTabVM::SetSelectedSlotIndex(int32 InSelectedSlotIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedSlotIndex, InSelectedSlotIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedSlotIndex);
        UE_LOG(Myth, Verbose, TEXT("Selected slot index set to: %d"), InSelectedSlotIndex);
    }
}

int32 UInventoryTabVM::GetSelectedSlotIndex() const {
    return SelectedSlotIndex;
}

void UInventoryTabVM::Initialize(FText InTabName, UTexture2D *InTabIcon, TArray<TObjectPtr<UItemSlotVM>> InSlots, int32 InSelectedSlotIndex) {
    SetTabName(InTabName);
    SetTabIcon(InTabIcon);
    SetSlots(InSlots);
    SetSelectedSlotIndex(InSelectedSlotIndex);
}

// ---------------- UInventoryVM -----------------
void UInventoryVM::SetSelectedTabIndex(int32 InSelectedTabIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SelectedTabIndex, InSelectedTabIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SelectedTabIndex);
        UE_LOG(Myth, Verbose, TEXT("Selected tab index set to: %d"), InSelectedTabIndex);
    }
}

int32 UInventoryVM::GetSelectedTabIndex() const {
    return SelectedTabIndex;
}

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
    SetSelectedTabIndex(0);
    AbsoluteIndexToSlotVM.Reset();
    EquipmentTabs.Reset();
    InventoryTabs.Reset();
    UE_LOG(Myth, Log, TEXT("InventoryVM cleared."));
}

TArray<TObjectPtr<UInventoryTabVM>> UInventoryVM::CreateVMs(const TArray<FMythicInventorySlotEntry> &allSlots, TSet<int32> InventoryIndices) {

    auto TypeToTabMap = TMap<FGameplayTag, TObjectPtr<UInventoryTabVM>>();

    auto AsArray = InventoryIndices.Array();
    for (int32 i = 0; i < AsArray.Num(); ++i) {
        auto AbsIndex = AsArray[i];
        const FMythicInventorySlotEntry &Entry = allSlots[AbsIndex];
        auto type = Entry.SlotType;

        // Get the tab for this type, or create it if it doesn't exist
        TObjectPtr<UInventoryTabVM> *FoundTab = TypeToTabMap.Find(type);
        if (FoundTab == nullptr) {
            // Create new tab
            auto NewTabVM = NewObject<UInventoryTabVM>(this);
            // Get the name and icon from the slot type row
            const FSlotType *SlotTypeData = Entry.SlotTypeRow.GetRow<FSlotType>(FString());
            if (!SlotTypeData) {
                UE_LOG(Myth, Warning, TEXT("SlotTypeRow %s is invalid or not found in data table."), *Entry.SlotTypeRow.RowName.ToString());
                continue;
            }
            NewTabVM->Initialize(FText::FromName(Entry.SlotTypeRow.RowName), SlotTypeData->Icon, TArray<TObjectPtr<UItemSlotVM>>(), 0);
            TypeToTabMap.Add(type, NewTabVM);
            FoundTab = TypeToTabMap.Find(type);
        }

        // Add to tab
        if (FoundTab && *FoundTab) {
            auto NewSlotVM = NewObject<UItemSlotVM>(this);
            NewSlotVM->Initialize(Entry.SlottedItemInstance, this, Entry.SlotType, Entry.ItemTypeWhitelist, AbsIndex);
            (*FoundTab)->Slots.Add(NewSlotVM);

            // Ensure the map is large enough
            if (AbsoluteIndexToSlotVM.Num() <= AbsIndex) {
                AbsoluteIndexToSlotVM.SetNum(AbsIndex + 1);
            }
            AbsoluteIndexToSlotVM[AbsIndex] = NewSlotVM;
        }
    }

    auto TabsArray = TArray<TObjectPtr<UInventoryTabVM>>();
    TypeToTabMap.GenerateValueArray(TabsArray);
    return TabsArray;
}

// Internal builder from inventory
void UInventoryVM::InitializeFromInventoryComponent(UMythicInventoryComponent *InInventoryComponent) {
    Clear();
    SetOwningInventoryComponent(InInventoryComponent);

    if (InInventoryComponent == nullptr) {
        return;
    }

    const auto AllSlots = InInventoryComponent->GetAllSlots();

    auto InventoryIndices = TSet<int32>();
    auto SetOfEquipableSlots = TSet<int32>();
    for (int32 SlotIdx = 0; SlotIdx < AllSlots.Num(); ++SlotIdx) {
        const FMythicInventorySlotEntry &Entry = AllSlots[SlotIdx];
        if (Entry.SlotType.IsValid()) {
            if (Entry.bIsActive) {
                SetOfEquipableSlots.Add(SlotIdx);
            }
            else {
                InventoryIndices.Add(SlotIdx);
            }
        }
    }

    SetInventoryTabs(CreateVMs(AllSlots, InventoryIndices));
    SetEquipmentTabs(CreateVMs(AllSlots, SetOfEquipableSlots));
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
    SlotVM->Initialize(Entry.SlottedItemInstance, this, Entry.SlotType, Entry.ItemTypeWhitelist, AbsoluteIndex);
    UE_LOG(Myth, Verbose, TEXT("Slot %d refreshed from inventory."), AbsoluteIndex);
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
    UE_LOG(Myth, Log, TEXT("All inventory slots refreshed from inventory."));
}
