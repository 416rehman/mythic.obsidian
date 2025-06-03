// 

#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "MythicInventoryComponent_ViewModel.h"
#include "MythicInventorySlot.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "MythicInventoryComponent.generated.h"

class AMythicWorldItem;
// delegate for active slot changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveSlotChanged, int32, NewIndex, int32, OldIndex);

// delegate for slot updated - called when an item is added or removed from a slot
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotUpdated, UMythicInventorySlot*, Slot);

// delegate for inventory size changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySizeChanged, int32, NewSize, int32, OldSize);

// delegate for item dropped - called when an item is dropped from the inventory
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDropped, UMythicInventorySlot*, Slot, AMythicWorldItem*, WorldItem);

// delegate for ViewModel created without any parameters
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewModelCreated);

USTRUCT(BlueprintType)

struct FSlotType : public FTableRowBase {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta=(Categories="Inventory.Slot"))
    FGameplayTag SlotType = INVENTORY_SLOT_DEFAULT;

    // Permit types of items that can be placed in this slot. To permit all items, leave this tag container empty. For example, a weapon slot may only permit items with the "Weapon" tag, while a chest slot may permit items with the "Armor" tag.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slot", meta=(Categories="Itemization.Type"))
    FGameplayTagContainer WhitelistedItemTypes = FGameplayTagContainer();
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))

class MYTHIC_API UMythicInventoryComponent : public UActorComponent {
    // View Model ////////////////////
protected:
    // ViewModel for this component - If ViewModelIdentifier is not set, this will be null
    UPROPERTY(BlueprintReadOnly, Category = "ViewModel")
    UMythicInventoryComponent_ViewModel *ViewModel;

    // ViewModel Identifier for this component
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ViewModel")
    FName ViewModelIdentifier = FName();

public:
    /// ViewModel - Run on owning clients
    UFUNCTION(Reliable, Client, Category="ViewModel")
    void ClientSetupViewModel();
    void ClientSetupViewModel_Implementation();

    UFUNCTION(BlueprintCallable, Category = "ViewModel", Client, Reliable)
    void ClientUpdateViewModelSlots();
    void ClientUpdateViewModelSlots_Implementation();

    UFUNCTION(BlueprintCallable, Category = "ViewModel", Client, Reliable)
    void ClientUpdateViewModelActiveSlot(int32 NewIndex);
    void ClientUpdateViewModelActiveSlot_Implementation(int32 NewIndex);

    UFUNCTION(BlueprintCallable, Category = "ViewModel", Client, Reliable)
    void ClientUpdateViewModeInventorySize(int32 NewSize);
    void ClientUpdateViewModeInventorySize_Implementation(int32 NewSize);

    UPROPERTY(BlueprintAssignable, Category = "ViewModel")
    FOnViewModelCreated OnViewModelCreated;
    // View Model End ////////////////////
protected:
    GENERATED_BODY()
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Slots")
    int32 ActiveSlotIndex = 0;

    // Default size of the inventory. In BeginPlay, this many slots will be created
    UPROPERTY(EditDefaultsOnly, Category = "Slots")
    int32 InitialSize = 10;

    /*
    * Slots maintained by this component
    */
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Slots")
    TArray<UMythicInventorySlot *> Slots;

public:
    UMythicInventoryComponent(const FObjectInitializer &OI);

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
        DOREPLIFETIME(UMythicInventoryComponent, Slots);
        DOREPLIFETIME(UMythicInventoryComponent, ActiveSlotIndex);
    }

    //Destroys a slot
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool DestroySlot(UMythicInventorySlot *InSlot);

    //Clears and destroys all Slots
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void DestroyAllSlots();

    //Returns the number of slots. Can be used by clients as well
    UFUNCTION(BlueprintCallable, Category = "Slots")
    int32 GetSlotCount() const;

    // Resizes the inventory to the given size. Items will be dropped to the ground if the new size is smaller than the old size
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void Resize(int32 NewSize);

    // Returns the item in the given slot
    UFUNCTION(BlueprintCallable, Category = "Slots")
    UMythicInventorySlot *GetSlot(int32 SlotIndex);

    // Make sure the TargetSlot is valid and has no items in it. Will set the target slot's item instance to the given item instance and the item instance's slot to the target slot.
    // Returns the amount of items that were transferred. 0 if the transfer failed.
    bool TryTransferToSlot(UMythicItemInstance *ItemInstance, UMythicInventorySlot *TargetSlot);

    // Add to inventory. Will stack if possible, otherwise will add to any available slot, and if no room is available, will drop the item to the ground. Returns pointer to the dropped item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    AMythicWorldItem *AddItem(UMythicItemInstance *ItemInstance, AController *TargetRecipient);

    // Add item to any available slot. Will stack if possible (if a full transfer occurs through stacking, the item instance will be destroyed).
    // Removes from owner's subobject list on success. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToAnySlot(UMythicItemInstance *ItemInstance);

    // Add item to the given slot. If the slot is already occupied, the item will be stacked if possible. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex);

    // Receives an item instance. Should be invoked from the SendItem function by the other inventory. Returns the amount of items that were received. Will remove the item from
    int32 ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, UMythicInventorySlot *TargetSlot);

    // Sends an item instance to another inventory. Returns the amount of items that were sent, must set the item instance to null if all items were sent.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 SendItem(UMythicInventorySlot *ItemSlot, UMythicInventoryComponent *TargetInventory, UMythicInventorySlot *TargetSlot = nullptr);

    // Drops an item instance to the ground through a WorldItem. Returns true if the item was dropped.
    // If TargetRecipient is provided, only they can interact with the item. Otherwise, all players can interact with the item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool DropItem(UMythicInventorySlot *item_slot, const FVector &location, const float radius = 100.0f, AController *TargetRecipient = nullptr);

    // Picks up a WorldItem. Returns the amount of items (stacks) that were picked up.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void PickupItem(AMythicWorldItem *world_item);

    /** Delegates */
    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnActiveSlotChanged OnActiveSlotChanged;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnSlotUpdated OnSlotUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnInventorySizeChanged OnInventorySizeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnItemDropped OnItemDropped;

    // Client only functions, called by the server
    UFUNCTION(Client, Reliable)
    void ClientOnSlotUpdatedDelegate(UMythicInventorySlot *Slot);

    UFUNCTION(Client, Reliable)
    void ClientOnActiveSlotChangedDelegate(int32 NewIndex, int32 OldIndex);

    // Called after the inventory has been resized as a result of a call to Resize
    UFUNCTION(Client, Reliable)
    void ClientOnInventorySizeChangedDelegate(int32 NewSize, int32 OldSize);

    // Count the number of items in the inventory that match the given item definition
    // Aggregates all slots
    int32 GetItemCount(UItemDefinition *RequiredItem) const;

    // Remove the given amount of stacks of an item from the inventory. Returns the amount of item stacks that were removed.
    UFUNCTION(Server, Reliable)
    void ServerRemoveItem(UMythicItemInstance *ItemInstance, int32 Amount = 1);

    UFUNCTION(Server, Reliable)
    void ServerRemoveItemByDefinition(UItemDefinition *ItemDefinition, int32 Amount = 1);

protected:
    // Ensure we destroy all objects when components is destroyed/unregistered
    virtual void OnUnregister() override;
};
