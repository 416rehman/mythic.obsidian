// 
#pragma once

#include "CoreMinimal.h"
#include "Net/UnrealNetwork.h"
#include "..\MythicGameplayTags.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "MythicInventorySlot.generated.h"

// forward declare the inventory component method GetSlotType
class UMythicInventoryComponent;

UCLASS(Blueprintable)
class MYTHIC_API UMythicInventorySlot : public UMythicReplicatedObject {
	GENERATED_BODY()
protected:
	UPROPERTY(Replicated)
	TObjectPtr<UMythicItemInstance> ItemInstance;
	
public:
	// Slot type - Item instances can have different functions depending on the slot they are in. For example, a helmet when put in the head slot will give a bonus to defense, but when put in the chest slot will give a bonus to health.
	UPROPERTY(BlueprintReadWrite, Category = "Slot", meta=(Categories="Inventory.Slot"))
	FGameplayTag SlotType = TAG_Inventory_Slot_Default;

	// Permit types of items that can be placed in this slot. To permit all items, leave this tag container empty. For example, a weapon slot may only permit items with the "Weapon" tag, while a chest slot may permit items with the "Armor" tag.
	UPROPERTY(BlueprintReadWrite, Category = "Slot", meta=(Categories="Itemization.Type"))
	FGameplayTagContainer ItemTypeWhitelist = FGameplayTagContainer();

	// Accessor for the relevant inventory component
	UFUNCTION(BlueprintPure, Category = "Slot")
	UMythicInventoryComponent* GetInventory() const
	{
		return OwningInventory;
	}
	
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override
	{
		Super::GetLifetimeReplicatedProps(OutLifetimeProps);
		
		DOREPLIFETIME(UMythicInventorySlot, OwningInventory);
		DOREPLIFETIME(UMythicInventorySlot, ItemInstance);
	}

	// Name of the item (every ItemInstance has a ItemDefinition, which has a name)
	UFUNCTION(BlueprintPure, Category = "Slot")
	UMythicItemInstance* GetItemInstance() const;

	// Set the item instance in this slot. Returns true if the item instance was changed, false if another item instance was already in this slot
	UFUNCTION(Category = "Slot")
	bool SetItemInstance(UMythicItemInstance* NewItemInstance);

	UFUNCTION(Category="Slot")
	void OnSlotActivated();

	UFUNCTION(Category="Slot")
	void OnSlotDeactivated();

	void Initialize(TObjectPtr<UMythicInventoryComponent> InventoryComponent, FGameplayTag newSlotType, FGameplayTagContainer newItemTypeWhitelist);
	
private:
	friend class UMythicInventoryComponent;

protected:
	UPROPERTY(BlueprintReadOnly, Replicated)
	TObjectPtr<UMythicInventoryComponent> OwningInventory;
};