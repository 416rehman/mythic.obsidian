

#include "MythicInventoryComponent.h"
#include "Serialization/MemoryWriter.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"
#include "MythicItemInstance.h"
#include "Itemization/MythicDataAsset.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Mythic/Mythic.h"
#include "Mythic/Itemization/Loot/MythicLootManagerSubsystem.h"
#include "ViewModels/InventoryVM.h"
#include "ItemDefinition.h"
#include "Mythic/Player/MythicPlayerController.h"
#include "Mythic/Player/MythicCharacter.h"
#include "AbilitySystemGlobals.h"
#include "AbilitySystemComponent.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "Fragments/ActionableItemFragment.h"
#include "Itemization/Affixes/MythicAffixApplicationComponent.h"

UMythicInventoryComponent::UMythicInventoryComponent(const FObjectInitializer &OI) :
    Super(OI) {
    SetIsReplicatedByDefault(true);
    SetIsReplicated(true);
    this->bReplicateUsingRegisteredSubObjectList = true;

    Slots.SetOwningInventory(this);
}

void FMythicInventorySlotEntry::ClientUpdateActiveState(UMythicInventoryComponent* Owner) {
    bool bChanged = ClientLastKnownItem != SlottedItemInstance;
    UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Slot Item: %s, LastKnown: %s, SlotDomain: %d, Changed: %d"),
           SlottedItemInstance ? *SlottedItemInstance->GetName() : TEXT("Null"),
           ClientLastKnownItem ? *ClientLastKnownItem->GetName() : TEXT("Null"),
           static_cast<int32>(SlotDomain),
           bChanged);

    if (bChanged) {
        if (IsGearSlot() && IsValid(ClientLastKnownItem)) {
            UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Deactivating Old Item: %s"), *ClientLastKnownItem->GetName());
            ClientLastKnownItem->OnClientInactiveItem();

            if (Owner && Owner->GetOwner()) {
                if (AMythicCharacter* CharOwner = Cast<AMythicCharacter>(Owner->GetOwner())) {
                    if (SlotDefinition) {
                        CharOwner->RemoveLocalEquipmentMesh(SlotDefinition->SlotType);
                    }
                }
            }
        }

        if (IsGearSlot() && IsValid(SlottedItemInstance)) {
            UE_LOG(Myth, Log, TEXT("ClientUpdateActiveState: Activating New Item: %s"), *SlottedItemInstance->GetName());
            SlottedItemInstance->OnClientActiveItem();

            if (Owner && Owner->GetOwner()) {
                if (AMythicCharacter* CharOwner = Cast<AMythicCharacter>(Owner->GetOwner())) {
                    if (SlotDefinition) {
                        if (UItemDefinition* ItemDef = SlottedItemInstance->GetItemDefinition()) {
                            if (USkeletalMesh* EquipMesh = ItemDef->EquippedMesh.LoadSynchronous()) {
                                CharOwner->ApplyLocalEquipmentMesh(EquipMesh, SlotDefinition->SlotType);
                            }
                        }
                    }
                }
            }
        }

        ClientLastKnownItem = SlottedItemInstance;
    }
}

void FMythicInventorySlotEntry::ServerUpdateActiveState() {
    if (IsGearSlot() && IsValid(SlottedItemInstance)) {
        SlottedItemInstance->OnActiveItem();
    }
}

void FMythicInventorySlotEntry::Clear() {
    if (SlottedItemInstance) {
        this->SlottedItemInstance->SetInventory(nullptr, INDEX_NONE);
        this->SlottedItemInstance = nullptr;
    }
}

void FMythicInventoryFastArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    if (Owner) {
        UE_LOG(Myth, Log, TEXT("FastArray: PostReplicatedAdd called with %d indices"), AddedIndices.Num());
        Owner->HandleSlotsAdded(AddedIndices, FinalSize);
    }
}

void FMythicInventoryFastArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {
    if (Owner) {
        UE_LOG(Myth, Log, TEXT("FastArray: PostReplicatedChange called with %d indices"), ChangedIndices.Num());
        Owner->HandleSlotsChanged(ChangedIndices, FinalSize);
    }
}

void FMythicInventoryFastArray::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    if (Owner) {
        UE_LOG(Myth, Log, TEXT("FastArray: PreReplicatedRemove called with %d indices"), RemovedIndices.Num());
        Owner->HandleSlotsRemoved(RemovedIndices, FinalSize);
    }
}

void FMythicInventoryFastArray::AddSlot(const FMythicInventorySlotEntry &NewSlot) {
    FMythicInventorySlotEntry &AddedItem = Items.Add_GetRef(NewSlot);
    MarkItemDirty(AddedItem);

    if (Owner && Owner->GetNetMode() != NM_Client) {
        int32 AddedIndex = Items.Num() - 1;
        PostReplicatedAdd(TArrayView<int32>(&AddedIndex, 1), Items.Num());
    }
}

void FMythicInventoryFastArray::AddSlotSilent(const FMythicInventorySlotEntry &NewSlot) {
    FMythicInventorySlotEntry &AddedItem = Items.Add_GetRef(NewSlot);
    MarkItemDirty(AddedItem);
}

void FMythicInventoryFastArray::NotifyServerBatchAdded(int32 StartIndex) {
    if (!Owner || Owner->GetNetMode() == NM_Client || Items.Num() <= StartIndex) {
        return;
    }
    TArray<int32> AddedIndices;
    AddedIndices.Reserve(Items.Num() - StartIndex);
    for (int32 i = StartIndex; i < Items.Num(); ++i) {
        AddedIndices.Add(i);
    }
    PostReplicatedAdd(AddedIndices, Items.Num());
}

void FMythicInventoryFastArray::RemoveSlotAt(int32 Index) {
    if (Items.IsValidIndex(Index)) {
        if (Owner && Owner->GetNetMode() != NM_Client) {
            int32 RemovedIndex = Index;
            PreReplicatedRemove(TArrayView<int32>(&RemovedIndex, 1), Items.Num() - 1);
        }

        Items.RemoveAt(Index);

        for (int32 i = Index; i < Items.Num(); ++i) {
            if (IsValid(Items[i].SlottedItemInstance)) {
                Items[i].SlottedItemInstance->SetInventory(Owner, i);
            }
            MarkItemDirty(Items[i]);
        }
        MarkArrayDirty();
    }
}

void FMythicInventoryFastArray::ModifySlotAtIndex(int32 Index, const TFunction<void(FMythicInventorySlotEntry &SlotData)> &Modifier) {
    if (Items.IsValidIndex(Index)) {
        Modifier(Items[Index]);
        MarkItemDirty(Items[Index]);

        if (Owner && Owner->GetNetMode() != NM_Client) {
            int32 ChangedIndex = Index;
            PostReplicatedChange(TArrayView<int32>(&ChangedIndex, 1), Items.Num());
        }
    }
}

void UMythicInventoryComponent::SetupLocalViewModel() {
    if (GetWorld() && GetWorld()->GetNetMode() == NM_DedicatedServer) {
        return;
    }

    const bool bWasNull = (ViewModel == nullptr);
    if (!IsValid(ViewModel)) {
        ViewModel = NewObject<UInventoryVM>(this);
    }
    if (IsValid(ViewModel)) {
        ViewModel->InitializeFromInventoryComponent(this);
        if (bWasNull) {
            OnViewModelCreated.Broadcast();
        }
    }
}

void UMythicInventoryComponent::BeginPlay() {
    Super::BeginPlay();

    Slots.Owner = this;

    if (GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Verbose, TEXT("Inventory Component BeginPlay: Has Authority"));
        InitializeSlots();
    }

    SetupLocalViewModel();
}

void UMythicInventoryComponent::OnRep_Slots() {
    Slots.Owner = this;
}

void UMythicInventoryComponent::InitializeSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("InitializeSlots:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("InitializeSlots:: Called without Authority!"));

    const int32 OldSlotsSize = Slots.Num();

    DestroyAllSlots();

    if (!InventoryProfile) {
        UE_LOG(Myth, Warning, TEXT("[InitializeSlots] No profile assigned to inventory component on %s"), *lOwner->GetName());
        return;
    }

    for (const auto &GroupPair : InventoryProfile->SlotGroups) {
        const FGameplayTag &GroupTag = GroupPair.Key;
        const FInventorySlotGroup &Group = GroupPair.Value;

        int32 EntryIndex = 0;
        for (const FInventoryProfileEntry &Entry : Group.Slots) {
            if (!Entry.SlotDefinition) {
                UE_LOG(Myth, Error, TEXT("InitializeSlots: SlotDefinition is null in group %s for owner %s"),
                       *GroupTag.ToString(), *lOwner->GetName());
                ++EntryIndex;
                continue;
            }

            for (int32 i = 0; i < Entry.Count; ++i) {
                FMythicInventorySlotEntry SlotEntry;
                SlotEntry.SlotDefinition = Entry.SlotDefinition;
                SlotEntry.SlotDomain = Group.SlotDomain;
                SlotEntry.GroupTag = GroupTag;
                SlotEntry.EntryIndex = EntryIndex;
                SlotEntry.bRequireUniqueInEntry = Entry.bRequireUniqueItems;
                SlotEntry.bCanPlayerTake = Group.bCanPlayerTake;
                SlotEntry.bCanPlayerPut = Group.bCanPlayerPut;

                Slots.AddSlotSilent(SlotEntry);
            }
            ++EntryIndex;
        }
    }
    // DestroyAllSlots emptied the array, so every current entry is new.
    Slots.NotifyServerBatchAdded(0);

    const int32 NewSlotsSize = Slots.Num();
    UE_LOG(Myth, Verbose, TEXT("Initialized Inventory from %d to %d slots"), OldSlotsSize, NewSlotsSize);

    if (OldSlotsSize != NewSlotsSize) {
        OnInventorySizeChanged.Broadcast(NewSlotsSize, OldSlotsSize);
    }
    else {
        if (IsValid(ViewModel)) {
            ViewModel->RefreshAllItemsFromInventory(this);
        }
    }
}

bool UMythicInventoryComponent::CanAcceptItemType(const FGameplayTag &ItemType) const {
    for (const auto &Slot : Slots.Items) {
        if (Slot.SlotDefinition) {
            if (Slot.SlotDefinition->WhitelistedItemTypes.Num() == 0 || ItemType.MatchesAny(Slot.SlotDefinition->WhitelistedItemTypes)) {
                return true;
            }
        }
    }
    return false;
}

bool UMythicInventoryComponent::CanSlotAcceptItem(int32 SlotIndex, UMythicItemInstance *ItemInstance, bool bFromPlayer) const {
    if (!Slots.IsValidIndex(SlotIndex) || !ItemInstance) {
        return false;
    }
    const FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    if (bFromPlayer && !Slot.bCanPlayerPut) {
        return false;
    }

    if (!SlotWhitelistAccepts(SlotIndex, ItemInstance)) {
        return false;
    }

    if (bFromPlayer && Slot.IsGearSlot()) {
        if (const UItemDefinition *Def = ItemInstance->GetItemDefinition()) {
            if (Def->RequiredEquipTag.IsValid()) {
                FGameplayTagContainer OwnerTags;
                if (UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(GetOwner())) {
                    ASC->GetOwnedGameplayTags(OwnerTags);
                }
                if (!MeetsEquipRequirement(Def->RequiredEquipTag, OwnerTags)) {
                    return false;
                }
            }
        }
    }

    if (Slot.bRequireUniqueInEntry && ItemInstance->GetItemDefinition()) {
        for (int32 i = 0; i < Slots.Num(); ++i) {
            if (i == SlotIndex) {
                continue;
            }
            const FMythicInventorySlotEntry &OtherSlot = Slots.Items[i];
            if (OtherSlot.GroupTag == Slot.GroupTag && OtherSlot.EntryIndex == Slot.EntryIndex && OtherSlot.SlottedItemInstance && OtherSlot.SlottedItemInstance
                ->GetItemDefinition() == ItemInstance->GetItemDefinition()) {
                return false;
            }
        }
    }

    return true;
}

bool UMythicInventoryComponent::MeetsEquipRequirement(const FGameplayTag &RequiredTag, const FGameplayTagContainer &OwnerTags) {
    return !RequiredTag.IsValid() || OwnerTags.HasTag(RequiredTag);
}

bool UMythicInventoryComponent::SlotWhitelistAccepts(int32 SlotIndex, const UMythicItemInstance *Inst) const {
    if (!Slots.IsValidIndex(SlotIndex) || !Inst) {
        return false;
    }
    const FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    if (!Slot.SlotDefinition || Slot.SlotDefinition->WhitelistedItemTypes.Num() == 0) {
        return true;
    }

    FGameplayTagContainer Probe;
    Inst->GetTypeProbe(Probe);
    return Probe.HasAny(Slot.SlotDefinition->WhitelistedItemTypes);
}

UMythicItemInstance *UMythicInventoryComponent::ReleaseFromSlot(int32 SlotIndex) {
    checkf(GetOwner() && GetOwner()->HasAuthority(), TEXT("ReleaseFromSlot is server-only."));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return nullptr;
    }

    UMythicItemInstance *Inst = Slots.Items[SlotIndex].SlottedItemInstance;
    if (!Inst) {
        return nullptr;
    }

    if (!SetItemInSlot(SlotIndex, nullptr)) {
        return nullptr;
    }
    return Inst;
}

UMythicItemInstance *UMythicInventoryComponent::GetItem(int32 SlotIndex) {
    return Slots.GetItemInSlot(SlotIndex);
}

bool UMythicInventoryComponent::TryTransferToSlot(UMythicItemInstance *ItemInstance, int32 TargetSlotIndex) {
    if (!ItemInstance || !Slots.IsValidIndex(TargetSlotIndex)) {
        return false;
    }
    UMythicInventoryComponent *OldInventory = ItemInstance->GetInventoryComponent();
    const int32 OldItemSlot = ItemInstance->GetSlot();
    if (OldInventory == this && OldItemSlot == TargetSlotIndex
        && Slots.Items[TargetSlotIndex].SlottedItemInstance == ItemInstance) {
        return true;
    }
    if (Slots.Items[TargetSlotIndex].SlottedItemInstance) {
        return false;
    }

    TArray<FStagedSlotMutation> Mutations;
    if (OldInventory) {
        if (!OldInventory->Slots.IsValidIndex(OldItemSlot)
            || OldInventory->Slots.Items[OldItemSlot].SlottedItemInstance != ItemInstance) {
            return false;
        }
        Mutations.Add({OldInventory, OldItemSlot, ItemInstance, nullptr});
    }
    Mutations.Add({this, TargetSlotIndex, nullptr, ItemInstance});
    return CommitSlotMutationsTransactional(Mutations);
}

bool UMythicInventoryComponent::SetItemInSlot(int32 SlotIndex, UMythicItemInstance *NewItemInstance) {
    checkf(GetOwner()->HasAuthority(), TEXT("This function is server-only."));

    return SetItemInSlotInternal(SlotIndex, NewItemInstance);
}

bool UMythicInventoryComponent::SetItemInSlotInternal(int32 SlotIndex, UMythicItemInstance *NewItemInstance) {
    if (!Slots.IsValidIndex(SlotIndex)) {
        return false;
    }

    UMythicItemInstance *CurrentItem = Slots.Items[SlotIndex].SlottedItemInstance;
    if (CurrentItem == NewItemInstance) {
        return true;
    }
    if (CurrentItem && NewItemInstance) {
        UE_LOG(Myth, Warning, TEXT("There is already an item in this slot"));
        return false;
    }
    const FStagedSlotMutation Mutation{this, SlotIndex, CurrentItem, NewItemInstance};
    return CommitSlotMutationsTransactional(MakeArrayView(&Mutation, 1));
}

bool UMythicInventoryComponent::ValidateFinalSlotLayout(
    const TConstArrayView<FStagedSlotMutation> Mutations) {
    if (Mutations.IsEmpty()) {
        return true;
    }

    TSet<UMythicInventoryComponent *> AffectedInventories;
    for (int32 Index = 0; Index < Mutations.Num(); ++Index) {
        const FStagedSlotMutation &Mutation = Mutations[Index];
        if (!Mutation.Inventory || !Mutation.Inventory->GetOwner()
            || !Mutation.Inventory->GetOwner()->HasAuthority()
            || !Mutation.Inventory->Slots.IsValidIndex(Mutation.SlotIndex)
            || Mutation.Inventory->Slots.Items[Mutation.SlotIndex].SlottedItemInstance != Mutation.ExpectedItem) {
            return false;
        }
        for (int32 OtherIndex = Index + 1; OtherIndex < Mutations.Num(); ++OtherIndex) {
            if (Mutations[OtherIndex].Inventory == Mutation.Inventory
                && Mutations[OtherIndex].SlotIndex == Mutation.SlotIndex) {
                return false;
            }
        }
        if (Mutation.ProposedItem
            && !Mutation.Inventory->SlotWhitelistAccepts(Mutation.SlotIndex, Mutation.ProposedItem)) {
            return false;
        }
        AffectedInventories.Add(Mutation.Inventory);
    }

    auto FindProposedItem = [&Mutations](UMythicInventoryComponent *Inventory, const int32 SlotIndex,
                                         UMythicItemInstance *Current) {
        for (const FStagedSlotMutation &Mutation : Mutations) {
            if (Mutation.Inventory == Inventory && Mutation.SlotIndex == SlotIndex) {
                return Mutation.ProposedItem;
            }
        }
        return Current;
    };

    // A currently owned item may only enter a new destination if its source slot is part of this same transaction.
    for (const FStagedSlotMutation &Mutation : Mutations) {
        UMythicItemInstance *Proposed = Mutation.ProposedItem;
        UMythicInventoryComponent *CurrentInventory = Proposed ? Proposed->GetInventoryComponent() : nullptr;
        if (!CurrentInventory) {
            continue;
        }
        const int32 CurrentSlot = Proposed->GetSlot();
        bool bCurrentSlotParticipates = false;
        for (const FStagedSlotMutation &SourceMutation : Mutations) {
            bCurrentSlotParticipates |= SourceMutation.Inventory == CurrentInventory
                && SourceMutation.SlotIndex == CurrentSlot && SourceMutation.ExpectedItem == Proposed;
        }
        if (!bCurrentSlotParticipates) {
            return false;
        }
    }

    TSet<UMythicItemInstance *> FinalItems;
    for (UMythicInventoryComponent *Inventory : AffectedInventories) {
        for (int32 SlotIndex = 0; SlotIndex < Inventory->Slots.Num(); ++SlotIndex) {
            const FMythicInventorySlotEntry &Slot = Inventory->Slots.Items[SlotIndex];
            UMythicItemInstance *FinalItem = FindProposedItem(Inventory, SlotIndex, Slot.SlottedItemInstance);
            if (!FinalItem) {
                continue;
            }
            if (FinalItems.Contains(FinalItem)) {
                return false;
            }
            FinalItems.Add(FinalItem);

            if (!Slot.bRequireUniqueInEntry || !FinalItem->GetItemDefinition()) {
                continue;
            }
            for (int32 OtherSlotIndex = SlotIndex + 1; OtherSlotIndex < Inventory->Slots.Num(); ++OtherSlotIndex) {
                const FMythicInventorySlotEntry &OtherSlot = Inventory->Slots.Items[OtherSlotIndex];
                UMythicItemInstance *OtherFinal = FindProposedItem(
                    Inventory, OtherSlotIndex, OtherSlot.SlottedItemInstance);
                if (OtherSlot.GroupTag == Slot.GroupTag && OtherSlot.EntryIndex == Slot.EntryIndex
                    && OtherFinal && OtherFinal->GetItemDefinition() == FinalItem->GetItemDefinition()) {
                    return false;
                }
            }
        }
    }
    return true;
}

UMythicAffixApplicationComponent *UMythicInventoryComponent::ResolveAffixApplicationComponent() const {
    auto FindApplication = [](AActor *Actor) -> UMythicAffixApplicationComponent * {
        return Actor ? Actor->FindComponentByClass<UMythicAffixApplicationComponent>() : nullptr;
    };
    AActor *OwnerActor = GetOwner();
    UMythicAffixApplicationComponent *Application = FindApplication(OwnerActor);
    if (!Application) {
        if (APawn *Pawn = Cast<APawn>(OwnerActor)) {
            Application = FindApplication(Pawn->GetPlayerState());
            if (!Application) Application = FindApplication(Pawn->GetController());
        }
        else if (AController *Controller = Cast<AController>(OwnerActor)) {
            Application = FindApplication(Controller->GetPlayerState<APlayerState>());
            if (!Application && Controller->GetPawn()) {
                Application = FindApplication(Controller->GetPawn()->GetPlayerState());
            }
        }
    }
    return Application ? Application : FindApplication(OwnerActor ? OwnerActor->GetOwner() : nullptr);
}

bool UMythicInventoryComponent::CommitSlotMutationsTransactional(
    const TConstArrayView<FStagedSlotMutation> Mutations) {
    if (!ValidateFinalSlotLayout(Mutations)) {
        return false;
    }

    TArray<FMythicAffixEquipmentSlotOverride> EquipmentOverrides;
    UMythicAffixApplicationComponent *Application = nullptr;
    for (const FStagedSlotMutation &Mutation : Mutations) {
        const FMythicInventorySlotEntry &Slot = Mutation.Inventory->Slots.Items[Mutation.SlotIndex];
        if (Mutation.ExpectedItem == Mutation.ProposedItem
            || !Slot.IsGearSlot()) {
            continue;
        }
        UMythicAffixApplicationComponent *CandidateApplication =
            Mutation.Inventory->ResolveAffixApplicationComponent();
        if (!CandidateApplication || (Application && CandidateApplication != Application)) {
            return false;
        }
        Application = CandidateApplication;
        EquipmentOverrides.Add({Mutation.Inventory, Mutation.SlotIndex, Mutation.ProposedItem});
    }

    // This is the only fallible mutation. Until it succeeds, slot pointers, item back-pointers, lifecycle state and
    // Fast Array dirtiness are all exactly the old transaction state.
    if (Application
        && !Application->ReconcileEquipmentMutationTransactional(EquipmentOverrides)) {
        return false;
    }

    TSet<UMythicItemInstance *> PreviouslyActiveItems;
    TSet<UMythicItemInstance *> ProposedActiveItems;
    for (const FStagedSlotMutation &Mutation : Mutations) {
        const FMythicInventorySlotEntry &Slot = Mutation.Inventory->Slots.Items[Mutation.SlotIndex];
        if (Mutation.ExpectedItem && Slot.IsGearSlot()) {
            PreviouslyActiveItems.Add(Mutation.ExpectedItem);
        }
        if (Mutation.ProposedItem && Slot.IsGearSlot()) {
            ProposedActiveItems.Add(Mutation.ProposedItem);
        }
    }
    for (UMythicItemInstance *PreviouslyActive : PreviouslyActiveItems) {
        if (PreviouslyActive && !ProposedActiveItems.Contains(PreviouslyActive)) {
            PreviouslyActive->OnInactiveItem();
        }
    }

    TSet<UMythicItemInstance *> DetachedItems;
    for (const FStagedSlotMutation &Mutation : Mutations) {
        if (Mutation.ExpectedItem && Mutation.ExpectedItem != Mutation.ProposedItem
            && !DetachedItems.Contains(Mutation.ExpectedItem)) {
            Mutation.ExpectedItem->SetInventory(nullptr, INDEX_NONE);
            DetachedItems.Add(Mutation.ExpectedItem);
        }
    }

    // Install every pointer before invoking inventory-change callbacks, so swaps/transfers never expose a partial
    // final layout to fragments. Replication is marked dirty only after all lifecycle callbacks complete.
    for (const FStagedSlotMutation &Mutation : Mutations) {
        Mutation.Inventory->Slots.Items[Mutation.SlotIndex].SlottedItemInstance = Mutation.ProposedItem;
    }
    for (const FStagedSlotMutation &Mutation : Mutations) {
        if (Mutation.ProposedItem) {
            Mutation.ProposedItem->SetOwner(Mutation.Inventory);
            Mutation.ProposedItem->SetInventory(Mutation.Inventory, Mutation.SlotIndex);
        }
    }

    TArray<TPair<UMythicInventoryComponent *, UMythicItemInstance *>> NewlyEquippedItems;
    for (const FStagedSlotMutation &Mutation : Mutations) {
        const FMythicInventorySlotEntry &Slot = Mutation.Inventory->Slots.Items[Mutation.SlotIndex];
        if (Mutation.ProposedItem
            && ProposedActiveItems.Contains(Mutation.ProposedItem)
            && !PreviouslyActiveItems.Contains(Mutation.ProposedItem)
            && !NewlyEquippedItems.ContainsByPredicate(
                [&Mutation](const TPair<UMythicInventoryComponent *, UMythicItemInstance *> &Pair) {
                    return Pair.Value == Mutation.ProposedItem;
                })) {
            Mutation.ProposedItem->OnActiveItem();
            NewlyEquippedItems.Emplace(Mutation.Inventory, Mutation.ProposedItem);
        }
    }

    // Equipment telemetry is a slot-transaction concern, not an affix/attack-fragment concern. Emit once only
    // after every lifecycle callback observes the complete committed layout.
    for (const TPair<UMythicInventoryComponent *, UMythicItemInstance *> &Equipped : NewlyEquippedItems) {
        AActor *InventoryOwner = Equipped.Key ? Equipped.Key->GetOwner() : nullptr;
        AMythicPlayerController *PlayerController = Cast<AMythicPlayerController>(InventoryOwner);
        if (!PlayerController) {
            if (const APawn *Pawn = Cast<APawn>(InventoryOwner)) {
                PlayerController = Cast<AMythicPlayerController>(Pawn->GetController());
            }
        }
        if (PlayerController && Equipped.Value && Equipped.Value->GetItemDefinition()) {
            PlayerController->NotifyItemEquipped(Equipped.Value->GetItemDefinition());
        }
    }

    TMap<UMythicInventoryComponent *, TArray<int32>> ChangedSlotsByInventory;
    for (const FStagedSlotMutation &Mutation : Mutations) {
        if (Mutation.ExpectedItem == Mutation.ProposedItem) {
            continue;
        }
        Mutation.Inventory->Slots.MarkItemDirty(Mutation.Inventory->Slots.Items[Mutation.SlotIndex]);
        ChangedSlotsByInventory.FindOrAdd(Mutation.Inventory).Add(Mutation.SlotIndex);
    }
    for (TPair<UMythicInventoryComponent *, TArray<int32>> &Pair : ChangedSlotsByInventory) {
        Pair.Key->HandleSlotsChanged(Pair.Value, Pair.Key->Slots.Num());
    }
    return true;
}

bool UMythicInventoryComponent::ReconcileEquippedAffixSnapshotMutationTransactional(
    UMythicItemInstance *ItemInstance,
    const TConstArrayView<FRolledAffix> ProposedSnapshots) const {
    if (!ItemInstance || !GetOwner() || !GetOwner()->HasAuthority()
        || ItemInstance->GetInventoryComponent() != this || !Slots.IsValidIndex(ItemInstance->GetSlot())) {
        return false;
    }
    const FMythicInventorySlotEntry &Slot = Slots.Items[ItemInstance->GetSlot()];
    if (!Slot.IsGearSlot() || Slot.SlottedItemInstance != ItemInstance) {
        return false;
    }
    UMythicAffixApplicationComponent *Application = ResolveAffixApplicationComponent();
    return Application
        && Application->ReconcileItemSnapshotMutationTransactional(ItemInstance, ProposedSnapshots);
}

void UMythicInventoryComponent::NotifyOwnerItemAcquired(const UItemDefinition *ItemDef, int32 Quantity) {
    if (!ItemDef || Quantity <= 0) {
        return;
    }
    if (AMythicPlayerController *OwningPC = Cast<AMythicPlayerController>(GetOwner())) {
        OwningPC->ClientNotifyLootPickup(ItemDef->Name, Quantity, UItemDefinition::GetRarityColor(ItemDef->Rarity));
        OwningPC->NotifyItemAcquired(ItemDef, Quantity);
    }
}

AMythicWorldItem *UMythicInventoryComponent::AddItem(UMythicItemInstance *ItemInstance, AController *TargetRecipient) {
    auto OriginalQty = ItemInstance->GetStacks();
    auto PickupDef = ItemInstance->GetItemDefinition();
    auto AmountAdded = AddToAnySlot(ItemInstance);

    NotifyOwnerItemAcquired(PickupDef, AmountAdded);

    if (AmountAdded != OriginalQty) {
        UMythicLootManagerSubsystem *LootManager = GetOwner()->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();
        AMythicWorldItem *WorldItem = LootManager->Spawn(ItemInstance, GetOwner()->GetActorLocation(), 100, TargetRecipient);
        if (WorldItem) {
            return WorldItem;
        }
    }

    return nullptr;
}

int32 UMythicInventoryComponent::AddToAnySlot(UMythicItemInstance *ItemInstance, bool bFromPlayer) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    const int32 original_qty = ItemInstance->GetStacks();

    const UItemDefinition *StackDef = ItemInstance->GetItemDefinition();
    if (StackDef && ShouldAttemptStackMerge(StackDef->StackSizeMax)) {
        for (int32 i = 0; i < Slots.Num(); ++i) {
            auto ItemInSlot = Slots.Items[i].SlottedItemInstance;
            if (bFromPlayer && !Slots.Items[i].bCanPlayerPut) {
                continue;
            }

            if (ItemInSlot != nullptr && ItemInSlot->GetItemDefinition() == ItemInstance->GetItemDefinition() && ItemInstance->isStackableWith(ItemInSlot)) {
                const int32 availableSpace = ItemInSlot->GetItemDefinition()->StackSizeMax - ItemInSlot->GetStacks();
                const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

                ItemInSlot->SetStackSize(ItemInSlot->GetStacks() + QuantityToAdd);
                ItemInstance->SetStackSize(ItemInstance->GetStacks() - QuantityToAdd);

                if (ItemInstance->GetStacks() == 0) {
                    ItemInstance->Destroy();
                    return original_qty;
                }
            }
        }
    }

    const int32 MaxStack = (StackDef && StackDef->StackSizeMax > 0) ? StackDef->StackSizeMax : 1;
    for (int32 i = 0; i < Slots.Num(); ++i) {
        const int32 remaining = ItemInstance->GetStacks();
        if (remaining <= 0) {
            break;
        }
        if (Slots.Items[i].SlottedItemInstance != nullptr) {
            continue;
        }
        if (!CanSlotAcceptItem(i, ItemInstance, bFromPlayer)) {
            continue;
        }

        if (remaining <= MaxStack) {
            if (TryTransferToSlot(ItemInstance, i)) {
                return original_qty;
            }
            continue;
        }

        UMythicItemInstance *Split = ItemInstance->CloneForStackSplit(this, MaxStack);
        if (!Split) {
            UE_LOG(Myth, Error,
                   TEXT("AddToAnySlot: current semantic stack clone failed for %s"),
                   *GetNameSafe(ItemInstance));
            break;
        }
        if (TryTransferToSlot(Split, i)) {
            ItemInstance->SetStackSize(remaining - MaxStack);
        }
        else if (IsValid(Split)) {
            Split->MarkAsGarbage();
        }
    }

    return original_qty - ItemInstance->GetStacks();
}

int32 UMythicInventoryComponent::AddToSlot(UMythicItemInstance *ItemInstance, int32 SlotIndex, bool bFromPlayer) {
    if (!ItemInstance) {
        UE_LOG(Myth, Verbose, TEXT("AddToSlot: ItemInstance is null"));
        return 0;
    }
    const int32 OriginalQty = ItemInstance->GetStacks();
    if (Slots.IsValidIndex(SlotIndex)) {
        if (bFromPlayer && !Slots.Items[SlotIndex].bCanPlayerPut) {
            return 0;
        }

        auto SlottedItem = Slots.Items[SlotIndex].SlottedItemInstance;
        if (SlottedItem == nullptr) {
            if (!CanSlotAcceptItem(SlotIndex, ItemInstance, bFromPlayer)) {
                return 0;
            }
            if (TryTransferToSlot(ItemInstance, SlotIndex)) {
                return OriginalQty;
            }
        }
        else if (SlottedItem->GetItemDefinition() == ItemInstance->GetItemDefinition() && SlottedItem->isStackableWith(ItemInstance)) {
            const int32 availableSpace = SlottedItem->GetItemDefinition()->StackSizeMax - SlottedItem->GetStacks();
            const int32 QuantityToAdd = FMath::Min(ItemInstance->GetStacks(), availableSpace);

            SlottedItem->SetStackSize(SlottedItem->GetStacks() + QuantityToAdd);
            ItemInstance->SetStackSize(ItemInstance->GetStacks() - QuantityToAdd);

            if (ItemInstance->GetStacks() == 0) {
                ItemInstance->Destroy();
                return OriginalQty;
            }
        }
    }

    return OriginalQty - ItemInstance->GetStacks();
}

int32 UMythicInventoryComponent::ReceiveItem(TObjectPtr<UMythicItemInstance> ItemInstance, int32 TargetSlotIndex, bool bFromPlayer) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(ItemInstance != nullptr, TEXT("AddToAnySlot:: Invalid ItemInstance!"));

    if (TargetSlotIndex == INDEX_NONE) {
        return AddToAnySlot(ItemInstance, bFromPlayer);
    }

    return AddToSlot(ItemInstance, TargetSlotIndex, bFromPlayer);
}

int32 UMythicInventoryComponent::SendItem(int32 SlotIndex, UMythicInventoryComponent *TargetInventory, int32 TargetSlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(TargetInventory != nullptr, TEXT("SendItem:: Invalid TargetInventory!"));

    if (this == TargetInventory && SlotIndex == TargetSlotIndex) {
        return 0;
    }

    auto itemInstance = Slots.GetItemInSlot(SlotIndex);
    if (!itemInstance) {
        return 0;
    }

    int32 amountSent = TargetInventory->ReceiveItem(itemInstance, TargetSlotIndex, true);

    if (!IsValid(itemInstance) || itemInstance->GetStacks() == 0) {
        SetItemInSlot(SlotIndex, nullptr);
    }

    return amountSent;
}

bool UMythicInventoryComponent::CanPlayerTakeFromSlot(int32 SlotIndex) const {
    return Slots.IsValidIndex(SlotIndex) && Slots.Items[SlotIndex].bCanPlayerTake;
}

bool UMythicInventoryComponent::DropItem(int32 SlotIndex, const FVector &location, const float radius, AController *TargetRecipient) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("DropItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("DropItem:: Called without Authority!"));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return false;
    }

    const FMythicInventorySlotEntry &Slot = Slots.Items[SlotIndex];

    if (!Slot.bCanPlayerTake) {
        UE_LOG(Myth, Warning, TEXT("DropItem: Cannot drop item from protected group"));
        return false;
    }

    auto item_instance = Slot.SlottedItemInstance;
    if (item_instance == nullptr) {
        UE_LOG(Myth, Verbose, TEXT("DropItem:: No item in slot %d"), SlotIndex);
        return false;
    }

    UMythicLootManagerSubsystem *loot_manager = lOwner->GetGameInstance()->GetSubsystem<UMythicLootManagerSubsystem>();

    AMythicWorldItem *world_item = loot_manager->Spawn(item_instance, location, radius, TargetRecipient);
    if (world_item == nullptr) {
        return false;
    }

    if (world_item->ItemInstance != item_instance) {
        UE_LOG(Myth, Error, TEXT("DropItem:: World item failed to take ownership of item instance"));
        world_item->Destroy();
        return false;
    }

    if (!SetItemInSlot(SlotIndex, nullptr)) {
        // Spawn may have temporarily adopted the instance. The staged unequip failed before changing the slot, so
        // detach the world representation and restore the item's old inventory ownership as the rollback state.
        world_item->ItemInstance = nullptr;
        item_instance->SetOwner(this);
        item_instance->SetInventory(this, SlotIndex);
        world_item->Destroy();
        return false;
    }

    this->OnItemDropped.Broadcast(SlotIndex, world_item);

    return true;
}

void UMythicInventoryComponent::PickupItem_Implementation(AMythicWorldItem *world_item) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("GetItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("AddToAnySlot:: Called without Authority!"));
    checkf(world_item != nullptr, TEXT("PickupItem:: Invalid WorldItem!"));

    auto Recipient = world_item->GetTargetRecipient();
    if (Recipient && Recipient != lOwner) {
        UE_LOG(Myth, Verbose, TEXT("PickupItem:: Not the TargetRecipient!"));
        return;
    }

    UMythicItemInstance *item_instance = world_item->ItemInstance;
    if (item_instance == nullptr) {
        return;
    }

    const int32 OriginalQty = item_instance->GetStacks();
    UItemDefinition *PickupDef = item_instance->GetItemDefinition();
    const int32 AmountAdded = AddToAnySlot(item_instance);

    if (AmountAdded >= OriginalQty) {
        // AddToAnySlot either merged/destroyed the source or transferred this exact physical identity. The dying
        // world representation must not keep publishing a pointer now owned by an inventory.
        world_item->ItemInstance = nullptr;
        world_item->Destroy();
    }
    else if (AmountAdded > 0) {
        // Partial merge/split already reduced the original world-owned stack in place.
        world_item->FlushNetDormancy();
    }

    NotifyOwnerItemAcquired(PickupDef, AmountAdded);
}

void UMythicInventoryComponent::HandleSlotsAdded(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    const int32 NumAdded = AddedIndices.Num();
    const int32 OldSize = FinalSize - NumAdded;

    SetupLocalViewModel();

    if (NumAdded > 0) {
        OnInventorySizeChanged.Broadcast(FinalSize, OldSize);
    }

    for (int32 idx : AddedIndices) {
        if (Slots.IsValidIndex(idx)) {
            Slots.Items[idx].ClientUpdateActiveState(this);
        }

        OnSlotUpdated.Broadcast(idx);
        if (IsValid(ViewModel)) {
            ViewModel->RefreshSlotFromInventory(this, idx);
        }
    }
}

void UMythicInventoryComponent::HandleSlotsChanged(const TArrayView<int32> &ChangedIndices, int32) {
    for (int32 idx : ChangedIndices) {
        if (Slots.IsValidIndex(idx)) {
            Slots.Items[idx].ClientUpdateActiveState(this);
        }

        OnSlotUpdated.Broadcast(idx);
        if (IsValid(ViewModel)) {
            ViewModel->RefreshSlotFromInventory(this, idx);
        }
    }
}

void UMythicInventoryComponent::HandleSlotsRemoved(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    for (int32 idx : RemovedIndices) {
        if (Slots.IsValidIndex(idx)) {
            FMythicInventorySlotEntry &Slot = Slots.Items[idx];
            if (IsValid(Slot.ClientLastKnownItem)) {
                Slot.ClientLastKnownItem->OnClientInactiveItem();

                if (Slot.IsGearSlot() && GetOwner()) {
                    if (AMythicCharacter* CharOwner = Cast<AMythicCharacter>(GetOwner())) {
                        if (Slot.SlotDefinition) {
                            CharOwner->RemoveLocalEquipmentMesh(Slot.SlotDefinition->SlotType);
                        }
                    }
                }

                Slot.ClientLastKnownItem = nullptr;
            }
        }
    }

    const int32 NumRemoved = RemovedIndices.Num();
    const int32 OldSize = FinalSize + NumRemoved;

    SetupLocalViewModel();

    if (NumRemoved > 0) {
        OnInventorySizeChanged.Broadcast(FinalSize, OldSize);
    }
}

UInventoryVM *UMythicInventoryComponent::GetViewModel() const {
    return ViewModel;
}

int32 UMythicInventoryComponent::GetItemCount(UItemDefinition *RequiredItem) const {
    if (!RequiredItem) {
        return 0;
    }

    int32 count = 0;
    for (const FMythicInventorySlotEntry &Slot : Slots.Items) {
        if (Slot.SlottedItemInstance && Slot.SlottedItemInstance->GetItemDefinition() == RequiredItem) {
            count += Slot.SlottedItemInstance->GetStacks();
        }
    }

    return count;
}

void UMythicInventoryComponent::ServerRemoveItem_Implementation(UMythicItemInstance *ItemInstance, int32 Amount) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("RemoveItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("RemoveItem:: Called without Authority!"));
    checkf(Amount > 0, TEXT("RemoveItem:: Invalid Amount!"));
    checkf(ItemInstance != nullptr, TEXT("RemoveItem:: Invalid ItemInstance!"));

    if (ItemInstance->GetInventoryComponent() != this) {
        UE_LOG(Myth, Error, TEXT("RemoveItem:: ItemInstance is not from this inventory!"));
        return;
    }

    int32 count = 0;

    if (ItemInstance->GetStacks() <= Amount) {
        count += ItemInstance->GetStacks();
        ItemInstance->Destroy();
    }
    else {
        ItemInstance->SetStackSize(ItemInstance->GetStacks() - Amount);
        count += Amount;
    }

    UE_LOG(Myth, Verbose, TEXT("Removed %d items from inventory"), count);
}

void UMythicInventoryComponent::ServerRemoveItemByDefinition_Implementation(UItemDefinition *ItemDef, int32 Amount) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("RemoveItem:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("RemoveItem:: Called without Authority!"));
    checkf(ItemDef != nullptr, TEXT("RemoveItem:: Invalid ItemDef!"));
    checkf(Amount > 0, TEXT("RemoveItem:: Invalid Amount!"));

    int32 RemovedSoFar = 0;
    for (int32 i = 0; i < Slots.Items.Num(); ++i) {
        FMythicInventorySlotEntry &Slot = Slots.Items[i];
        UMythicItemInstance *item = Slot.SlottedItemInstance;
        if (!item) {
            continue;
        }

        if (item->GetItemDefinition() == ItemDef) {
            const int32 ItemStacks = item->GetStacks();
            const int32 RemainingToRemove = Amount - RemovedSoFar;

            if (ItemStacks <= RemainingToRemove) {
                RemovedSoFar += ItemStacks;
                item->Destroy();
            }
            else {
                Slot.SlottedItemInstance->SetStackSize(ItemStacks - RemainingToRemove);
                RemovedSoFar += RemainingToRemove;
            }

            if (RemovedSoFar >= Amount) {
                break;
            }
        }
    }

    UE_LOG(Myth, Verbose, TEXT("Removed %d items from inventory"), RemovedSoFar);
}

bool UMythicInventoryComponent::DestroySlot(int32 SlotIndex) {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("Called without Authority!"));

    if (!Slots.IsValidIndex(SlotIndex)) {
        return false;
    }

    UMythicItemInstance *Item = Slots.Items[SlotIndex].SlottedItemInstance;
    if (Item && !SetItemInSlotInternal(SlotIndex, nullptr)) {
        return false;
    }
    if (Item) {
        Item->Destroy();
    }

    Slots.RemoveSlotAt(SlotIndex);

    return true;
}

void UMythicInventoryComponent::DestroyAllSlots() {
    AActor *lOwner = GetOwner();
    checkf(lOwner != nullptr, TEXT("DestroySlot:: Invalid Inventory Owner"));
    checkf(lOwner->HasAuthority(), TEXT("DestroySlot:: Called without Authority!"));

    for (int32 i = Slots.Num() - 1; i >= 0; --i) {
        if (!DestroySlot(i)) {
            UE_LOG(Myth, Error,
                   TEXT("DestroyAllSlots stopped at slot %d because its transactional unequip failed."), i);
            return;
        }
    }

    Slots.Items.Empty();
}

void UMythicInventoryComponent::OnUnregister() {
    const AActor *lOwner = GetOwner();
    if (lOwner && lOwner->HasAuthority()) {
        DestroyAllSlots();
    }

    Super::OnUnregister();
}

bool UMythicInventoryComponent::GetSlotEntry(int32 Index, FMythicInventorySlotEntry &OutEntry) const {
    if (Slots.IsValidIndex(Index)) {
        OutEntry = Slots.Items[Index];
        return true;
    }
    return false;
}

float UMythicInventoryComponent::ComputeSlotWeight(float UnitWeight, int32 StackCount) {
    return FMath::Max(0.0f, UnitWeight) * static_cast<float>(FMath::Max(0, StackCount));
}

float UMythicInventoryComponent::GetTotalCarriedWeight() const {
    float Total = 0.0f;
    for (const FMythicInventorySlotEntry &Entry : Slots.GetItems()) {
        const UMythicItemInstance *Inst = Entry.SlottedItemInstance;
        if (Inst && Inst->GetItemDefinition()) {
            Total += ComputeSlotWeight(Inst->GetItemDefinition()->Weight, Inst->GetStacks());
        }
    }
    return Total;
}

int32 UMythicInventoryComponent::GetTotalCurrency() const {
    int32 Total = 0;
    for (const FMythicInventorySlotEntry &Entry : Slots.GetItems()) {
        const UMythicItemInstance *Inst = Entry.SlottedItemInstance;
        if (Inst && Inst->GetItemDefinition() && Inst->GetItemDefinition()->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            Total += FMath::Max(0, Inst->GetStacks());
        }
    }
    return Total;
}

int32 UMythicInventoryComponent::SpendCurrency(int32 Amount) {
    const AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority() || Amount <= 0) {
        return 0;
    }

    int32 SpentSoFar = 0;
    for (int32 i = 0; i < Slots.Items.Num() && SpentSoFar < Amount; ++i) {
        UMythicItemInstance *Item = Slots.Items[i].SlottedItemInstance;
        if (!Item) {
            continue;
        }
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def || !Def->ItemType.MatchesTag(ITEMIZATION_TYPE_CURRENCY)) {
            continue;
        }
        const int32 Stacks = Item->GetStacks();
        if (Stacks <= 0) {
            continue;
        }
        const int32 Take = FMath::Min(Stacks, Amount - SpentSoFar);
        ServerRemoveItem(Item, Take);
        SpentSoFar += Take;
    }
    return SpentSoFar;
}

void UMythicInventoryComponent::NotifyItemInstanceUpdated(const int32 SlotIndex,
                                                           const bool bReconcileAffixes) {
    if (IsValid(ViewModel)) {
        ViewModel->RefreshSlotFromInventory(this, SlotIndex);
    }
    OnSlotUpdated.Broadcast(SlotIndex);

    // Equipment, restore, socket, crafting and durability mutations all converge here after their authoritative
    // item state is committed. Reconcile the complete equipped snapshot set so cross-item stacking/conflicts cannot
    // depend on fragment callback order and reconnect/load never relies on a stale incremental ledger.
    if (bReconcileAffixes && GetOwner() && GetOwner()->HasAuthority()
        && Slots.Items.IsValidIndex(SlotIndex)
        && Slots.Items[SlotIndex].IsGearSlot()) {
        UMythicAffixApplicationComponent *Application = ResolveAffixApplicationComponent();
        if (Application) Application->RequestAuthoritativeReconciliation();
    }
}

void UMythicInventoryComponent::AddSlot(UInventorySlotDefinition *SlotDefinition, int32 Count) {
    if (!SlotDefinition || Count <= 0) {
        return;
    }

    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Warning, TEXT("AddSlot called without authority!"));
        return;
    }

    for (int32 i = 0; i < Count; ++i) {
        FMythicInventorySlotEntry NewSlot;
        NewSlot.SlotDefinition = SlotDefinition;
        NewSlot.SlotDomain = EMythicInventorySlotDomain::Carried;

        Slots.AddSlot(NewSlot);
    }

    UE_LOG(Myth, Verbose, TEXT("Added %d slots of type %s"), Count, *SlotDefinition->GetName());
}

bool UMythicInventoryComponent::RemoveSlot(UInventorySlotDefinition *SlotDefinition, int32 Count, bool bDropItems) {
    if (!SlotDefinition || Count <= 0) {
        return false;
    }

    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Warning, TEXT("RemoveSlot called without authority!"));
        return false;
    }

    int32 RemovedCount = 0;

    for (int32 i = Slots.Items.Num() - 1; i >= 0; --i) {
        if (RemovedCount >= Count) {
            break;
        }

        if (Slots.Items[i].SlotDefinition == SlotDefinition && Slots.Items[i].SlottedItemInstance == nullptr) {
            Slots.RemoveSlotAt(i);
            RemovedCount++;
        }
    }

    for (int32 i = Slots.Items.Num() - 1; i >= 0; --i) {
        if (RemovedCount >= Count) {
            break;
        }

        if (Slots.Items[i].SlotDefinition == SlotDefinition) {
            UMythicItemInstance *Item = Slots.Items[i].SlottedItemInstance;
            if (Item) {
                if (bDropItems) {
                    if (!DropItem(i, GetOwner()->GetActorLocation())) {
                        continue;
                    }
                }
                else {
                    if (!SetItemInSlotInternal(i, nullptr)) {
                        continue;
                    }
                    Item->Destroy();
                }
            }

            Slots.RemoveSlotAt(i);
            RemovedCount++;
        }
    }

    return RemovedCount > 0;
}

void UMythicInventoryComponent::ServerSplitStack_Implementation(int32 SourceSlotIndex, int32 SplitAmount) {
    SplitStackToFreeSlot(SourceSlotIndex, SplitAmount);
}

int32 UMythicInventoryComponent::SplitStackToFreeSlot(int32 SourceSlotIndex, int32 SplitAmount) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return INDEX_NONE;
    }

    if (!Slots.IsValidIndex(SourceSlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: invalid source slot %d"), SourceSlotIndex);
        return INDEX_NONE;
    }

    const FMythicInventorySlotEntry &SourceSlot = Slots.Items[SourceSlotIndex];

    if (SourceSlot.IsGearSlot()) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: cannot split from equipment slot %d"), SourceSlotIndex);
        return INDEX_NONE;
    }

    UMythicItemInstance *SourceItem = SourceSlot.SlottedItemInstance;
    if (!SourceItem) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: source slot %d is empty"), SourceSlotIndex);
        return INDEX_NONE;
    }

    if (SplitAmount <= 0 || SplitAmount >= SourceItem->GetStacks()) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: invalid split amount %d for stack of %d"), SplitAmount, SourceItem->GetStacks());
        return INDEX_NONE;
    }

    int32 TargetSlotIndex = INDEX_NONE;
    for (int32 i = 0; i < Slots.Num(); ++i) {
        if (i == SourceSlotIndex) {
            continue;
        }
        const FMythicInventorySlotEntry &CandidateSlot = Slots.Items[i];
        if (CandidateSlot.GroupTag == SourceSlot.GroupTag
            && !CandidateSlot.IsGearSlot()
            && !CandidateSlot.SlottedItemInstance) {
            if (SlotWhitelistAccepts(i, SourceItem)) {
                TargetSlotIndex = i;
                break;
            }
        }
    }

    if (TargetSlotIndex == INDEX_NONE) {
        UE_LOG(Myth, Warning, TEXT("ServerSplitStack: no empty slot in group %s for split"), *SourceSlot.GroupTag.ToString());
        return INDEX_NONE;
    }

    UMythicItemInstance *NewItem = SourceItem->CloneForStackSplit(this, SplitAmount);

    if (!NewItem) {
        UE_LOG(Myth, Error,
               TEXT("ServerSplitStack: failed to clone current immutable stack state"));
        return INDEX_NONE;
    }

    if (!SetItemInSlot(TargetSlotIndex, NewItem)) {
        UE_LOG(Myth, Error, TEXT("ServerSplitStack: failed to place split item in slot %d"), TargetSlotIndex);
        NewItem->MarkAsGarbage();
        return INDEX_NONE;
    }

    SourceItem->SetStackSize(SourceItem->GetStacks() - SplitAmount);
    return TargetSlotIndex;
}

void UMythicInventoryComponent::ServerSwapSlots_Implementation(int32 SlotA, int32 SlotB) {
    if (!TrySwapSlotsTransactional(SlotA, SlotB)) {
        UE_LOG(Myth, Warning,
               TEXT("ServerSwapSlots: transactional swap rejected for slots %d / %d; old slots and GAS preserved"),
               SlotA, SlotB);
    }
}

bool UMythicInventoryComponent::TrySwapSlotsTransactional(const int32 SlotA, const int32 SlotB) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return false;
    }

    if (SlotA == SlotB) {
        return Slots.IsValidIndex(SlotA);
    }

    if (!Slots.IsValidIndex(SlotA) || !Slots.IsValidIndex(SlotB)) {
        UE_LOG(Myth, Warning, TEXT("ServerSwapSlots: invalid slot indices %d / %d"), SlotA, SlotB);
        return false;
    }

    UMythicItemInstance *ItemA = Slots.Items[SlotA].SlottedItemInstance;
    UMythicItemInstance *ItemB = Slots.Items[SlotB].SlottedItemInstance;

    if (!ItemA && !ItemB) {
        return true;
    }

    const FStagedSlotMutation Mutations[] = {
        {this, SlotA, ItemA, ItemB},
        {this, SlotB, ItemB, ItemA}
    };
    return CommitSlotMutationsTransactional(MakeArrayView(Mutations));
}

void UMythicInventoryComponent::ServerQuickMoveToInventory_Implementation(int32 SourceSlotIndex, UMythicInventoryComponent *TargetInventory) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!TargetInventory) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: null target inventory"));
        return;
    }

    if (TargetInventory == this) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: target is self"));
        return;
    }

    if (!Slots.IsValidIndex(SourceSlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: invalid source slot %d"), SourceSlotIndex);
        return;
    }

    if (!Slots.Items[SourceSlotIndex].bCanPlayerTake) {
        UE_LOG(Myth, Warning, TEXT("ServerQuickMoveToInventory: slot %d does not allow player take"), SourceSlotIndex);
        return;
    }

    if (Slots.Items[SourceSlotIndex].IsGearSlot()) {
        UMythicItemInstance *EquippedItem = Slots.Items[SourceSlotIndex].SlottedItemInstance;
        if (!EquippedItem) {
            return;
        }
        // Do not release first: an equipment-to-inventory move is one slot/GAS transaction or no move at all.
        for (int32 TargetSlotIndex = 0; TargetSlotIndex < TargetInventory->Slots.Num(); ++TargetSlotIndex) {
            if (!TargetInventory->Slots.Items[TargetSlotIndex].SlottedItemInstance
                && TargetInventory->CanSlotAcceptItem(TargetSlotIndex, EquippedItem, true)
                && TargetInventory->TryTransferToSlot(EquippedItem, TargetSlotIndex)) {
                return;
            }
        }
        return;
    }

    UMythicItemInstance *Released = ReleaseFromSlot(SourceSlotIndex);
    if (!Released) {
        return;
    }

    int32 OriginalQty = Released->GetStacks();
    int32 Added = TargetInventory->AddToAnySlot(Released, true);

    if (Added == 0) {
        SetItemInSlot(SourceSlotIndex, Released);
        return;
    }

    if (Added < OriginalQty && IsValid(Released)) {
        SetItemInSlot(SourceSlotIndex, Released);
    }
}

void UMythicInventoryComponent::ServerSortGroup_Implementation(FGameplayTag GroupTag, ESortMode Mode) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!GroupTag.IsValid()) {
        return;
    }

    TArray<int32> GroupSlotIndices;
    TArray<UMythicItemInstance *> GroupItems;

    for (int32 i = 0; i < Slots.Num(); ++i) {
        const FMythicInventorySlotEntry &Slot = Slots.Items[i];
        if (Slot.GroupTag != GroupTag) {
            continue;
        }
        if (Slot.IsGearSlot()) {
            UE_LOG(Myth, Warning, TEXT("ServerSortGroup: cannot sort equipment group %s"), *GroupTag.ToString());
            return;
        }
        GroupSlotIndices.Add(i);
        if (Slot.SlottedItemInstance) {
            GroupItems.Add(Slot.SlottedItemInstance);
        }
    }

    if (GroupItems.Num() <= 1) {
        return;
    }

    GroupItems.Sort([Mode](const UMythicItemInstance &A, const UMythicItemInstance &B) {
        const UItemDefinition *DefA = A.GetItemDefinition();
        const UItemDefinition *DefB = B.GetItemDefinition();
        if (!DefA || !DefB) {
            return DefA != nullptr;
        }

        switch (Mode) {
            case ESortMode::ByRarity:
                return static_cast<int32>(DefA->Rarity) > static_cast<int32>(DefB->Rarity);
            case ESortMode::ByType:
                return DefA->ItemType.ToString() < DefB->ItemType.ToString();
            case ESortMode::ByName:
                return DefA->Name.ToString() < DefB->Name.ToString();
            case ESortMode::ByValue:
                return DefA->Value > DefB->Value;
            case ESortMode::ByWeight:
                return DefA->Weight > DefB->Weight;
        }
        return false;
    });

    for (int32 SlotIdx : GroupSlotIndices) {
        UMythicItemInstance *Inst = Slots.Items[SlotIdx].SlottedItemInstance;
        if (Inst) {
            Inst->SetInventory(nullptr, INDEX_NONE);
            Slots.ModifySlotAtIndex(SlotIdx, [](FMythicInventorySlotEntry &SlotData) {
                SlotData.SlottedItemInstance = nullptr;
            });
        }
    }

    int32 ItemIdx = 0;
    for (int32 SlotIdx : GroupSlotIndices) {
        if (ItemIdx >= GroupItems.Num()) {
            break;
        }

        UMythicItemInstance *Item = GroupItems[ItemIdx];
        Slots.ModifySlotAtIndex(SlotIdx, [this, Item, SlotIdx](FMythicInventorySlotEntry &Slot) {
            Slot.SlottedItemInstance = Item;
            Item->SetOwner(this);
            Item->SetInventory(this, SlotIdx);
        });
        NotifyItemInstanceUpdated(SlotIdx);
        ++ItemIdx;
    }

    for (int32 i = ItemIdx; i < GroupSlotIndices.Num(); ++i) {
        NotifyItemInstanceUpdated(GroupSlotIndices[i]);
    }
}

void UMythicInventoryComponent::ServerDepositAll_Implementation(UMythicInventoryComponent *Target, FGameplayTag OptionalTypeFilter) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!Target || Target == this) {
        return;
    }

    for (int32 i = Slots.Num() - 1; i >= 0; --i) {
        const FMythicInventorySlotEntry &Slot = Slots.Items[i];

        if (Slot.IsGearSlot()) {
            continue;
        }

        if (!Slot.bCanPlayerTake) {
            continue;
        }

        UMythicItemInstance *Item = Slot.SlottedItemInstance;
        if (!Item) {
            continue;
        }

        if (OptionalTypeFilter.IsValid()) {
            const UItemDefinition *Def = Item->GetItemDefinition();
            if (!Def || !Def->ItemType.MatchesTag(OptionalTypeFilter)) {
                continue;
            }
        }

        UMythicItemInstance *Released = ReleaseFromSlot(i);
        if (!Released) {
            continue;
        }

        int32 OriginalQty = Released->GetStacks();
        int32 Added = Target->AddToAnySlot(Released, true);

        if (Added == 0) {
            SetItemInSlot(i, Released);
            continue;
        }

        if (Added < OriginalQty && IsValid(Released)) {
            SetItemInSlot(i, Released);
        }
    }
}

void UMythicInventoryComponent::ServerUseItemInSlot_Implementation(int32 SlotIndex) {
    AActor *lOwner = GetOwner();
    if (!lOwner || !lOwner->HasAuthority()) {
        return;
    }

    if (!Slots.IsValidIndex(SlotIndex)) {
        UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: invalid slot %d"), SlotIndex);
        return;
    }

    UMythicItemInstance *Item = Slots.Items[SlotIndex].SlottedItemInstance;
    if (!Item) {
        UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: slot %d is empty"), SlotIndex);
        return;
    }

    if (!Item->HasTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def || !Def->ItemType.MatchesTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
            UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: item in slot %d does not support in-inventory use"), SlotIndex);
            return;
        }
    }

    if (const auto *ActionFrag = Item->GetFragment<UActionableItemFragment>()) {
        const_cast<UActionableItemFragment *>(ActionFrag)->ExecuteGenericAction(Item);
    }
    else {
        UE_LOG(Myth, Warning, TEXT("ServerUseItemInSlot: no actionable fragment found on item in slot %d"), SlotIndex);
    }
}

bool UMythicInventoryComponent::CanUseItemInSlot(int32 SlotIndex) const {
    if (!Slots.IsValidIndex(SlotIndex)) {
        return false;
    }

    UMythicItemInstance *Item = Slots.Items[SlotIndex].SlottedItemInstance;
    if (!Item) {
        return false;
    }

    if (!Item->HasTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
        const UItemDefinition *Def = Item->GetItemDefinition();
        if (!Def || !Def->ItemType.MatchesTag(ITEMIZATION_ACTIONTYPE_ININVENTORY)) {
            return false;
        }
    }

    return Item->GetFragment<UActionableItemFragment>() != nullptr;
}
