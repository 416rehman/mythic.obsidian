#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "Net/UnrealNetwork.h"
#include "MythicItemInstance.generated.h"

class UItemFragment;
class UMythicInventorySlot;
class UMythicInventoryComponent;

// Call Initialize after spawning to set the item definition and quantity
UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UMythicItemInstance : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    // object pointer to the item definition data asset
    UPROPERTY(ReplicatedUsing=OnRep_ItemDefinition, BlueprintReadOnly, Category = "Item", SaveGame)
    UItemDefinition *ItemDefinition;

    // Item Fragments copied from the item definition
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    TArray<TObjectPtr<UItemFragment>> ItemFragments;

    // Quantity of item (Current size of stack), with a setter to make sure its never over the max stack size
    UPROPERTY(ReplicatedUsing=OnRep_Quantity, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"), SaveGame)
    int32 Quantity = 1;

    UPROPERTY(ReplicatedUsing=OnRep_OwningInventory, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UMythicInventoryComponent> OwningInventory;

    UPROPERTY(ReplicatedUsing=OnRep_SlotIndex, BlueprintReadOnly, Category = "Item")
    int32 SlotIndex = -1;

    // The level of the item
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", SaveGame)
    int32 ItemLevel = 1;

    // Tags assigned to the item - these are dynamic and can be changed at runtime
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", SaveGame)
    FGameplayTagContainer ItemTags;

    UFUNCTION()
    void OnRep_Quantity();

    UFUNCTION()
    void OnRep_ItemDefinition();

    UFUNCTION()
    void OnRep_OwningInventory();

    UFUNCTION()
    void OnRep_SlotIndex();

public:
    virtual void Serialize(FArchive &Ar) override;

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UMythicItemInstance, ItemDefinition);
        DOREPLIFETIME(UMythicItemInstance, Quantity);
        DOREPLIFETIME(UMythicItemInstance, OwningInventory);
        DOREPLIFETIME(UMythicItemInstance, SlotIndex);
        DOREPLIFETIME(UMythicItemInstance, ItemFragments);
        DOREPLIFETIME(UMythicItemInstance, ItemLevel);
        DOREPLIFETIME(UMythicItemInstance, ItemTags);
        // was flagged Replicated but never registered → runtime tags (e.g. ore→ingot transforms) never reached clients, and it tripped the "marked for replication but not registered" validation
    }

    // Set the quantity of the item, clamped to the max stack size - Should be called via LootSubsystem
    void SetStackSize(const int32 newQuantity);

    // Get the quantity of the item
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetStacks() const { return Quantity; }

    // Get the level of the item
    UFUNCTION(BlueprintCallable, Category = "Item")
    int32 GetItemLevel() const { return ItemLevel; }

    // Initialize the item instance
    void Initialize(UItemDefinition *ItemDef, const int32 quantityIfStackable, const int32 level);

    // Get the item definition
    UFUNCTION(BlueprintCallable, Category = "Item")
    UItemDefinition *GetItemDefinition() const {
        return ItemDefinition;
    }

    // Creates a fragment from a fragment config and adds it to the item
    void AddFragment(TObjectPtr<UItemFragment> Fragment);

    // the item is the active item in inventory (only one item can be active at a time)
    void OnActiveItem();

    // the item is no longer the active item in inventory
    void OnInactiveItem();

    // Client-side activation methods
    void OnClientActiveItem();
    void OnClientInactiveItem();
    void SetInventory(UMythicInventoryComponent *NewInventory, int32 NewSlotIndex);

    // Set Slot
    int32 GetSlot() const;

    // Get Inventory Component, can be null if the item is not in an inventory
    UFUNCTION(BlueprintCallable, Category = "Item")
    UMythicInventoryComponent *GetInventoryComponent() const;

    // Get Inventory Owner, can be null if the item is not in an inventory
    UFUNCTION(BlueprintCallable, Category = "Item")
    AActor *GetInventoryOwner() const;

    // Add a tag to the item
    void AddTag(const FGameplayTag &Tag);

    // Remove a tag from the item
    void RemoveTag(const FGameplayTag &Tag);

    // Check if the item has a tag
    bool HasTag(const FGameplayTag &Tag) const;

    // Read-only access to the item's runtime tags (used by conversion ingredient matching + slot whitelisting).
    const FGameplayTagContainer &GetItemTags() const { return ItemTags; }

    // Centralized "effective type" probe: {Definition->ItemType} ∪ ItemTags.
    // Single source of truth for type matching (conversion ingredients) and slot whitelisting.
    void GetTypeProbe(FGameplayTagContainer &Out) const;

    // SERVER-ONLY: transform THIS instance in place, preserving fragments / level / seed / quantity.
    // Adds/removes runtime type tags (and optionally swaps the definition), then notifies the owning slot
    // exactly once if currently slotted. Used by conversion "Transform" products.
    void ServerApplyTransform(const FGameplayTag &NewItemType,
                              const FGameplayTagContainer &TagsToAdd,
                              const FGameplayTagContainer &TagsToRemove,
                              UItemDefinition *OptionalNewDef);

    bool isStackableWith(const UMythicItemInstance *Other) const;

    // Templated Item Fragment getter
    template <typename T>
    const T *GetFragment() {
        for (const UItemFragment *frag : this->ItemFragments) {
            if (auto casted = Cast<T>(frag)) {
                return casted;
            }
        }

        return nullptr;
    }

    /// Consume the item by reducing its stack size by StackQty. If in inventory, uses inventory's ServerRemoveItem. 
    /// On zero stack, inventory will remove the item from inventory, if not in inventory, destroys world item.
    void ConsumeItem(int32 StackQty = 1);

    /// on destroy, do proper chain of destruction
    virtual void OnDestroyed() override;
};
