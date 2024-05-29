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
class MYTHIC_API UMythicItemInstance : public UMythicReplicatedObject
{
	GENERATED_BODY()

protected:
	// object pointer to the item definition, expose on spawn
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	TSubclassOf<UItemDefinition> ItemDefinitionClass;

	// Item Fragments copied from the item definition
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	TArray<UItemFragment*> ItemFragments;

	// Quantity of item (Current size of stack), with a setter to make sure its never over the max stack size
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item", meta = (ClampMin = "1"))
	int32 Quantity = 1;

	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	int randomSeed;
	
	UPROPERTY(Replicated, BlueprintReadOnly, Category = "Item")
	TObjectPtr<UMythicInventorySlot> CurrentSlot;	// Reference to the slot this item is in, if any
	
public:	
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override {
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		DOREPLIFETIME(UMythicItemInstance, ItemDefinitionClass);
		DOREPLIFETIME(UMythicItemInstance, Quantity);
		DOREPLIFETIME(UMythicItemInstance, CurrentSlot);
		DOREPLIFETIME(UMythicItemInstance, ItemFragments);
		DOREPLIFETIME(UMythicItemInstance, randomSeed);
	}

	// Set the quantity of the item, clamped to the max stack size
	UFUNCTION(BlueprintCallable, Category = "Item")
	void SetStackSize(const int32 newQuantity);

	// Get the quantity of the item
	UFUNCTION(BlueprintCallable, Category = "Item")
	int32 GetStacks() const { return Quantity; }
	
	// Initialize the item instance
	UFUNCTION(BlueprintCallable, Category = "Item")
	void Initialize(const TSubclassOf<UItemDefinition> ItemDefClass, const int32 quantityIfStackable);

	// Get the item definition
	UFUNCTION(BlueprintCallable, Category = "Item")
	UItemDefinition* GetItemDefinitionObject() const {
	    return ItemDefinitionClass.GetDefaultObject();
	}

    // Get the item definition class
    UFUNCTION(BlueprintCallable, Category = "Item")
    TSubclassOf<UItemDefinition> GetItemDefinitionClass() const {
        return ItemDefinitionClass;
    }

	// Creates a fragment from a fragment config and adds it to the item
	void AddFragment(TObjectPtr<UItemFragment> Fragment);
	
	// the item is the active item in inventory (only one item can be active at a time)
	void OnActiveItem();

	// the item is no longer the active item in inventory
	void OnInactiveItem();

	// Set Slot
	void SetSlot(TObjectPtr<UMythicInventorySlot> NewSlot);
	TObjectPtr<UMythicInventorySlot> GetSlot() const;
};
