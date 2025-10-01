// 

#pragma once

#include "CoreMinimal.h"
#include "ItemSlotVM.h"
#include "MVVMViewModelBase.h"
#include "InventoryVM.generated.h"

// A VM representing a single inventory tab with all its slots
UCLASS()
class MYTHIC_API UInventoryTabVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // Name of the tab
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    FText TabName;

    // Icon of the tab
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    UTexture2D *TabIcon;

    // Slots in this tab
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    TArray<TObjectPtr<UItemSlotVM>> Slots; // Should be UItemSlotVM

    // Selected slot index
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    int32 SelectedSlotIndex;

    void SetTabName(FText InTabName);
    FText GetTabName() const;
    void SetTabIcon(UTexture2D *InTabIcon);
    UTexture2D *GetTabIcon() const;
    void SetSlots(TArray<TObjectPtr<UItemSlotVM>> InSlots);
    TArray<TObjectPtr<UItemSlotVM>> GetSlots() const;
    void SetSelectedSlotIndex(int32 InSelectedSlotIndex);
    int32 GetSelectedSlotIndex() const;
};

/**
 * 
 */
UCLASS()
class MYTHIC_API UInventoryVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // Selected tab index
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    int32 SelectedTabIndex;

    // Tabs VMs
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TArray<TObjectPtr<UInventoryTabVM>> Tabs;

    // The owning inventory component (not serialized, just for convenience)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UMythicInventoryComponent> OwningInventoryComponent;

    // Helpers to translate indices
    UFUNCTION(BlueprintPure, Category="Mythic|Inventory|VM")
    bool AbsoluteIndexToTabSlot(int32 AbsoluteIndex, int32 &OutTabIndex, int32 &OutSlotIndex) const;

    UFUNCTION(BlueprintPure, Category="Mythic|Inventory|VM")
    int32 TabSlotToAbsoluteIndex(int32 TabIndex, int32 SlotIndex) const;
    
    void InitializeFromInventoryComponent(UMythicInventoryComponent *InInventoryComponent);
protected:
    void SetSelectedTabIndex(int32 InSelectedTabIndex);
    int32 GetSelectedTabIndex() const;
    void SetTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs);
    TArray<TObjectPtr<UInventoryTabVM>> GetTabs() const;
    // Prefer using InitializeFromInventory() to set this
    void SetOwningInventoryComponent(UMythicInventoryComponent *InOwningInventoryComponent);


    void Clear();

public:
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    UMythicInventoryComponent *GetOwningInventoryComponent() const;
    
    // Slot state helpers by absolute index
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    bool SetSlotLockedByAbsoluteIndex(int32 AbsoluteIndex, bool bLocked);

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    bool SetSlotInUseByAbsoluteIndex(int32 AbsoluteIndex, bool bInUse);

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    bool SetSlotDisabledByAbsoluteIndex(int32 AbsoluteIndex, bool bDisabled);

    // Refresh methods for manual updates
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshSlotFromInventory(class UMythicInventoryComponent *Inventory, int32 AbsoluteIndex);

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshAllItemsFromInventory(class UMythicInventoryComponent *Inventory);

private:
    // Cumulative offsets per tab to compute absolute <-> (tab,slot)
    TArray<int32> TabStartOffsets;
    TArray<int32> TabSlotCounts;
    // Explicit mapping: per-tab list of absolute indices backing each displayed slot
    TArray<TArray<int32>> TabAbsoluteIndices;
    // Fast reverse lookup: for a given absolute index, find owning tab and slot
    TArray<int32> AbsoluteToTabIndex;
    TArray<int32> AbsoluteToSlotIndex;
    // Direct pointer map to slot view models by absolute index
    TArray<TObjectPtr<UItemSlotVM>> AbsoluteIndexToSlotVM;
};
