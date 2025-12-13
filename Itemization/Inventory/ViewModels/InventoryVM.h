// 

#pragma once

#include "CoreMinimal.h"
#include "ItemSlotVM.h"
#include "MVVMViewModelBase.h"
#include "InventoryVM.generated.h"


// Inventory SelectionVM
UCLASS()
class MYTHIC_API UInventorySelectionVM : public UMVVMViewModelBase {
    GENERATED_BODY()

    // selected tab vm
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UInventoryTabVM> SelectedTabVM;

    // selected slot vm
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UItemSlotVM> SelectedSlotVM;

public:
    void SetSelectedTabVM(UInventoryTabVM *InSelectedTabVM);
    UInventoryTabVM *GetSelectedTabVM() const;
    void SetSelectedSlotVM(UItemSlotVM *InSelectedSlotVM);
    UItemSlotVM *GetSelectedSlotVM() const;

    void ClearSelection() {
        SetSelectedTabVM(nullptr);
        SetSelectedSlotVM(nullptr);
    }
};


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

    void SetTabName(FText InTabName);
    FText GetTabName() const;
    void SetTabIcon(UTexture2D *InTabIcon);
    UTexture2D *GetTabIcon() const;
    void SetSlots(TArray<TObjectPtr<UItemSlotVM>> InSlots);
    TArray<TObjectPtr<UItemSlotVM>> GetSlots() const;

    void Initialize(FText InTabName, UTexture2D *InTabIcon, TArray<TObjectPtr<UItemSlotVM>> InSlots, int32 InSelectedSlotIndex = 0);
};

/**
 * 
 */
UCLASS()
class MYTHIC_API UInventoryVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // The owning inventory component (not serialized, just for convenience)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UMythicInventoryComponent> OwningInventoryComponent;

    // Equipment tabs: tabs with at least one equipable or auto-activate slot
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    TArray<TObjectPtr<UInventoryTabVM>> EquipmentTabs;

    // Inventory tabs: tabs with only non-equipable slots
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    TArray<TObjectPtr<UInventoryTabVM>> InventoryTabs;

    // Selection VM for tracking selected tab and slot
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    TObjectPtr<UInventorySelectionVM> SelectionVM;

    void InitializeFromInventoryComponent(UMythicInventoryComponent *InInventoryComponent);

protected:
    void SetInventoryTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs);
    TArray<TObjectPtr<UInventoryTabVM>> GetInventoryTabs() const;
    void SetEquipmentTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs);
    TArray<TObjectPtr<UInventoryTabVM>> GetEquipmentTabs() const;
    void SetSelectionVM(UInventorySelectionVM *InSelectionVM);
    UInventorySelectionVM *GetSelectionVM() const;

    // Prefer using InitializeFromInventory() to set this
    void SetOwningInventoryComponent(UMythicInventoryComponent *InOwningInventoryComponent);


    void Clear();

public:
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    UMythicInventoryComponent *GetOwningInventoryComponent() const;

    // Refresh methods for manual updates
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshSlotFromInventory(class UMythicInventoryComponent *Inventory, int32 AbsoluteIndex);

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshAllItemsFromInventory(class UMythicInventoryComponent *Inventory);

    // Maps absolute slot indices to their SlotVM instances for fast lookup
    UPROPERTY(Transient)
    TArray<TObjectPtr<UItemSlotVM>> AbsoluteIndexToSlotVM;

private:
    TArray<TObjectPtr<UInventoryTabVM>> CreateVMs(const TArray<FMythicInventorySlotEntry> &allSlots, TSet<int32> InventoryIndices);
};
