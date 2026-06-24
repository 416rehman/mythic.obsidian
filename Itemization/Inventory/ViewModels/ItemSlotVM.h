// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Styling/SlateColor.h"
#include "ItemSlotVM.generated.h"

class UInventoryVM;
class UInventoryTabVM;
struct FTimerHandle;

UCLASS()
class MYTHIC_API UItemSlotVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // the icon to display for this item slot
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    UTexture2D *Icon;

    // is marked junk
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsJunk;

    // background color for this item slot
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FSlateColor BackgroundColor;

    // quantity of items in this slot
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 Quantity = 0;

    // slot is locked (not yet unlocked by player)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsLocked;

    // slot is currently in use
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsInUse;

    // slot is disabled by game rules
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsDisabled;

    // absolute inventory index backing this slot
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 AbsoluteIndex;

    // slot definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UInventorySlotDefinition> SlotDefinition;

    // owning InventoryVM
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    UInventoryVM *ParentInventoryVM;

    // item name from definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText ItemName;

    // item description from definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FText ItemDescription;

    // rarity tier from definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TEnumAsByte<EItemRarity> Rarity = EItemRarity::Common;

    // instance level
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 ItemLevel = 1;

    // current durability from DurabilityFragment (0 if none)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float CurrentDurability = 0.0f;

    // max durability from DurabilityFragment (0 if none)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float MaxDurability = 0.0f;

    // current/max ratio, 0 when max is 0
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float DurabilityPercent = 0.0f;

    // whether this slot is an equipment slot with an active item
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsEquipped = false;

    // item is broken (durability at 0 with a max above 0)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsBroken = false;

    // item type tag from definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FGameplayTag ItemType;

    // per-unit weight from definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    float Weight = 0.0f;

    // base monetary value from definition
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 Value = 0;

    void SetIcon(UTexture2D *InIcon);
    UTexture2D *GetIcon() const;
    void SetIsJunk(bool bInIsJunk);
    bool GetIsJunk() const;
    void SetBackgroundColor(FSlateColor InBackgroundColor);
    FSlateColor GetBackgroundColor() const;
    void SetQuantity(int32 InQuantity);
    int32 GetQuantity() const;

    void SetIsLocked(bool bInLocked);
    bool GetIsLocked() const;
    void SetIsInUse(bool NewIsInUse);
    bool GetIsInUse() const;
    void SetIsDisabled(bool bInDisabled);
    bool GetIsDisabled() const;

    void SetAbsoluteIndex(int32 InAbsoluteIndex);
    int32 GetAbsoluteIndex() const;

    void SetSlotDefinition(UInventorySlotDefinition *InSlotDefinition);
    UInventorySlotDefinition *GetSlotDefinition() const;

    void SetParentInventoryVM(UInventoryVM *InParentInventoryVM);
    UInventoryVM *GetParentInventoryVM() const;

    void SetItemName(FText InItemName);
    FText GetItemName() const;
    void SetItemDescription(FText InItemDescription);
    FText GetItemDescription() const;
    void SetRarity(EItemRarity InRarity);
    EItemRarity GetRarity() const;
    void SetItemLevel(int32 InItemLevel);
    int32 GetItemLevel() const;
    void SetCurrentDurability(float InCurrentDurability);
    float GetCurrentDurability() const;
    void SetMaxDurability(float InMaxDurability);
    float GetMaxDurability() const;
    void SetDurabilityPercent(float InDurabilityPercent);
    float GetDurabilityPercent() const;
    void SetIsEquipped(bool bInIsEquipped);
    bool GetIsEquipped() const;
    void SetIsBroken(bool bInIsBroken);
    bool GetIsBroken() const;
    void SetItemType(FGameplayTag InItemType);
    FGameplayTag GetItemType() const;
    void SetWeight(float InWeight);
    float GetWeight() const;
    void SetValue(int32 InValue);
    int32 GetValue() const;

    void Initialize(UMythicItemInstance *InItemInstance, UInventoryVM *InParentVM, UInventorySlotDefinition *InSlotDefinition, int32 InAbsoluteIndex);

    // get owning inventory component from ParentInventoryVM
    UFUNCTION(BlueprintPure, Category="Mythic|Inventory|VM")
    UMythicInventoryComponent *TryGetOwningInventoryComponent() const;

    // try getting the item instance currently in this slot
    UFUNCTION(BlueprintPure, Category="Mythic|Inventory|VM")
    UMythicItemInstance *TryGetItemInstance() const;
};
