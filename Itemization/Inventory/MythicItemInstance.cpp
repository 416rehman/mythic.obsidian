#include "MythicItemInstance.h"

#include "MythicInventoryComponent.h"
#include "Fragments/ItemFragment.h"
#include "Mythic/Mythic.h"

void UMythicItemInstance::Serialize(FArchive &Ar) {
    Super::Serialize(Ar);

    if (Ar.IsSaveGame()) {
        if (Ar.IsLoading()) {
            int32 FragCount = 0;
            Ar << FragCount;

            ItemFragments.Empty(FragCount);

            TArray<UItemFragment *> UsedTemplates;
            for (int32 i = 0; i < FragCount; ++i) {
                FSoftClassPath FragClassPath;
                Ar << FragClassPath;

                UClass *FragClass = FragClassPath.TryLoadClass<UItemFragment>();
                if (FragClass) {
                    UItemFragment *Template = nullptr;
                    if (ItemDefinition) {
                        for (UItemFragment *DefFrag : ItemDefinition->Fragments) {
                            if (DefFrag && DefFrag->GetClass() == FragClass && !UsedTemplates.Contains(DefFrag)) {
                                Template = DefFrag;
                                UsedTemplates.Add(DefFrag);
                                break;
                            }
                        }
                    }

                    UItemFragment *NewFrag = NewObject<UItemFragment>(this, FragClass, NAME_None, RF_NoFlags, Template);
                    NewFrag->Serialize(Ar);
                    NewFrag->SetOwnerItemInstance(this);
                    ItemFragments.Add(NewFrag);
                }
                else {
                    UE_LOG(MythSaveLoad, Error, TEXT("Failed to load fragment class %s during deserialization"), *FragClassPath.ToString());
                }
            }
        }
        else if (Ar.IsSaving()) {
            int32 FragCount = ItemFragments.Num();
            Ar << FragCount;

            for (UItemFragment *Frag : ItemFragments) {
                if (Frag) {
                    FSoftClassPath FragClassPath(Frag->GetClass());
                    Ar << FragClassPath;
                    Frag->Serialize(Ar);
                }
            }
        }
    }
}

void UMythicItemInstance::SetStackSize(const int32 newQuantity) {
    // Authority check
    auto owner = this->GetOwningActor();
    checkf(owner->HasAuthority(), TEXT("Only the server can set the stack size of an item instance"));

    const auto newQty = FMath::Min(newQuantity, ItemDefinition->StackSizeMax);
    if (newQty != Quantity) {
        Quantity = newQty;

        // If the item is in an inventory
        auto inventory = this->GetInventoryComponent();
        if (!inventory) {
            UE_LOG(Myth, Verbose, TEXT("SetStackSize: ItemInstance %s is not in an inventory"), *GetName());
            return;
        }
        auto slot = inventory->GetItem(this->SlotIndex);
        if (!slot) {
            UE_LOG(Myth, Verbose, TEXT("SetStackSize: ItemInstance %s is not in a valid slot"), *GetName());
            return;
        }

        if (Quantity <= 0) {
            // Remove the item from the slot if the stack is 0
            // NOTE: Item->Destroy() should be called by the caller if full removal is intended
            inventory->SetItemInSlot(this->SlotIndex, nullptr);
        }

        // Update local (server or owning client for listen-server)
        inventory->NotifyItemInstanceUpdated(this->SlotIndex);
    }
}

// Sets the item definition and quantity, then instantiates fragments from the item definition.
void UMythicItemInstance::Initialize(UItemDefinition *ItemDef, const int32 quantityIfStackable, const int32 level) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can initialize an item instance"));

    this->ItemDefinition = ItemDef;
    this->randomSeed = FMath::Rand();
    this->ItemLevel = level;
    this->Quantity = ItemDef->StackSizeMax > 1 ? quantityIfStackable : 1;
    UE_LOG(Myth, Verbose, TEXT("Level %d item %s has random seed %d"), level, *GetName(), this->randomSeed);

    // Create fragments for this item from the item definition
    for (int i = 0; i < ItemDef->Fragments.Num(); i++) {
        auto FragmentSource = ItemDef->Fragments[i];
        if (!FragmentSource) {
            UE_LOG(Myth, Error, TEXT("ItemInstance %s has a null fragment at index %d"), *GetName(), i);
            continue;
        }

        AddFragment(FragmentSource);
    }
}


void UMythicItemInstance::AddFragment(TObjectPtr<UItemFragment> FragmentSource) {
    auto owner = this->GetOwningActor();
    checkf(owner->HasAuthority(), TEXT("Only the server can add fragments to an item instance"));

    UItemFragment *Fragment = NewObject<UItemFragment>(this, FragmentSource->GetClass(), NAME_None, RF_NoFlags, FragmentSource);
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
    UE_LOG(Myth, Verbose, TEXT("ItemInstance %s has random seed %d"), *GetName(), this->randomSeed);
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

void UMythicItemInstance::SetInventory(UMythicInventoryComponent *NewInventory, int32 NewSlotIndex) {
    this->OwningInventory = NewInventory;
    this->SlotIndex = NewSlotIndex;
    for (TObjectPtr ItemFragment : this->ItemFragments) {
        ItemFragment->OnInventorySlotChanged(NewInventory, NewSlotIndex);
    }
}

int32 UMythicItemInstance::GetSlot() const {
    return this->SlotIndex;
}

UMythicInventoryComponent *UMythicItemInstance::GetInventoryComponent() const {
    return this->OwningInventory;
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

void UMythicItemInstance::OnDestroyed() {
    if (IsValid(this)) {
        auto inventory = this->GetInventoryComponent();
        if (IsValid(inventory)) {
            inventory->SetItemInSlot(this->GetSlot(), nullptr);
        }

        for (auto Fragment : ItemFragments) {
            if (IsValid(Fragment)) {
                Fragment->MarkAsGarbage();
            }
        }

        ItemFragments.Empty();
    }
}

void UMythicItemInstance::OnRep_Quantity() {
    if (OwningInventory) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_ItemDefinition() {
    if (OwningInventory) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_OwningInventory() {
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::OnRep_SlotIndex() {
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}
