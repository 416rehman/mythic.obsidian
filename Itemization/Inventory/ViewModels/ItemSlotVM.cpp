

#include "ItemSlotVM.h"

#include "InventoryVM.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/InventorySlotDefinition.h"
#include "Itemization/Inventory/MythicLootFilter.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Itemization/Inventory/Fragments/Passive/DurabilityFragment.h"

void UItemSlotVM::SetIcon(UTexture2D *InIcon) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Icon, InIcon)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Icon);
    }
}

UTexture2D *UItemSlotVM::GetIcon() const {
    return Icon;
}

void UItemSlotVM::SetEmptySlotIcon(UTexture2D *InIcon) {
    if (UE_MVVM_SET_PROPERTY_VALUE(EmptySlotIcon, InIcon)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(EmptySlotIcon);
    }
}

UTexture2D *UItemSlotVM::GetEmptySlotIcon() const {
    return EmptySlotIcon;
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
        SetQuantityText(InQuantity > 1 ? FText::AsNumber(InQuantity) : FText::GetEmpty());
    }
}

int32 UItemSlotVM::GetQuantity() const {
    return Quantity;
}

void UItemSlotVM::SetQuantityText(FText InQuantityText) {
    if (UE_MVVM_SET_PROPERTY_VALUE(QuantityText, InQuantityText)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(QuantityText);
    }
}

FText UItemSlotVM::GetQuantityText() const {
    return QuantityText;
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

void UItemSlotVM::SetItemName(FText InItemName) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ItemName, InItemName)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemName);
    }
}

FText UItemSlotVM::GetItemName() const { return ItemName; }

void UItemSlotVM::SetItemDescription(FText InItemDescription) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ItemDescription, InItemDescription)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemDescription);
    }
}

FText UItemSlotVM::GetItemDescription() const { return ItemDescription; }

void UItemSlotVM::SetRarity(EItemRarity InRarity) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Rarity, InRarity)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Rarity);
    }
}

EItemRarity UItemSlotVM::GetRarity() const { return Rarity; }

void UItemSlotVM::SetItemLevel(int32 InItemLevel) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ItemLevel, InItemLevel)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemLevel);
    }
}

int32 UItemSlotVM::GetItemLevel() const { return ItemLevel; }

void UItemSlotVM::SetCurrentDurability(float InCurrentDurability) {
    if (UE_MVVM_SET_PROPERTY_VALUE(CurrentDurability, InCurrentDurability)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(CurrentDurability);
    }
}

float UItemSlotVM::GetCurrentDurability() const { return CurrentDurability; }

void UItemSlotVM::SetMaxDurability(float InMaxDurability) {
    if (UE_MVVM_SET_PROPERTY_VALUE(MaxDurability, InMaxDurability)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(MaxDurability);
    }
}

float UItemSlotVM::GetMaxDurability() const { return MaxDurability; }

void UItemSlotVM::SetDurabilityPercent(float InDurabilityPercent) {
    if (UE_MVVM_SET_PROPERTY_VALUE(DurabilityPercent, InDurabilityPercent)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(DurabilityPercent);
    }
}

float UItemSlotVM::GetDurabilityPercent() const { return DurabilityPercent; }

void UItemSlotVM::SetIsEquipped(bool bInIsEquipped) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsEquipped, bInIsEquipped)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsEquipped);
    }
}

bool UItemSlotVM::GetIsEquipped() const { return IsEquipped; }

void UItemSlotVM::SetIsBroken(bool bInIsBroken) {
    if (UE_MVVM_SET_PROPERTY_VALUE(IsBroken, bInIsBroken)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(IsBroken);
    }
}

bool UItemSlotVM::GetIsBroken() const { return IsBroken; }

void UItemSlotVM::SetItemType(FGameplayTag InItemType) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ItemType, InItemType)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ItemType);
    }
}

FGameplayTag UItemSlotVM::GetItemType() const { return ItemType; }

void UItemSlotVM::SetWeight(float InWeight) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Weight, InWeight)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Weight);
    }
}

float UItemSlotVM::GetWeight() const { return Weight; }

void UItemSlotVM::SetValue(int32 InValue) {
    if (UE_MVVM_SET_PROPERTY_VALUE(Value, InValue)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(Value);
    }
}

int32 UItemSlotVM::GetValue() const { return Value; }

void UItemSlotVM::Initialize(UMythicItemInstance *InItemInstance, UInventoryVM *InParentVM, UInventorySlotDefinition *InSlotDefinition, int32 InAbsoluteIndex) {
    SetAbsoluteIndex(InAbsoluteIndex);
    SetSlotDefinition(InSlotDefinition);
    SetParentInventoryVM(InParentVM);

    bool bSlotIsEquipment = false;
    bool bSlotCanTake = true;
    if (InParentVM) {
        UMythicInventoryComponent *InvComp = InParentVM->GetOwningInventoryComponent();
        if (InvComp) {
            FMythicInventorySlotEntry SlotEntry;
            if (InvComp->GetSlotEntry(InAbsoluteIndex, SlotEntry)) {
                bSlotIsEquipment = SlotEntry.bEquipmentSlot;
                bSlotCanTake = SlotEntry.bCanPlayerTake;
            }
        }
    }

    if (InItemInstance == nullptr) {
        SetIcon(nullptr);
        SetEmptySlotIcon(bSlotIsEquipment && SlotDefinition ? SlotDefinition->Icon.Get() : nullptr);
        SetIsJunk(false);
        SetBackgroundColor(FLinearColor::Black);
        SetQuantity(0);
        SetItemName(FText::GetEmpty());
        SetItemDescription(FText::GetEmpty());
        SetRarity(EItemRarity::Common);
        SetItemLevel(1);
        SetCurrentDurability(0.0f);
        SetMaxDurability(0.0f);
        SetDurabilityPercent(0.0f);
        SetIsEquipped(false);
        SetIsBroken(false);
        SetItemType(FGameplayTag());
        SetWeight(0.0f);
        SetValue(0);
        return;
    }

    auto ItemDef = InItemInstance->GetItemDefinition();
    if (ItemDef == nullptr) {
        UE_LOG(Myth, Error, TEXT("ItemInstance %s has null ItemDefinition"), *GetNameSafe(InItemInstance));
        SetIcon(nullptr);
        SetEmptySlotIcon(bSlotIsEquipment && SlotDefinition ? SlotDefinition->Icon.Get() : nullptr);
        SetIsJunk(false);
        SetBackgroundColor(FLinearColor::Black);
        SetQuantity(0);
        SetItemName(FText::GetEmpty());
        SetItemDescription(FText::GetEmpty());
        SetRarity(EItemRarity::Common);
        SetItemLevel(1);
        SetCurrentDurability(0.0f);
        SetMaxDurability(0.0f);
        SetDurabilityPercent(0.0f);
        SetIsEquipped(false);
        SetIsBroken(false);
        SetItemType(FGameplayTag());
        SetWeight(0.0f);
        SetValue(0);
        return;
    }

    SetIcon(ItemDef->Icon2d.IsNull() ? nullptr : ItemDef->Icon2d.LoadSynchronous());
    SetEmptySlotIcon(nullptr);

    const bool bItemIsCurrency = ItemDef->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY);
    SetIsJunk(MythicLootFilter::IsJunk(InItemInstance->IsMarkedJunk(),
                                       static_cast<int32>(ItemDef->Rarity.GetValue()),
                                       MythicLootFilter::DefaultMaxJunkRarity,
                                       ItemDef->Value, bItemIsCurrency, bSlotIsEquipment, bSlotCanTake));

    SetBackgroundColor(UItemDefinition::GetRarityColor(ItemDef->Rarity));
    SetQuantity(InItemInstance->GetStacks());

    SetItemName(ItemDef->Name);
    SetItemDescription(ItemDef->Description);
    SetRarity(ItemDef->Rarity);
    SetItemLevel(InItemInstance->GetItemLevel());
    SetItemType(ItemDef->ItemType);
    SetWeight(ItemDef->Weight);
    SetValue(ItemDef->Value);

    SetIsEquipped(bSlotIsEquipment && InItemInstance != nullptr);

    const UDurabilityFragment *DurFrag = InItemInstance->GetFragment<UDurabilityFragment>();
    if (DurFrag) {
        float CurDur = static_cast<float>(DurFrag->GetCurrentDurability());
        float MaxDur = static_cast<float>(DurFrag->GetMaxDurability());
        SetCurrentDurability(CurDur);
        SetMaxDurability(MaxDur);
        SetDurabilityPercent(MaxDur > 0.0f ? CurDur / MaxDur : 0.0f);
        SetIsBroken(MaxDur > 0.0f && CurDur <= 0.0f);
    }
    else {
        SetCurrentDurability(0.0f);
        SetMaxDurability(0.0f);
        SetDurabilityPercent(0.0f);
        SetIsBroken(false);
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
