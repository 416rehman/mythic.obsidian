// 


#include "ItemSlotVM.h"

#include "InventoryVM.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"

void UItemSlotVM::SetIcon(UTexture2D *InIcon) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon);
    }
}

UTexture2D *UItemSlotVM::GetIcon() const {
    return Icon;
}

void UItemSlotVM::SetIsJunk(bool NewIsJunk) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsJunk, NewIsJunk)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsJunk);
    }
}

bool UItemSlotVM::GetIsJunk() const {
    return IsJunk;
}

void UItemSlotVM::SetBackgroundColor(FSlateColor InBackgroundColor) {
    if (UE_MVVM_SET_PROPERTY_VALUE(BackgroundColor, InBackgroundColor)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(BackgroundColor);
    }
}

FSlateColor UItemSlotVM::GetBackgroundColor() const {
    return BackgroundColor;
}

void UItemSlotVM::SetQuantity(int32 InQuantity) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Quantity, InQuantity)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Quantity);
    }
}

int32 UItemSlotVM::GetQuantity() const {
    return Quantity;
}

void UItemSlotVM::SetIsLocked(bool bInLocked) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsLocked, bInLocked)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsLocked);
    }
}

bool UItemSlotVM::GetIsLocked() const { return IsLocked; }

void UItemSlotVM::SetIsInUse(bool NewIsInUse) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsInUse, NewIsInUse)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsInUse);
    }
}

bool UItemSlotVM::GetIsInUse() const { return IsInUse; }

void UItemSlotVM::SetIsDisabled(bool NewIsDisabled) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsDisabled, NewIsDisabled)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsDisabled);
    }
}

bool UItemSlotVM::GetIsDisabled() const { return IsDisabled; }

void UItemSlotVM::SetAbsoluteIndex(int32 InAbsoluteIndex) {
    if (UE_MVVM_SET_PROPERTY_VALUE(AbsoluteIndex, InAbsoluteIndex)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(AbsoluteIndex);
    }
}

int32 UItemSlotVM::GetAbsoluteIndex() const { return AbsoluteIndex; }

void UItemSlotVM::SetSlotDefinition(UInventorySlotDefinition *InSlotDefinition) {
    if (UE_MVVM_SET_PROPERTY_VALUE(SlotDefinition, InSlotDefinition)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(SlotDefinition);
    }
}

UInventorySlotDefinition *UItemSlotVM::GetSlotDefinition() const {
    return SlotDefinition;
}

void UItemSlotVM::SetParentInventoryVM(UInventoryVM *InParentInventoryVM) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ParentInventoryVM, InParentInventoryVM)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ParentInventoryVM);
    }
}

UInventoryVM *UItemSlotVM::GetParentInventoryVM() const {
    return ParentInventoryVM;
}

void UItemSlotVM::Initialize(UMythicItemInstance *InItemInstance, UInventoryVM *InParentVM, UInventorySlotDefinition *InSlotDefinition, int32 InAbsoluteIndex) {
    SetAbsoluteIndex(InAbsoluteIndex);
    SetSlotDefinition(InSlotDefinition);
    SetParentInventoryVM(InParentVM);
    if (InItemInstance == nullptr) {
        SetIcon(nullptr);
        SetIsJunk(false);
        SetBackgroundColor(FLinearColor::Black);
        SetQuantity(0);
    }
    else {
        auto ItemDef = InItemInstance->GetItemDefinition();
        if (ItemDef == nullptr) {
            UE_LOG(Myth, Error, TEXT("ItemInstance %s has null ItemDefinition"), *GetNameSafe(InItemInstance));
            SetIcon(nullptr);
            SetIsJunk(false);
            SetBackgroundColor(FLinearColor::Black);
            SetQuantity(0);
            return;
        }

        // LoadSynchronous (not .Get()): Icon2d is a soft ref, so .Get() returns null until the texture happens to be
        // resident — the slot rendered icon-less on an item type's first appearance. Matches ConversionViewModels::LoadIcon.
        SetIcon(ItemDef->Icon2d.IsNull() ? nullptr : ItemDef->Icon2d.LoadSynchronous());
        SetIsJunk(/*InItemInstance->bIsJunk*/ false);
        // Single-source the rarity tint (was an inline hex switch here; now centralized so the slot background and the
        // loot-pickup callout share one mapping — UItemDefinition::GetRarityColor).
        SetBackgroundColor(UItemDefinition::GetRarityColor(ItemDef->Rarity));
        SetQuantity(InItemInstance->GetStacks());
    }
}

UMythicInventoryComponent *UItemSlotVM::TryGetOwningInventoryComponent() const {
    if (ParentInventoryVM) {
        return ParentInventoryVM->GetOwningInventoryComponent();
    }
    return nullptr;
}

UMythicItemInstance *UItemSlotVM::TryGetItemInstance() const {
    if (ParentInventoryVM == nullptr) {
        return nullptr;
    }
    UMythicInventoryComponent *InvComp = ParentInventoryVM->GetOwningInventoryComponent();
    if (InvComp == nullptr) {
        return nullptr;
    }
    FMythicInventorySlotEntry Entry;
    if (!InvComp->GetSlotEntry(AbsoluteIndex, Entry)) {
        return nullptr;
    }
    return Entry.SlottedItemInstance;
}
