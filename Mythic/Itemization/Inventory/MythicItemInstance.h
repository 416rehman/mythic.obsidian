#pragma once

#include "CoreMinimal.h"
#include "ItemDefinition.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "Net/UnrealNetwork.h"
#include "MythicItemInstance.generated.h"

class UItemFragment;
class UMythicInventorySlot;

// Call Initialize after spawning to set the item definition and quantity
UCLASS(Blueprintable, BlueprintType)
class MYTHIC_API UMythicItemInstance : public UMythicReplicatedObject {
    GENERATED_BODY()

protected:
    // object pointer to the item definition data asset
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    UItemDefinition *ItemDefinition;

    // Item Fragments copied from the item definition
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    TArray<TObjectPtr<UItemFragment>> ItemFragments;

    // Quantity of item (Current size of stack), with a setter to make sure its never over the max stack size
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
    int32 Quantity = 1;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    int randomSeed;

    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    TObjectPtr<UMythicInventorySlot> CurrentSlot; // Reference to the slot this item is in, if any

    // The level of the item
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    int32 ItemLevel = 1;

    // Tags assigned to the item - these are dynamic and can be changed at runtime
    UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
    FGameplayTagContainer ItemTags;

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UMythicItemInstance, ItemDefinition);
        DOREPLIFETIME(UMythicItemInstance, Quantity);
        DOREPLIFETIME(UMythicItemInstance, CurrentSlot);
        DOREPLIFETIME(UMythicItemInstance, ItemFragments);
        DOREPLIFETIME(UMythicItemInstance, randomSeed);
        DOREPLIFETIME(UMythicItemInstance, ItemLevel);
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

    // Set Slot
    void SetSlot(TObjectPtr<UMythicInventorySlot> NewSlot);
    TObjectPtr<UMythicInventorySlot> GetSlot() const;

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
};
