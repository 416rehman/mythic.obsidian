#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryProfile.h"
#include "InventorySlotDefinition.h"
#include "MythicInventoryComponent.generated.h"

// sort modes for ServerSortGroup
UENUM(BlueprintType)
enum class ESortMode : uint8 {
    ByRarity,
    ByType,
    ByName,
    ByValue,
    ByWeight
};

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

    // Tag identifying which group this slot belongs to
    UPROPERTY()
    FGameplayTag GroupTag;

    // Index of the entry within the group (for uniqueness scoping)
    UPROPERTY()
    int32 EntryIndex = 0;

    // Cached from entry - if true, enforce unique items within slots of same EntryIndex
    UPROPERTY()
    bool bRequireUniqueInEntry = false;

    // Cached from group - if false, items cannot be dropped/sold/transferred by player
    UPROPERTY()
    bool bCanPlayerTake = true;

    // Cached from group - if false, player cannot insert items manually
    UPROPERTY()
    bool bCanPlayerPut = true;

    // Transient client-side cache of the last item in this slot to handle deactivation
    UPROPERTY(Transient, NotReplicated)
    TObjectPtr<UMythicItemInstance> ClientLastKnownItem = nullptr;

    // Definition of the slot (Icon, Whitelists, Tags)
    UPROPERTY()
    TObjectPtr<UInventorySlotDefinition> SlotDefinition = nullptr;

    void ClientUpdateActiveState(UMythicInventoryComponent* Owner);
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

    // Total carry weight of everything in this inventory: sum of each slotted item's UnitWeight × stack (encumbrance).
    // Reads the replicated FastArray, so it is valid on the server and the owning client. 0 if every item is weightless.
    float GetTotalCarriedWeight() const;

    // Pure per-slot weight contribution: UnitWeight × StackCount with both clamped non-negative (a weightless or
    // malformed entry contributes 0, never a negative). Static so the aggregation rule is unit-testable headlessly.
    static float ComputeSlotWeight(float UnitWeight, int32 StackCount);

    // The CURRENCY this inventory holds = summed stack quantity over Itemization.Type.Currency items (a player's wallet
    // balance, since currency is modelled as stackable currency-type items). 0 if it holds none. Server + owning client.
    int32 GetTotalCurrency() const;

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

    // Checks if a specific slot can accept an item instance, enforcing whitelists and player restrictions.
    bool CanSlotAcceptItem(int32 SlotIndex, UMythicItemInstance *ItemInstance, bool bFromPlayer = false) const;

    // Pure equip-requirement gate: an item with a RequiredEquipTag may only be equipped by an owner whose gameplay
    // tags include it. An empty/invalid RequiredTag always passes (the default — no requirement). Static + no engine
    // state for unit testing. (Range/authority are enforced elsewhere; this is purely the tag-gate.)
    static bool MeetsEquipRequirement(const FGameplayTag &RequiredTag, const FGameplayTagContainer &OwnerTags);

    // Effective-type whitelist check using the item's type probe ({def ItemType} ∪ ItemTags).
    // An empty slot whitelist accepts all types. Single source of truth for slot whitelisting so that
    // runtime-tagged / transformed items are evaluated by their effective type, not just the base def type.
    bool SlotWhitelistAccepts(int32 SlotIndex, const UMythicItemInstance *Inst) const;

    // SERVER-ONLY: detach an instance from its slot WITHOUT destroying it. Clears the slot and the
    // instance's OwningInventory / SlotIndex back-pointers. Returns the released (now ownerless) instance,
    // or nullptr. Distinct from ServerRemoveItem, which destroys the instance on full removal.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    UMythicItemInstance *ReleaseFromSlot(int32 SlotIndex);

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
    int32 AddToAnySlot(UMythicItemInstance *ItemInstance, bool bFromPlayer = false);

    /**
     * Whether AddToAnySlot should attempt to merge an incoming item into existing partial stacks: true iff the item's
     * type is stackable (StackSizeMax > 1). Pure + static so the gate is unit-testable. (Fix: the gate was previously
     * `StackSizeMax > IncomingStacks`, which skipped merging a FULL incoming stack — so picking up a full stack never
     * topped off existing partial stacks, wasting inventory slots. Merging DRAINS the incoming; its own fullness is
     * irrelevant. The per-slot `isStackableWith` check still guards genuine compatibility.)
     */
    static bool ShouldAttemptStackMerge(int32 StackSizeMax) { return StackSizeMax > 1; }

    // Add item to the given slot. If the slot is already occupied, the item will be stacked if possible. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex, bool bFromPlayer = false);

    // Receives an item instance. Should be invoked from the SendItem function by the other inventory. Returns the amount of items that were received. Will remove the item from
    int32 ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, int32 TargetSlotIndex, bool bFromPlayer = false);

    // Sends an item instance to another inventory. Returns the amount of items that were sent, must set the item instance to null if all items were sent.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 SendItem(int32 SlotIndex, UMythicInventoryComponent *TargetInventory, int32 TargetSlotIndex);

    // True if a player is allowed to take the item out of SlotIndex (the slot's bCanPlayerTake flag). SendItem
    // does NOT enforce this (only the target's bCanPlayerPut), so player-initiated transfers must check it.
    UFUNCTION(BlueprintPure, Category = "Slots")
    bool CanPlayerTakeFromSlot(int32 SlotIndex) const;

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

    // Count the number of items in the inventory that match the given item definition (aggregates all slots).
    // BlueprintPure so HUD/vendor widgets can read a currency balance, e.g. GetItemCount(GoldDef).
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(UItemDefinition *RequiredItem) const;

    // Remove the given amount of stacks of an item from the inventory. Server RPC — void (a reliable Server RPC cannot
    // return a value); the removed count is not reported back.
    UFUNCTION(Server, Reliable)
    void ServerRemoveItem(UMythicItemInstance *ItemInstance, int32 Amount = 1);

    UFUNCTION(Server, Reliable)
    void ServerRemoveItemByDefinition(UItemDefinition *ItemDefinition, int32 Amount = 1);

    // split SplitAmount stacks from SourceSlotIndex into the first empty slot in the same group
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerSplitStack(int32 SourceSlotIndex, int32 SplitAmount);

    // swap items between two slots, handling equipment activation/deactivation and empty slot moves
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerSwapSlots(int32 SlotA, int32 SlotB);

    // move item from this inventory to a target inventory using AddToAnySlot
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerQuickMoveToInventory(int32 SourceSlotIndex, UMythicInventoryComponent *TargetInventory);

    // sort all items in slots matching GroupTag by the specified mode
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerSortGroup(FGameplayTag GroupTag, ESortMode Mode);

    // move all non-equipment items (optionally filtered by type tag) to the target inventory
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerDepositAll(UMythicInventoryComponent *Target, FGameplayTag OptionalTypeFilter);

    // use a consumable item directly from inventory without equipping it
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerUseItemInSlot(int32 SlotIndex);

    // returns true if the item in the slot has actionable fragments that support in-inventory use
    UFUNCTION(BlueprintPure, Category = "Slots")
    bool CanUseItemInSlot(int32 SlotIndex) const;

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
