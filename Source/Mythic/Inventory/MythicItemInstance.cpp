#include "MythicItemInstance.h"

#include "MythicInventoryComponent.h"
#include "MythicInventorySlot.h"
#include "Fragments/GAS/GameplayAbilityFragment.h"
#include "Mythic/Mythic.h"

void UMythicItemInstance::SetStackSize(const int32 newQuantity) {
	const auto newQty = FMath::Min(newQuantity, ItemDefinitionClass.GetDefaultObject()->StackSizeMax);
	if (newQty != Quantity) {
		Quantity = newQty;
		if (this->CurrentSlot && this->CurrentSlot->GetInventory()) {
			this->CurrentSlot->GetInventory()->OnSlotUpdated.Broadcast(this->CurrentSlot);
		}
	}
}

// Sets the item definition and quantity, then instantiates fragments from the item definition.
void UMythicItemInstance::Initialize(const TSubclassOf<UItemDefinition> ItemDefClass, const int32 quantityIfStackable) {
	checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can initialize an item instance"));

	ItemDefinitionClass = ItemDefClass;
	this->randomSeed = FMath::Rand();
	UE_LOG(Mythic, Warning, TEXT("ItemInstance %s has random seed %d"), *GetName(), this->randomSeed);
	Quantity = ItemDefClass.GetDefaultObject()->StackSizeMax > 1 ? quantityIfStackable : 1;

	// Create fragments for this item from the item definition
	for (int i = 0; i < ItemDefClass.GetDefaultObject()->Fragments.Num(); i++) {
		if (!ItemDefClass.GetDefaultObject()->Fragments[i]) {
			UE_LOG(Mythic, Error, TEXT("ItemInstance %s has an invalid fragment at index %d"), *GetName(), i);
			continue;
		}

		AddFragment(ItemDefClass.GetDefaultObject()->Fragments[i]);
	}
}


void UMythicItemInstance::AddFragment(TObjectPtr<UItemFragment> FragmentSource) {
	auto owner = this->GetOwningActor();
	checkf(owner->HasAuthority(), TEXT("Only the server can add fragments to an item instance"));
	
	auto Fragment = NewObject<UItemFragment>(this, FragmentSource.GetClass(), NAME_None, RF_NoFlags, FragmentSource);
	Fragment->SetOwner(owner); // this object's owner will be responsible for replicating it

	ItemFragments.Add(Fragment);

	Fragment->OnInstanced(this);
}

void UMythicItemInstance::OnActiveItem() {
	for (int i = 0; i < ItemFragments.Num(); i++) {
		if (ItemFragments[i] == nullptr) { continue; }
		ItemFragments[i]->OnActiveItem(this);
	}
	UE_LOG(Mythic, Warning, TEXT("ItemInstance %s has random seed %d"), *GetName(), this->randomSeed);
}

void UMythicItemInstance::OnInactiveItem() {
	for (int i = 0; i < ItemFragments.Num(); i++) {
		if (ItemFragments[i] == nullptr) { continue; }
		ItemFragments[i]->OnInactiveItem(this);
	}
}

void UMythicItemInstance::SetSlot(TObjectPtr<UMythicInventorySlot> NewSlot) {
	UE_LOG(Mythic, Warning, TEXT("ItemInstance %s moved from slot %s to slot %s"), *GetName(), CurrentSlot ? *CurrentSlot->GetName() : TEXT("None"), NewSlot ? *NewSlot->GetName() : TEXT("None"));
	auto oldSlot = CurrentSlot;
	this->CurrentSlot = NewSlot;
	for (UItemFragment* Fragment : ItemFragments) {
		Fragment->OnSlotChanged(NewSlot, oldSlot);
	}
	this->randomSeed = FMath::Rand();
	UE_LOG(Mythic, Warning, TEXT("ItemInstance %s has random seed %d"), *GetName(), this->randomSeed);
}

TObjectPtr<UMythicInventorySlot> UMythicItemInstance::GetSlot() const {
	return this->CurrentSlot;
}
