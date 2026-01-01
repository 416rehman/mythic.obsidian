#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryProfile.h"
#include "InventorySlotDefinition.h"
#include "MythicInventoryComponent.generated.h"

class UInventoryVM;
class AMythicWorldItem;

// delegate for active slot changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveSlotChanged, int32, NewIndex, int32, OldIndex);

// delegate for slot updated - called when an item is added or removed from a slot
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotUpdated, int32, Slot);

// delegate for inventory size changed
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySizeChanged, int32, NewSize, int32, OldSize);

// delegate for item dropped - called when an item is dropped from the inventory
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDropped, int32, Slot, AMythicWorldItem*, WorldItem);

// delegate for ViewModel created without any parameters
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewModelCreated);

USTRUCT(BlueprintType, Blueprintable)
struct FMythicInventorySlotEntry : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UMythicItemInstance> SlottedItemInstance = nullptr;

    UPROPERTY()
    bool bEquipmentSlot = false;

    // Transient client-side cache of the last item in this slot to handle deactivation
    UPROPERTY(Transient, NotReplicated)
    TObjectPtr<UMythicItemInstance> ClientLastKnownItem = nullptr;

    // Definition of the slot (Icon, Whitelists, Tags)
    UPROPERTY()
    TObjectPtr<UInventorySlotDefinition> SlotDefinition = nullptr;

    void ClientUpdateActiveState();
    void ServerUpdateActiveState();
    void Clear();
};

// Forward declare component for owner back-reference
class UMythicInventoryComponent;

// FastArray container
USTRUCT(BlueprintType)
struct FMythicInventoryFastArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicInventorySlotEntry> Items = TArray<FMythicInventorySlotEntry>();

    // Non-replicated owner pointer for callbacks
    UPROPERTY(Transient)
    TObjectPtr<UMythicInventoryComponent> Owner = nullptr;

    const TArray<FMythicInventorySlotEntry> &GetItems() const { return Items; }
    int32 Num() const { return Items.Num(); }
    bool IsValidIndex(int32 Index) const { return Items.IsValidIndex(Index); }

    void AddSlot(const FMythicInventorySlotEntry &NewSlot);

    void RemoveSlotAt(int32 Index);

    void ModifySlotAtIndex(int32 Index, const TFunction<void(FMythicInventorySlotEntry &SlotData)> &Modifier);

    TObjectPtr<UMythicItemInstance> GetItemInSlot(int32 Index) const {
        if (Items.IsValidIndex(Index)) { return Items[Index].SlottedItemInstance; }
        return nullptr;
    }

    // Callback entry points
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);

    // Helper to set owner for callbacks
    FORCEINLINE void SetOwningInventory(UMythicInventoryComponent *InOwner) {
        Owner = InOwner;
    }

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FMythicInventorySlotEntry, FMythicInventoryFastArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FMythicInventoryFastArray> : TStructOpsTypeTraitsBase2<FMythicInventoryFastArray> {
    enum { WithNetDeltaSerializer = true, };
};

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicInventoryComponent : public UActorComponent {
    // View Model ////////////////////
protected:
    UPROPERTY(Transient, BlueprintReadOnly, Category = "ViewModel")
    UInventoryVM *ViewModel = nullptr;

    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ViewModel")
    FName ViewModelIdentifier = FName();

public:
    // Create or refresh the local ViewModel on this instance (no RPC)
    UFUNCTION(BlueprintCallable, Category="ViewModel")
    void SetupLocalViewModel();

    UPROPERTY(BlueprintAssignable, Category = "ViewModel")
    FOnViewModelCreated OnViewModelCreated;
    // View Model End ////////////////////
protected:
    GENERATED_BODY()

    /*
    * Fast array serializer for inventory slots
    */
    /*
    * Fast array serializer for inventory slots
    */
    UPROPERTY(ReplicatedUsing=OnRep_Slots, BlueprintReadOnly, Category = "Slots")
    FMythicInventoryFastArray Slots = FMythicInventoryFastArray();

    UFUNCTION()
    void OnRep_Slots();



public:
    // Inventory Profile to use for initialization
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Slots")
    TObjectPtr<UInventoryProfile> InventoryProfile;

    // Lightweight read-only accessors for UI/ViewModel code
    FORCEINLINE int32 GetNumSlots() const { return Slots.Num(); }
    bool GetSlotEntry(int32 Index, FMythicInventorySlotEntry &OutEntry) const;

    // Get all slots
    const TArray<FMythicInventorySlotEntry> &GetAllSlots() const { return Slots.GetItems(); }
    TArray<FMythicInventorySlotEntry> &GetAllSlotsMutable() { return Slots.Items; }

    UMythicInventoryComponent(const FObjectInitializer &OI);

    virtual void BeginPlay() override;
    void InitializeSlots();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UMythicInventoryComponent, Slots);
    }

    // --- Save System Interface Removed ---

    // Checks if any slot in this inventory can accept an item of the given type.
    UFUNCTION(BlueprintCallable, Category = "Slots")
    bool CanAcceptItemType(const FGameplayTag &ItemType) const;

    // Returns the item in the given slot
    UFUNCTION(BlueprintCallable, Category = "Slots")
    UMythicItemInstance *GetItem(int32 SlotIndex);

    // Make sure the TargetSlot is valid and has no items in it. Will set the target slot's item instance to the given item instance and the item instance's slot to the target slot.
    // Returns the amount of items that were transferred. 0 if the transfer failed.
    bool TryTransferToSlot(UMythicItemInstance *ItemInstance, int32 TargetSlotIndex);

    bool SetItemInSlot(int32 SlotIndex, UMythicItemInstance *ItemInstance);

    /**
     * Internal version of SetItemInSlot that can be used by systems like Save/Load 
     * to bypass or customize standard checks.
     */
    bool SetItemInSlotInternal(int32 SlotIndex, UMythicItemInstance *ItemInstance);

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
    int32 ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, int32 TargetSlotIndex);

    // Sends an item instance to another inventory. Returns the amount of items that were sent, must set the item instance to null if all items were sent.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 SendItem(int32 SlotIndex, UMythicInventoryComponent *TargetInventory, int32 TargetSlotIndex);

    // Drops an item instance to the ground through a WorldItem. Returns true if the item was dropped.
    // If TargetRecipient is provided, only they can interact with the item. Otherwise, all players can interact with the item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool DropItem(int32 SlotIndex, const FVector &location, float radius = 100.0f, AController *TargetRecipient = nullptr);

    // Picks up a WorldItem. Returns the amount of items (stacks) that were picked up.
    UFUNCTION(BlueprintCallable, Server, Reliable, BlueprintAuthorityOnly, Category = "Slots")
    void PickupItem(AMythicWorldItem *world_item);

    /** DYNAMIC INVENTORY API */

    // Adds new slots of the given definition to the inventory.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void AddSlot(UInventorySlotDefinition *SlotDefinition, int32 Count = 1);

    // Removes slots of the given definition from the inventory.
    // NOTE: This will destroy items in those slots unless bDropItems is true!
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool RemoveSlot(UInventorySlotDefinition *SlotDefinition, int32 Count = 1, bool bDropItems = false);

    /** END DYNAMIC INVENTORY API */

    /** Delegates */
    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnSlotUpdated OnSlotUpdated;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnInventorySizeChanged OnInventorySizeChanged;

    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnItemDropped OnItemDropped;

    UFUNCTION(BlueprintPure, Category = "ViewModel")
    UInventoryVM *GetViewModel() const;

    // Count the number of items in the inventory that match the given item definition
    // Aggregates all slots
    int32 GetItemCount(UItemDefinition *RequiredItem) const;

    // Remove the given amount of stacks of an item from the inventory. Returns the amount of item stacks that were removed.
    UFUNCTION(Server, Reliable)
    void ServerRemoveItem(UMythicItemInstance *ItemInstance, int32 Amount = 1);

    UFUNCTION(Server, Reliable)
    void ServerRemoveItemByDefinition(UItemDefinition *ItemDefinition, int32 Amount = 1);

    // Called by item instances when a replicated field changed (e.g., quantity)
    void NotifyItemInstanceUpdated(int32 SlotIndex);

protected:
    bool DestroySlot(int32 SlotIndex);

    void DestroyAllSlots();

    // Ensure we destroy all objects when components is destroyed/unregistered
    virtual void OnUnregister() override;

    // FastArray replication callbacks forwarded from Slots
    void HandleSlotsAdded(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void HandleSlotsChanged(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void HandleSlotsRemoved(const TArrayView<int32> &RemovedIndices, int32 FinalSize);

    // friend the fast array struct to allow it to call our callbacks
    friend struct FMythicInventoryFastArray;
};
