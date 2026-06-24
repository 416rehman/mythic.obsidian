// 


#include "InventoryVM.h"

#include "Mythic.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/MythicEncumbrance.h"
#include "Settings/MythicDeveloperSettings.h"
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

void UInventoryTabVM::SetCanPlayerTake(bool bInCanPlayerTake) {
    if (UE_MVVM_SET_PROPERTY_VALUE(CanPlayerTake, bInCanPlayerTake)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanPlayerTake);
    }
}

bool UInventoryTabVM::GetCanPlayerTake() const {
    return CanPlayerTake;
}

void UInventoryTabVM::SetCanPlayerPut(bool bInCanPlayerPut) {
    if (UE_MVVM_SET_PROPERTY_VALUE(CanPlayerPut, bInCanPlayerPut)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CanPlayerPut);
    }
}

bool UInventoryTabVM::GetCanPlayerPut() const {
    return CanPlayerPut;
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

void UInventoryTabVM::Initialize(FText InTabName, UTexture2D *InTabIcon, FGameplayTag InGroupTag, bool bInCanPlayerTake, bool bInCanPlayerPut,
                                 TArray<TObjectPtr<UItemSlotVM>> InSlots) {
    SetTabName(InTabName);
    SetTabIcon(InTabIcon);
    SetGroupTag(InGroupTag);
    SetCanPlayerTake(bInCanPlayerTake);
    SetCanPlayerPut(bInCanPlayerPut);
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

void UInventoryVM::SetTotalWeight(float InTotalWeight) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TotalWeight, InTotalWeight)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TotalWeight);
    }
}

float UInventoryVM::GetTotalWeight() const { return TotalWeight; }

void UInventoryVM::SetMaxCarryWeight(float InMaxCarryWeight) {
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxCarryWeight, InMaxCarryWeight)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MaxCarryWeight);
    }
}

float UInventoryVM::GetMaxCarryWeight() const { return MaxCarryWeight; }

void UInventoryVM::SetEncumbranceTier(EMythicEncumbranceTier InEncumbranceTier) {
    if (UE_MVVM_SET_PROPERTY_VALUE(EncumbranceTier, InEncumbranceTier)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EncumbranceTier);
    }
}

EMythicEncumbranceTier UInventoryVM::GetEncumbranceTier() const { return EncumbranceTier; }

void UInventoryVM::SetTotalCurrency(int32 InTotalCurrency) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TotalCurrency, InTotalCurrency)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TotalCurrency);
    }
}

int32 UInventoryVM::GetTotalCurrency() const { return TotalCurrency; }

void UInventoryVM::SetUsedSlots(int32 InUsedSlots) {
    if (UE_MVVM_SET_PROPERTY_VALUE(UsedSlots, InUsedSlots)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(UsedSlots);
    }
}

int32 UInventoryVM::GetUsedSlots() const { return UsedSlots; }

void UInventoryVM::SetTotalSlots(int32 InTotalSlots) {
    if (UE_MVVM_SET_PROPERTY_VALUE(TotalSlots, InTotalSlots)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(TotalSlots);
    }
}

int32 UInventoryVM::GetTotalSlots() const { return TotalSlots; }

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
    // map GroupTag -> TabVM
    TMap<FGameplayTag, TObjectPtr<UInventoryTabVM>> GroupToTabMap;

    auto AsArray = SlotIndices.Array();
    for (int32 i = 0; i < AsArray.Num(); ++i) {
        int32 AbsIndex = AsArray[i];
        const FMythicInventorySlotEntry &Entry = allSlots[AbsIndex];

        if (!Entry.SlotDefinition) {
            continue;
        }

        // get or create tab for this group
        TObjectPtr<UInventoryTabVM> *FoundTab = GroupToTabMap.Find(Entry.GroupTag);
        if (FoundTab == nullptr) {
            auto NewTabVM = NewObject<UInventoryTabVM>(this);

            // get group info from Profile if available
            FText GroupName = FText::FromString(Entry.GroupTag.ToString());
            UTexture2D *GroupIcon = nullptr;
            bool bCanTake = true;
            bool bCanPut = true;

            if (OwningInventoryComponent && OwningInventoryComponent->InventoryProfile) {
                const FInventorySlotGroup *Group = OwningInventoryComponent->InventoryProfile->SlotGroups.Find(Entry.GroupTag);
                if (Group) {
                    GroupName = Group->GroupName.IsEmpty() ? GroupName : Group->GroupName;
                    GroupIcon = Group->Icon;
                    bCanTake = Group->bCanPlayerTake;
                    bCanPut = Group->bCanPlayerPut;
                }
            }

            NewTabVM->Initialize(GroupName, GroupIcon, Entry.GroupTag, bCanTake, bCanPut, TArray<TObjectPtr<UItemSlotVM>>());
            GroupToTabMap.Add(Entry.GroupTag, NewTabVM);
            FoundTab = GroupToTabMap.Find(Entry.GroupTag);
        }

        // create slot VM and add to tab
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

    // convert to array and sort by DisplayOrder
    TArray<TObjectPtr<UInventoryTabVM>> TabsArray;
    GroupToTabMap.GenerateValueArray(TabsArray);

    // sort by DisplayOrder from Profile
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

    // compute aggregates after all slot VMs are built
    RefreshAggregates(InInventoryComponent);
}

void UInventoryVM::SetTransactionLocked(bool bInLocked) {
    if (bTransactionLocked != bInLocked) {
        bTransactionLocked = bInLocked;
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bTransactionLocked);

        // handle client-side timeout timer for transaction lock state
        if (bTransactionLocked) {
            if (UMythicInventoryComponent* Comp = GetOwningInventoryComponent()) {
                if (UWorld* World = Comp->GetWorld()) {
                    World->GetTimerManager().SetTimer(
                        TransactionLockTimerHandle,
                        this,
                        &UInventoryVM::ClearTransactionLock,
                        0.5f,
                        false
                    );
                }
            }
        } else {
            if (UMythicInventoryComponent* Comp = GetOwningInventoryComponent()) {
                if (UWorld* World = Comp->GetWorld()) {
                    World->GetTimerManager().ClearTimer(TransactionLockTimerHandle);
                }
            }
        }
    }
}

bool UInventoryVM::GetTransactionLocked() const {
    return bTransactionLocked;
}

void UInventoryVM::ClearTransactionLock() {
    SetTransactionLocked(false);
}

void UInventoryVM::RefreshSlotFromInventory(UMythicInventoryComponent *Inventory, int32 AbsoluteIndex) {
    // clear the transaction lock since a slot refresh indicates replication completed
    if (GetTransactionLocked()) {
        SetTransactionLocked(false);
    }

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

    // update slot VM from current inventory data
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

    RefreshAggregates(Inventory);
    UE_LOG(Myth, Log, TEXT("All inventory slots refreshed from inventory."));
}

void UInventoryVM::RefreshAggregates(UMythicInventoryComponent *Inventory) {
    if (!Inventory) {
        return;
    }

    const auto &AllSlots = Inventory->GetAllSlots();

    // weight from the inventory component helper
    float ComputedWeight = Inventory->GetTotalCarriedWeight();
    SetTotalWeight(ComputedWeight);

    // max carry weight and encumbrance from developer settings
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    float SoftCap = Settings->EncumbranceSoftCapacity;
    float HardCap = Settings->EncumbranceHardCapacity;
    SetMaxCarryWeight(SoftCap);
    SetEncumbranceTier(MythicEncumbrance::ComputeTier(ComputedWeight, SoftCap, HardCap));

    // currency from the inventory component helper
    SetTotalCurrency(Inventory->GetTotalCurrency());

    // slot counts: only non-equipment slots
    int32 NumUsed = 0;
    int32 NumTotal = 0;
    for (const FMythicInventorySlotEntry &Entry : AllSlots) {
        if (Entry.bEquipmentSlot) {
            continue;
        }
        if (!Entry.SlotDefinition) {
            continue;
        }
        ++NumTotal;
        if (Entry.SlottedItemInstance != nullptr) {
            ++NumUsed;
        }
    }
    SetUsedSlots(NumUsed);
    SetTotalSlots(NumTotal);
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
