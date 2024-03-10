// 

#pragma once

#include "CoreMinimal.h"
#include "InventorySlot.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "WorldItem.h"
#include "InventoryComponent.generated.h"

// delegate for active slot changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveSlotChanged, int32, NewIndex, int32, OldIndex);

// delegate for slot updated - called when an item is added or removed from a slot
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotUpdated, UInventorySlot*, Slot);

// delegate for inventory size changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySizeChanged, int32, NewSize, int32, OldSize);

// delegate for item dropped - called when an item is dropped from the inventory
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDropped, UInventorySlot*, Slot, AWorldItem*, WorldItem);

USTRUCT(BlueprintType)

struct FSlotType : public FTableRowBase {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta=(Categories="Inventory.Slot"))
    FGameplayTag SlotType = TAG_Inventory_Slot_Default;

    // Permit types of items that can be placed in this slot. To permit all items, leave this tag container empty. For example, a weapon slot may only permit items with the "Weapon" tag, while a chest slot may permit items with the "Armor" tag.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta=(Categories="Itemization.Type"))
    FGameplayTagContainer WhitelistedItemTypes = FGameplayTagContainer();
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))

class MYTHIC_API UInventoryComponent : public UActorComponent {
protected:
    GENERATED_BODY()
    int32 ActiveSlotIndex = 0;

    // Default size of the inventory. In BeginPlay, this many slots will be created
    UPROPERTY(EditAnywhere, Category = "Slots")
    int32 DefaultSize = 10;

    /*
    * Slots maintained by this component
    */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Slots")
    TArray<UInventorySlot *> Slots;

public:
    UInventoryComponent(const FObjectInitializer &OI);
    virtual void BeginPlay() override;

    // Set active slot index
    UFUNCTION(BlueprintCallable, Category = "Slots")
    void SetActiveSlotIndex(int32 NewIndex);

    // Get active slot index
    UFUNCTION(BlueprintCallable, Category = "Slots")
    int32 GetActiveSlotIndex() const;

    // Row name of the slot type to use for the given slot index from the SlotTypes table
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots")
    FDataTableRowHandle SlotTypeRow;

    UFUNCTION(BlueprintCallable, Category = "Slots")
    FGameplayTag GetSlotType() const;

    UFUNCTION(BlueprintCallable, Category = "Slots")
    FGameplayTagContainer GetSlotWhitelistedTypes() const;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UInventoryComponent, Slots);
    }

    //Destroys a slot
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool DestroySlot(UInventorySlot *InSlot);

    //Clears and destroys all Slots
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void DestroyAllSlots();

    //Returns the number of slots. Can be used by clients as well
    UFUNCTION(BlueprintCallable, Category = "Slots")
    int32 GetSlotCount() const;

    // Populates the given list view with the slots in this inventory
    UFUNCTION(BlueprintCallable, Category = "Slots")
    void PopulateListView(UListView *SlotListView);

    // Resizes the inventory to the given size. Items will be dropped to the ground if the new size is smaller than the old size
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void Resize(int32 NewSize);

    // Returns the item in the given slot
    UFUNCTION(BlueprintCallable, Category = "Slots")
    UInventorySlot *GetSlot(int32 SlotIndex);

    // Make sure the TargetSlot is valid and has no items in it. Will set the target slot's item instance to the given item instance and the item instance's slot to the target slot.
    // Returns the amount of items that were transferred. 0 if the transfer failed.
    bool TryTransferToSlot(UItemInstance *ItemInstance, UInventorySlot *TargetSlot);

    // Add to inventory. Will stack if possible, otherwise will add to any available slot, and if no room is available, will drop the item to the ground. Returns pointer to the dropped item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    AWorldItem *AddItem(UItemInstance *ItemInstance, bool is_private);
    
    // Add item to any available slot. Will stack if possible (if a full transfer occurs through stacking, the item instance will be destroyed).
    // Removes from owner's subobject list on success. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToAnySlot(UItemInstance *ItemInstance);

    // Add item to the given slot. If the slot is already occupied, the item will be stacked if possible. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToSlot(UItemInstance *ItemInstance, int32 SlotIndex);

    // Receives an item instance. Should be invoked from the SendItem function by the other inventory. Returns the amount of items that were received. Will remove the item from
    int32 ReceiveItem(TObjectPtr<UItemInstance> ItemInstance, UInventorySlot *TargetSlot);

    // Sends an item instance to another inventory. Returns the amount of items that were sent, must set the item instance to null if all items were sent.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 SendItem(UInventorySlot *ItemSlot, UInventoryComponent *TargetInventory, UInventorySlot *TargetSlot = nullptr);

    // Drops an item instance to the ground through a WorldItem. Returns true if the item was dropped.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool DropItem(UInventorySlot *item_slot, const FVector &location, const float radius = 100.0f);

    // Picks up a WorldItem. Returns true if the item was picked up and the WorldItem was destroyed.
    // UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    // int32 PickupItem(AWorldItem *world_item);

    /** Delegates */
    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnActiveSlotChanged OnActiveSlotChanged;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnSlotUpdated OnSlotUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnInventorySizeChanged OnInventorySizeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnItemDropped OnItemDropped;

protected:
    // Ensure we destroy all objects when components is destroyed/unregistered
    virtual void OnUnregister() override;
};
