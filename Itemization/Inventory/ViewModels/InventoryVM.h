// 

#pragma once

#include "CoreMinimal.h"
#include "ItemSlotVM.h"
#include "MVVMViewModelBase.h"
#include "Itemization/Inventory/MythicEncumbrance.h"
#include "InventoryVM.generated.h"


// inventory selection vm
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


// a vm representing a single inventory tab with all its slots
UCLASS()
class MYTHIC_API UInventoryTabVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // name of the tab
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    FText TabName;

    // icon of the tab
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    UTexture2D *TabIcon;

    // tag identifying this group
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    FGameplayTag GroupTag;

    // if false, items in this tab cannot be dropped/sold/transferred by player
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    bool CanPlayerTake = true;

    // if false, player cannot insert items manually
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    bool CanPlayerPut = true;

    // slots in this tab
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess=true))
    TArray<TObjectPtr<UItemSlotVM>> Slots;

    void SetTabName(FText InTabName);
    FText GetTabName() const;
    void SetTabIcon(UTexture2D *InTabIcon);
    UTexture2D *GetTabIcon() const;
    void SetGroupTag(FGameplayTag InGroupTag);
    FGameplayTag GetGroupTag() const;
    void SetCanPlayerTake(bool bInCanPlayerTake);
    bool GetCanPlayerTake() const;
    void SetCanPlayerPut(bool bInCanPlayerPut);
    bool GetCanPlayerPut() const;
    void SetSlots(TArray<TObjectPtr<UItemSlotVM>> InSlots);
    TArray<TObjectPtr<UItemSlotVM>> GetSlots() const;

    void Initialize(FText InTabName, UTexture2D *InTabIcon, FGameplayTag InGroupTag, bool bInCanPlayerTake, bool bInCanPlayerPut,
                    TArray<TObjectPtr<UItemSlotVM>> InSlots);
};

UCLASS()
class MYTHIC_API UInventoryVM : public UMVVMViewModelBase {
    GENERATED_BODY()

public:
    // the owning inventory component (not serialized, just for convenience)
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter, Getter, meta=(AllowPrivateAccess))
    TObjectPtr<UMythicInventoryComponent> OwningInventoryComponent;

    // equipment tabs: tabs with at least one equipable or auto-activate slot
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    TArray<TObjectPtr<UInventoryTabVM>> EquipmentTabs;

    // inventory tabs: tabs with only non-equipable slots
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    TArray<TObjectPtr<UInventoryTabVM>> InventoryTabs;

    // selection vm for tracking selected tab and slot
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    TObjectPtr<UInventorySelectionVM> SelectionVM;

    // total weight of all items in the inventory
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    float TotalWeight = 0.0f;

    // max carry weight from developer settings (EncumbranceSoftCapacity)
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    float MaxCarryWeight = 0.0f;

    // current encumbrance tier
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    EMythicEncumbranceTier EncumbranceTier = EMythicEncumbranceTier::Unencumbered;

    // total currency value across all currency-type items
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    int32 TotalCurrency = 0;

    // number of non-empty, non-equipment slots
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    int32 UsedSlots = 0;

    // total number of non-equipment slots
    UPROPERTY(BlueprintReadOnly, FieldNotify, Setter, Getter, Category="Mythic|Inventory|VM")
    int32 TotalSlots = 0;

    // locks the inventory UI during network transactions to prevent input exploits
    UPROPERTY(BlueprintReadWrite, FieldNotify, Setter=SetTransactionLocked, Getter=GetTransactionLocked, Category="Mythic|Inventory|VM")
    bool bTransactionLocked = false;

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void SetTransactionLocked(bool bInLocked);

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    bool GetTransactionLocked() const;

    // helper to clear the transaction lock from timers
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void ClearTransactionLock();

    void InitializeFromInventoryComponent(UMythicInventoryComponent *InInventoryComponent);

    // recompute aggregate fields from the inventory
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshAggregates(UMythicInventoryComponent *Inventory);

protected:
    void SetInventoryTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs);
    TArray<TObjectPtr<UInventoryTabVM>> GetInventoryTabs() const;
    void SetEquipmentTabs(TArray<TObjectPtr<UInventoryTabVM>> InTabs);
    TArray<TObjectPtr<UInventoryTabVM>> GetEquipmentTabs() const;
    void SetSelectionVM(UInventorySelectionVM *InSelectionVM);
    UInventorySelectionVM *GetSelectionVM() const;

    void SetTotalWeight(float InTotalWeight);
    float GetTotalWeight() const;
    void SetMaxCarryWeight(float InMaxCarryWeight);
    float GetMaxCarryWeight() const;
    void SetEncumbranceTier(EMythicEncumbranceTier InEncumbranceTier);
    EMythicEncumbranceTier GetEncumbranceTier() const;
    void SetTotalCurrency(int32 InTotalCurrency);
    int32 GetTotalCurrency() const;
    void SetUsedSlots(int32 InUsedSlots);
    int32 GetUsedSlots() const;
    void SetTotalSlots(int32 InTotalSlots);
    int32 GetTotalSlots() const;

    // prefer using InitializeFromInventory() to set this
    void SetOwningInventoryComponent(UMythicInventoryComponent *InOwningInventoryComponent);

    void Clear();

public:
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    UMythicInventoryComponent *GetOwningInventoryComponent() const;

    // refresh methods for manual updates
    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshSlotFromInventory(class UMythicInventoryComponent *Inventory, int32 AbsoluteIndex);

    UFUNCTION(BlueprintCallable, Category="Mythic|Inventory|VM")
    void RefreshAllItemsFromInventory(class UMythicInventoryComponent *Inventory);

    // maps absolute slot indices to their SlotVM instances for fast lookup
    UPROPERTY(Transient)
    TArray<TObjectPtr<UItemSlotVM>> AbsoluteIndexToSlotVM;

private:
    TArray<TObjectPtr<UInventoryTabVM>> CreateVMs(const TArray<FMythicInventorySlotEntry> &allSlots, TSet<int32> InventoryIndices);

    // handles client-side timeout of transaction lock state
    FTimerHandle TransactionLockTimerHandle;
};
