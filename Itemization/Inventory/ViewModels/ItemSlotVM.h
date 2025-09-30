// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MVVMViewModelBase.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "ItemSlotVM.generated.h"

struct FTimerHandle;

/**
 * 
 */
UCLASS()
class MYTHIC_API UItemSlotVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // The icon to display for this item slot - it can be empty
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    UTexture2D *Icon;

    // Is marked junk
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsJunk;

    // Background color for this item slot - can be used to indicate rarity or other states
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FLinearColor BackgroundColor;

    // Quantity of items in this slot
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 Quantity;

    // Slot is locked (not yet unlocked by player) - locked slots are not interactive
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsLocked;

    // Slot is currently in use (e.g., an ability is channeling from this slot)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsInUse;

    // Slot is disabled by game rules (e.g., debuff disables all abilities)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    bool IsDisabled;

    // Absolute inventory index backing this slot (in the owning InventoryComponent)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 AbsoluteIndex;

    // Gameplay slot type (e.g., Inventory.Slot.Weapon)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    FGameplayTag SlotTypeTag;

public:
    void SetIcon(UTexture2D *InIcon);
    UTexture2D *GetIcon() const;
    void SetIsJunk(bool bInIsJunk);
    bool GetIsJunk() const;
    void SetBackgroundColor(FLinearColor InBackgroundColor);
    FLinearColor GetBackgroundColor() const;
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
    void SetSlotTypeTag(FGameplayTag InSlotTypeTag);
    FGameplayTag GetSlotTypeTag() const;

    // Initialize view model fields from an item instance (icon, quantity, rarity color, etc.)
    void SetFromItemInstance(UMythicItemInstance *InItemInstance);
};
