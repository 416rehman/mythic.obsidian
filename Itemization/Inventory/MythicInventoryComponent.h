#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "Components/ActorComponent.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "InventoryProfile.h"
#include "InventorySlotDefinition.h"
#include "MythicInventoryComponent.generated.h"

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
class UMythicAffixApplicationComponent;
struct FRolledAffix;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnActiveSlotChanged, int32, NewIndex, int32, OldIndex);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotUpdated, int32, Slot);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnInventorySizeChanged, int32, NewSize, int32, OldSize);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnItemDropped, int32, Slot, AMythicWorldItem*, WorldItem);

DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnViewModelCreated);

USTRUCT(BlueprintType, Blueprintable)
struct FMythicInventorySlotEntry : public FFastArraySerializerItem {
    GENERATED_BODY()

    UPROPERTY()
    TObjectPtr<UMythicItemInstance> SlottedItemInstance = nullptr;

    /** Replicated profile-authored slot domain; current profile data initializes it and save payloads never override it. */
    UPROPERTY()
    EMythicInventorySlotDomain SlotDomain = EMythicInventorySlotDomain::Carried;

    UPROPERTY()
    FGameplayTag GroupTag;

    UPROPERTY()
    int32 EntryIndex = 0;

    UPROPERTY()
    bool bRequireUniqueInEntry = false;

    UPROPERTY()
    bool bCanPlayerTake = true;

    UPROPERTY()
    bool bCanPlayerPut = true;

    UPROPERTY(Transient, NotReplicated)
    TObjectPtr<UMythicItemInstance> ClientLastKnownItem = nullptr;

    UPROPERTY()
    TObjectPtr<UInventorySlotDefinition> SlotDefinition = nullptr;

    /** True for gear slots - armor, accessories, weapon, tools - whose item is active solely by slot membership. */
    bool IsGearSlot() const { return SlotDomain == EMythicInventorySlotDomain::Equipment; }

    void ClientUpdateActiveState(UMythicInventoryComponent* Owner);
    void ServerUpdateActiveState();
    void Clear();
};

class UMythicInventoryComponent;

USTRUCT(BlueprintType)
struct FMythicInventoryFastArray : public FFastArraySerializer {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FMythicInventorySlotEntry> Items = TArray<FMythicInventorySlotEntry>();

    UPROPERTY(Transient)
    TObjectPtr<UMythicInventoryComponent> Owner = nullptr;

    const TArray<FMythicInventorySlotEntry> &GetItems() const { return Items; }
    int32 Num() const { return Items.Num(); }
    bool IsValidIndex(int32 Index) const { return Items.IsValidIndex(Index); }

    void AddSlot(const FMythicInventorySlotEntry &NewSlot);

    /** Add without the per-slot server notify; pair with NotifyServerBatchAdded after the last add. */
    void AddSlotSilent(const FMythicInventorySlotEntry &NewSlot);

    /**
     * One server-side callback for every slot added since StartIndex, matching how replication batches the
     * same adds for clients. The per-add path rebuilt the local ViewModel once per slot - a hundred-slot
     * init meant a hundred full rebuilds.
     */
    void NotifyServerBatchAdded(int32 StartIndex);

    void RemoveSlotAt(int32 Index);

    void ModifySlotAtIndex(int32 Index, const TFunction<void(FMythicInventorySlotEntry &SlotData)> &Modifier);

    TObjectPtr<UMythicItemInstance> GetItemInSlot(int32 Index) const {
        if (Items.IsValidIndex(Index)) { return Items[Index].SlottedItemInstance; }
        return nullptr;
    }

    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);

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
protected:
    /** Local presentation model built for this inventory on clients that render it. */
    UPROPERTY(Transient, BlueprintReadOnly, Category = "ViewModel")
    UInventoryVM *ViewModel = nullptr;

    /** View-model collection key used when publishing this inventory to the UI layer. */
    UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "ViewModel")
    FName ViewModelIdentifier = FName();

public:
    // Create or refresh the local ViewModel on this instance (no RPC)
    UFUNCTION(BlueprintCallable, Category="ViewModel")
    void SetupLocalViewModel();

    /** Broadcast locally after the inventory view model has been created or refreshed. */
    UPROPERTY(BlueprintAssignable, Category = "ViewModel")
    FOnViewModelCreated OnViewModelCreated;
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

    FORCEINLINE int32 GetNumSlots() const { return Slots.Num(); }
    bool GetSlotEntry(int32 Index, FMythicInventorySlotEntry &OutEntry) const;

    const TArray<FMythicInventorySlotEntry> &GetAllSlots() const { return Slots.GetItems(); }
    TArray<FMythicInventorySlotEntry> &GetAllSlotsMutable() { return Slots.Items; }

    float GetTotalCarriedWeight() const;

    static float ComputeSlotWeight(float UnitWeight, int32 StackCount);

    // The CURRENCY this inventory holds = summed stack quantity over Itemization.Type.Currency items (a player's wallet
    // balance, since currency is modelled as stackable currency-type items). 0 if it holds none. Server + owning client.
    // BlueprintPure so a trade/HUD widget can show the player's purse — without it no UI could read the wallet at all.
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetTotalCurrency() const;

    int32 SpendCurrency(int32 Amount);

    UMythicInventoryComponent(const FObjectInitializer &OI);

    virtual void BeginPlay() override;
    void InitializeSlots();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UMythicInventoryComponent, Slots);
    }


    // Checks if any slot in this inventory can accept an item of the given type.
    UFUNCTION(BlueprintCallable, Category = "Slots")
    bool CanAcceptItemType(const FGameplayTag &ItemType) const;

    bool CanSlotAcceptItem(int32 SlotIndex, UMythicItemInstance *ItemInstance, bool bFromPlayer = false) const;

    static bool MeetsEquipRequirement(const FGameplayTag &RequiredTag, const FGameplayTagContainer &OwnerTags);

    bool SlotWhitelistAccepts(int32 SlotIndex, const UMythicItemInstance *Inst) const;

    // SERVER-ONLY: detach an instance from its slot WITHOUT destroying it. Clears the slot and the
    // instance's OwningInventory / SlotIndex back-pointers. Returns the released (now ownerless) instance,
    // or nullptr. Distinct from ServerRemoveItem, which destroys the instance on full removal.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    UMythicItemInstance *ReleaseFromSlot(int32 SlotIndex);

    // Returns the item in the given slot
    UFUNCTION(BlueprintCallable, Category = "Slots")
    UMythicItemInstance *GetItem(int32 SlotIndex);

    bool TryTransferToSlot(UMythicItemInstance *ItemInstance, int32 TargetSlotIndex);

    bool SetItemInSlot(int32 SlotIndex, UMythicItemInstance *ItemInstance);

    bool SetItemInSlotInternal(int32 SlotIndex, UMythicItemInstance *ItemInstance);

    /** Applies a staged base-affix snapshot set to GAS while the equipped item's replicated data is still unchanged. */
    bool ReconcileEquippedAffixSnapshotMutationTransactional(
        UMythicItemInstance *ItemInstance,
        TConstArrayView<FRolledAffix> ProposedSnapshots) const;

    // Add to inventory. Will stack if possible, otherwise will add to any available slot, and if no room is available, will drop the item to the ground. Returns pointer to the dropped item.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
    AMythicWorldItem *AddItem(UMythicItemInstance *ItemInstance, AController *TargetRecipient);

    // Add item to any available slot. Will stack if possible (if a full transfer occurs through stacking, the item instance will be destroyed).
    // Removes from owner's subobject list on success. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToAnySlot(UMythicItemInstance *ItemInstance, bool bFromPlayer = false);

    // Tell the owning player they gained an item: the loot feed line plus the Item Acquired gameplay event.
    // Any path that inserts with AddToAnySlot rather than AddItem must call this, or the gain is silent and
    // nothing keyed on Item Acquired ever fires.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void NotifyOwnerItemAcquired(const UItemDefinition *ItemDef, int32 Quantity);

    static bool ShouldAttemptStackMerge(int32 StackSizeMax) { return StackSizeMax > 1; }

    // Add item to the given slot. If the slot is already occupied, the item will be stacked if possible. Returns the amount of items that were added.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    int32 AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex, bool bFromPlayer = false);

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


    // Adds new slots of the given definition to the inventory.
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    void AddSlot(UInventorySlotDefinition *SlotDefinition, int32 Count = 1);

    // Removes slots of the given definition from the inventory.
    // NOTE: This will destroy items in those slots unless bDropItems is true!
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Slots")
    bool RemoveSlot(UInventorySlotDefinition *SlotDefinition, int32 Count = 1, bool bDropItems = false);


    /** Delegates */
    /** Broadcast when the authoritative contents or presentation state of one slot changes. */
    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnSlotUpdated OnSlotUpdated;

    /** Broadcast after the inventory gains or loses addressable slots. */
    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnInventorySizeChanged OnInventorySizeChanged;

    /** Broadcast after an inventory item is successfully transferred into a world pickup. */
    UPROPERTY(BlueprintAssignable, Category = "Slots")
    FOnItemDropped OnItemDropped;

    /** Returns the local presentation model created for this inventory, or null before setup. */
    UFUNCTION(BlueprintPure, Category = "ViewModel")
    UInventoryVM *GetViewModel() const;

    // Count the number of items in the inventory that match the given item definition (aggregates all slots).
    // BlueprintPure so HUD/vendor widgets can read a currency balance, e.g. GetItemCount(GoldDef).
    UFUNCTION(BlueprintPure, Category = "Inventory")
    int32 GetItemCount(UItemDefinition *RequiredItem) const;

    UFUNCTION(Server, Reliable)
    void ServerRemoveItem(UMythicItemInstance *ItemInstance, int32 Amount = 1);

    UFUNCTION(Server, Reliable)
    void ServerRemoveItemByDefinition(UItemDefinition *ItemDefinition, int32 Amount = 1);

    // split SplitAmount stacks from SourceSlotIndex into the first empty slot in the same group
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerSplitStack(int32 SourceSlotIndex, int32 SplitAmount);

    int32 SplitStackToFreeSlot(int32 SourceSlotIndex, int32 SplitAmount);

    // swap items between two slots, handling equipment activation/deactivation and empty slot moves
    UFUNCTION(Server, Reliable, BlueprintCallable, Category = "Slots")
    void ServerSwapSlots(int32 SlotA, int32 SlotB);

    /** Authoritative result-bearing implementation used by the RPC and tests/callers that need commit status. */
    bool TrySwapSlotsTransactional(int32 SlotA, int32 SlotB);

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

    void NotifyItemInstanceUpdated(int32 SlotIndex, bool bReconcileAffixes = true);

protected:
    struct FStagedSlotMutation {
        UMythicInventoryComponent *Inventory = nullptr;
        int32 SlotIndex = INDEX_NONE;
        UMythicItemInstance *ExpectedItem = nullptr;
        UMythicItemInstance *ProposedItem = nullptr;
    };

    static bool CommitSlotMutationsTransactional(TConstArrayView<FStagedSlotMutation> Mutations);
    static bool ValidateFinalSlotLayout(TConstArrayView<FStagedSlotMutation> Mutations);
    UMythicAffixApplicationComponent *ResolveAffixApplicationComponent() const;

    bool DestroySlot(int32 SlotIndex);

    void DestroyAllSlots();

    virtual void OnUnregister() override;

    void HandleSlotsAdded(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void HandleSlotsChanged(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
    void HandleSlotsRemoved(const TArrayView<int32> &RemovedIndices, int32 FinalSize);

    friend struct FMythicInventoryFastArray;
};
