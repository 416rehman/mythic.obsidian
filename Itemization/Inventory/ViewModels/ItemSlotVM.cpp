// 


#include "ItemSlotVM.h"

#include "InventoryVM.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"

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

        SetIcon(ItemDef->Icon2d.Get());
        SetIsJunk(/*InItemInstance->bIsJunk*/ false);
        switch (ItemDef->Rarity) {
        case Common: {
            auto Gray = FColor::FromHex("#808080");
            SetBackgroundColor(FLinearColor::FromSRGBColor(Gray));
        }
        break;
        case Rare: {
            auto Blue = FColor::FromHex("#15c965");
            SetBackgroundColor(FLinearColor::FromSRGBColor(Blue));
        }
        break;
        case Epic: {
            auto Purple = FColor::FromHex("#732BD2FF");
            SetBackgroundColor(FLinearColor::FromSRGBColor(Purple));
        }
        break;
        case Legendary: {
            auto Orange = FColor::FromHex("#BE6009FF");
            SetBackgroundColor(FLinearColor::FromSRGBColor(Orange));
        }
        break;
        case Mythic: {
            auto Gold = FColor::FromHex("#FF3F36FF");
            SetBackgroundColor(FLinearColor::FromSRGBColor(Gold));
        }
        break;
        default:
            SetBackgroundColor(FLinearColor::Black);
            break;
        }
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
