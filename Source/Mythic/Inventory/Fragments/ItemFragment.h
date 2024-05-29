#pragma once
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "Mythic/Utility/MythicReplicatedObject.h"
#include "ItemFragment.generated.h"

class UMythicItemInstance;
class UMythicInventorySlot;
class UMythicInventoryComponent;

/**
 *ItemFragments are actors unique to each item instance, and are used to define the item's behavior.
 *These receive events such as OnInstanced, OnPutInSlot, OnRemovedFromSlot, OnActiveItem, OnInactiveItem, etc.
 */
UCLASS(DefaultToInstanced, BlueprintType, Blueprintable, EditInlineNew, Abstract)
class MYTHIC_API UItemFragment  : public UMythicReplicatedObject {
	GENERATED_BODY()
public:
	// This is called after the item fragment has been instanced and added to an item instance. Sets the Config property.
	virtual void OnInstanced(UMythicItemInstance* Instance) {}
	virtual void OnPutInSlot(TObjectPtr<UMythicInventorySlot> newSlot) {}
	virtual void OnRemovedFromSlot(TObjectPtr<UMythicInventorySlot> oldSlot, bool bIsSwappingBetweenSlots) {}
	virtual void OnActiveItem(UMythicItemInstance* ItemInstance){}
	virtual void OnInactiveItem(UMythicItemInstance* ItemInstance){}
	virtual void OnSlotChanged(UMythicInventorySlot* newSlot, UMythicInventorySlot* oldSlot) {}
};

// A fragment that comes with a primary and secondary action. Should only contain self-dependent logic.
// DO: (i.e an apple has a "eat" action, this is a self-dependent action, and thus this action should be defined here)
// DON'T: (i.e an item that can be sold has a "sell" action, but the action depends on a vendor to sell the item to, and thus this action should not be defined here.				Instead create a "Price" fragment, and use that in the sell logic in the vendor)
UCLASS(DefaultToInstanced, BlueprintType, Abstract, Blueprintable, EditInlineNew)
class MYTHIC_API UActionableItemFragment : public UItemFragment
{
	GENERATED_BODY()
public:
	// UI Only: ActionTypeTags can be used in widgets to see if an action is available for an item.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action", meta=(Categories="Itemization.ActionType"))
	FGameplayTagContainer ActionTypeTags;

	// UI Only: ActionRowName is used to bind Action() from a widget to a specific row in an action datatable.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action")
	FDataTableRowHandle ActionRowName;

	UFUNCTION(BlueprintCallable, Category = "Action")
	virtual void Action(UMythicItemInstance* ItemInstance) {}
};

