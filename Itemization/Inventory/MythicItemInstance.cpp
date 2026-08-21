#include "MythicItemInstance.h"

#include "MythicInventoryComponent.h"
#include "Fragments/ItemFragment.h"
#include "Itemization/Loot/MythicWorldItem.h"
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
    auto owner = this->GetOwningActor();
    checkf(owner->HasAuthority(), TEXT("Only the server can set the stack size of an item instance"));

    if (!ItemDefinition) {
        UE_LOG(Myth, Error, TEXT("SetStackSize: ItemDefinition is null on %s; cannot clamp stack"), *GetName());
        return;
    }

    const auto newQty = FMath::Min(newQuantity, ItemDefinition->StackSizeMax);
    if (newQty != Quantity) {
        Quantity = newQty;

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
            inventory->SetItemInSlot(this->SlotIndex, nullptr);
        }

        inventory->NotifyItemInstanceUpdated(this->SlotIndex);
    }
}

int32 UMythicItemInstance::ClampInitialStackQuantity(int32 Requested, int32 StackSizeMax) {
    if (StackSizeMax <= 1) {
        return 1;
    }
    return FMath::Clamp(Requested, 1, StackSizeMax);
}

void UMythicItemInstance::Initialize(UItemDefinition *ItemDef, const int32 quantityIfStackable, const int32 level) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can initialize an item instance"));

    this->ItemDefinition = ItemDef;
    this->ItemLevel = level;
    this->Quantity = ClampInitialStackQuantity(quantityIfStackable, ItemDef->StackSizeMax);
    UE_LOG(Myth, Verbose, TEXT("Initialized level %d item %s"), level, *GetName());

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
    Fragment->SetOwner(owner);

    ItemFragments.Add(Fragment);

    Fragment->OnInstanced(this);
}

void UMythicItemInstance::OnActiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnItemActivated(this);
    }
}

void UMythicItemInstance::OnInactiveItem() {
    for (int i = ItemFragments.Num() - 1; i >= 0; i--) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnItemDeactivated(this);
    }
}

void UMythicItemInstance::OnClientActiveItem() {
    for (int i = 0; i < ItemFragments.Num(); i++) {
        if (ItemFragments[i] == nullptr) { continue; }
        ItemFragments[i]->OnClientItemActivated(this);
    }
}

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

    if (HasTag(Tag)) {
        return;
    }

    ItemTags.AddTag(Tag);
}

void UMythicItemInstance::RemoveTag(const FGameplayTag &Tag) {
    checkf(this->GetOwningActor()->HasAuthority(), TEXT("Only the server can remove tags from an item instance"));

    ItemTags.RemoveTag(Tag);
}

bool UMythicItemInstance::HasTag(const FGameplayTag &Tag) const {
    return ItemTags.HasTag(Tag);
}

void UMythicItemInstance::GetTypeProbe(FGameplayTagContainer &Out) const {
    Out.Reset();
    if (ItemDefinition) {
        Out.AddTag(ItemDefinition->ItemType);
    }
    Out.AppendTags(ItemTags);
}

void UMythicItemInstance::ServerApplyTransform(const FGameplayTag &NewItemType,
                                               const FGameplayTagContainer &TagsToAdd,
                                               const FGameplayTagContainer &TagsToRemove,
                                               UItemDefinition *OptionalNewDef) {
    checkf(GetOwningActor() && GetOwningActor()->HasAuthority(), TEXT("ServerApplyTransform: authority only"));

    for (const FGameplayTag &T : TagsToRemove) {
        ItemTags.RemoveTag(T);
    }
    for (const FGameplayTag &T : TagsToAdd) {
        if (!ItemTags.HasTag(T)) {
            ItemTags.AddTag(T);
        }
    }
    if (NewItemType.IsValid() && !ItemTags.HasTag(NewItemType)) {
        ItemTags.AddTag(NewItemType);
    }
    if (OptionalNewDef) {
        ItemDefinition = OptionalNewDef;
    }

    if (OwningInventory) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

bool UMythicItemInstance::isStackableWith(const UMythicItemInstance *Other) const {
    if (ItemDefinition && ItemDefinition->StackSizeMax <= 0) {
        return false;
    }

    if (!Other) {
        return false;
    }
    if (ItemFragments.Num() != Other->ItemFragments.Num()) {
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

void UMythicItemInstance::ConsumeItem(int32 StackQty) {
    if (auto Inventory = this->GetInventoryComponent()) {
        Inventory->ServerRemoveItem(this, StackQty);
        return;
    }

    this->SetStackSize(this->GetStacks() - StackQty);
    if (this->GetStacks() <= 0) {
        auto WorldItem = Cast<AMythicWorldItem>(this->GetOwningActor());
        this->Destroy();
        if (WorldItem) {
            WorldItem->Destroy();
        }
    }
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

void UMythicItemInstance::OnRep_MarkedJunk() {
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}

void UMythicItemInstance::ServerSetMarkedJunk(bool bJunk) {
    checkf(GetOwningActor() && GetOwningActor()->HasAuthority(), TEXT("Only the server can set the junk flag on an item instance"));

    if (bMarkedJunk == bJunk) {
        return;
    }
    bMarkedJunk = bJunk;
    if (OwningInventory && SlotIndex != INDEX_NONE) {
        OwningInventory->NotifyItemInstanceUpdated(SlotIndex);
    }
}
