#include "MythicItemInstance.h"

#include "MythicInventoryComponent.h"
#include "MythicInventorySlot.h"
#include "Fragments/ItemFragment.h"
#include "Misc/UObjectToken.h"
#include "Mythic/Mythic.h"

void UMythicItemInstance::SetStackSize(const int32 newQuantity) {
    // Authority check
    auto owner = this->GetOwningActor();
    checkf(owner->HasAuthority(), TEXT("Only the server can set the stack size of an item instance"));

    const auto newQty = FMath::Min(newQuantity, ItemDefinition->StackSizeMax);
    if (newQty != Quantity) {
        Quantity = newQty;

        // If the item is in an inventory
        if (this->CurrentSlot && this->CurrentSlot->GetInventory()) {
            if (Quantity <= 0) {
                // Remove the item from the slot if the stack is 0
                // NOTE: Item->Destroy() should be called by the caller of this function by checking if stack size <= 0
                this->GetSlot()->SetItemInstance(nullptr);
            }
            if (auto InventoryComponent = this->CurrentSlot->GetInventory()) {
                InventoryComponent->ClientOnSlotUpdatedDelegate(this->CurrentSlot);
            }
        }
    }
}

// Sets the item definition and quantity, then instantiates fragments from the item definition.
void UMythicItemInstance::Initialize(UItemDefinition *ItemDef, const int32 quantityIfStackable, const int32 level) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can initialize an item instance"));

    this->ItemDefinition = ItemDef;
    this->randomSeed = FMath::Rand();
    this->ItemLevel = level;
    this->Quantity = ItemDef->StackSizeMax > 1 ? quantityIfStackable : 1;
    UE_LOG(Mythic, Warning, TEXT("Level %d item %s has random seed %d"), level, *GetName(), this->randomSeed);

    // Create fragments for this item from the item definition
    for (int i = 0; i < ItemDef->Fragments.Num(); i++) {
        auto Fragment = ItemDef->Fragments[i];
        if (!Fragment) {
            UE_LOG(Mythic, Error, TEXT("ItemInstance %s has a null fragment at index %d"), *GetName(), i);
            continue;
        }

        AddFragment(Fragment);
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

/*
* Item Instance Activation Flow - Server Side
* ------------------------------
* 1. Called by InventorySlot when slot becomes active
* 2. Iterates through all fragments
* 3. Calls OnItemActivated() on each fragment
* 4. Used for gameplay effects like:
*    - Granting abilities
*    - Applying stat modifiers
*    - Triggering gameplay events
*/
void UMythicItemInstance::OnActiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnItemActivated(this);
    }
    UE_LOG(Mythic, Warning, TEXT("ItemInstance %s has random seed %d"), *GetName(), this->randomSeed);
}

void UMythicItemInstance::OnInactiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnItemDeactivated(this);
    }
}

/*
* Item Instance Activation Flow - Client Side
* ------------------------------
* 1. Called by InventorySlot's ClientActivateSlot
* 2. Iterates through all fragments
* 3. Calls OnClientItemActivated() on each fragment
* 4. Used for visual effects like:
*    - Playing animations
*    - Spawning particle effects
*    - Updating UI elements
*/
void UMythicItemInstance::OnClientActiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnClientItemActivated(this);
    }
}

/*
* Item Instance Deactivation Flow - Client Side
* ------------------------------
* 1. Called by InventorySlot's ClientDeactivateSlot
* 2. Iterates through all fragments
* 3. Calls OnClientItemDeactivated() on each fragment
* 4. Used for cleaning up visual effects like:
*    - Stopping animations
*    - Removing particle effects
*    - Reverting UI elements
*/
void UMythicItemInstance::OnClientInactiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnClientItemDeactivated(this);
    }
}

void UMythicItemInstance::SetSlot(TObjectPtr<UMythicInventorySlot> NewSlot) {
    this->CurrentSlot = NewSlot;
    for (TObjectPtr ItemFragment : this->ItemFragments) {
        ItemFragment->OnInventorySlotChanged(NewSlot);
    }
}

TObjectPtr<UMythicInventorySlot> UMythicItemInstance::GetSlot() const {
    return this->CurrentSlot;
}

UMythicInventoryComponent *UMythicItemInstance::GetInventoryComponent() const {
    // Get slot
    auto Slot = this->GetSlot();
    if (!Slot) {
        return nullptr;
    }

    // Get inventory
    return Slot->GetInventory();
}

AActor *UMythicItemInstance::GetInventoryOwner() const {
    if (auto InventoryComponent = GetInventoryComponent()) {
        return InventoryComponent->GetOwner();
    }
    return nullptr;
}

void UMythicItemInstance::AddTag(const FGameplayTag &Tag) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can add tags to an item instance"));

    //check if the tag is already present
    if (HasTag(Tag)) {
        return;
    }

    //add the tag
    ItemTags.AddTag(Tag);
}

void UMythicItemInstance::RemoveTag(const FGameplayTag &Tag) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can remove tags from an item instance"));

    //remove the tag
    ItemTags.RemoveTag(Tag);
}

bool UMythicItemInstance::HasTag(const FGameplayTag &Tag) const {
    return ItemTags.HasTag(Tag);
}

bool UMythicItemInstance::isStackableWith(const UMythicItemInstance *Other) const {
    if (ItemDefinition && ItemDefinition->StackSizeMax <= 0) {
        return false;
    }

    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        if (!ItemFragments[i]->CanBeStackedWith(Other->ItemFragments[i])) {
            return false;
        }
    }

    return true;
}
