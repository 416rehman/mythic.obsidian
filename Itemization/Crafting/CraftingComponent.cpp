// 


#include "CraftingComponent.h"

#include "AbilitySystemComponent.h"
#include "Itemization/Inventory/Fragments/Passive/CraftableFragment.h"
#include "TimerManager.h"
#include "Player/MythicPlayerController.h"
#include "System/MythicAssetManager.h"

// FastArray callbacks (no-op for now, reserved for UI or state updates)
void FCraftingQueue::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    // Intentionally empty; hook for client-side removal handling
}

void FCraftingQueue::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    // Intentionally empty; hook for client-side add handling
}

void FCraftingQueue::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {
    // Intentionally empty; hook for client-side change handling
}

// Sets default values for this component's properties
UCraftingComponent::UCraftingComponent() {
    // Disable ticking per performance guidelines
    PrimaryComponentTick.bCanEverTick = false;
}


// Called when the game starts
void UCraftingComponent::BeginPlay() {
    Super::BeginPlay();
}

void UCraftingComponent::OnRep_CraftingQueue() {
    // Client-side notification point for UI; server drives timers
}

bool UCraftingComponent::QueueNext() {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return false;
    }

    // Don't queue if a crafting timer is already active
    if (GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(CurrentCraftingTimerHandle)) {
        return true;
    }

    const auto &QueuedItems = CraftingQueue.GetItems();
    if (QueuedItems.Num() > 0) {
        const auto &NextItem = QueuedItems[0];
        auto *NextCraftableFragment = UCraftableFragment::GetCraftableFragmentFromDefinition(NextItem.ItemDefinition);
        if (!NextCraftableFragment) {
            UE_LOG(Myth, Error, TEXT("UCraftingComponent::QueueNext: Next item in queue is not craftable."));
            return false;
        }
        if (ensure(GetWorld())) {
            GetWorld()->GetTimerManager().SetTimer(CurrentCraftingTimerHandle, this, &UCraftingComponent::OnCurrentItemCrafted,
                                                   NextCraftableFragment->TimeNeeded, false);
        }
    }

    return true;
}

// Resets the timer to the next item in queue, if none, clears the timer
// And calls OnItemCrafted with the crafted item so the station can either keep it in a buffer, drop it in the world, or put it in the player's inventory
// This is only called on the server
void UCraftingComponent::OnCurrentItemCrafted() {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    if (GetWorld()) {
        GetWorld()->GetTimerManager().ClearTimer(CurrentCraftingTimerHandle);
    }

    const TArray<FCraftingQueueEntry> &QueuedItems = CraftingQueue.GetItems();
    if (QueuedItems.Num() == 0) {
        return;
    }

    // Item crafted, do something with it
    // Get the first item in the queue
    const FCraftingQueueEntry &CurrentItem = QueuedItems[0];

    // Get the craftable fragment from it
    auto CraftableFragment = UCraftableFragment::GetCraftableFragmentFromDefinition(CurrentItem.ItemDefinition);
    if (!CraftableFragment) {
        UE_LOG(Myth, Error, TEXT("UCraftingComponent::OnCurrentItemCrafted: Item in queue is not craftable."));
        // Remove the invalid item to avoid getting stuck
        const int32 IndexToRemove = 0;
        auto IndicesToRemove = TArrayView<int32>(const_cast<int32 *>(&IndexToRemove), 1);
        CraftingQueue.RemoveItems(IndicesToRemove);
        // Try queueing next valid item
        QueueNext();
        return;
    }

    // Remove it from the queue
    const int32 IndexToRemove = 0;
    auto IndicesToRemove = TArrayView<int32>(const_cast<int32 *>(&IndexToRemove), 1);
    CraftingQueue.RemoveItems(IndicesToRemove);

    // Notify that the item is crafted
    OnItemCrafted.Broadcast(CurrentItem);

    // Now that the item is removed from the queue, if there are more items, start the timer for the next item
    QueueNext();
}

bool UCraftingComponent::VerifyRequirements(UItemDefinition *Item, int32 Amount,
                                            TArray<UMythicInventoryComponent *> Inventories,
                                            const UAbilitySystemComponent *SchematicsASC) const {
    checkf(Item, TEXT("Item is null."));
    checkf(Amount > 0, TEXT("Amount must be greater than 0."));
    checkf(SchematicsASC, TEXT("Player does not have an ability system component."));
    checkf(Inventories.Num() > 0, TEXT("No inventories provided."));

    auto CraftableFragment = UCraftableFragment::GetCraftableFragmentFromDefinition(Item);
    checkf(CraftableFragment, TEXT("This item cannot be crafted."));

    // Requires tag?
    if (CraftableFragment->RequiredTag.IsValid()) {
        if (!SchematicsASC->HasMatchingGameplayTag(CraftableFragment->RequiredTag)) {
            UE_LOG(Myth, Warning, TEXT("Missing required schematic tag: %s"), *CraftableFragment->RequiredTag.ToString());
            return false;
        }
    }

    // Only verify, don't consume
    for (auto Requirement : CraftableFragment->CraftingRequirements) {
        auto RequiredItem = Requirement.RequiredItem.Get();
        if (!RequiredItem) {
            UE_LOG(Myth, Warning, TEXT("Crafting requirement item not loaded. Precache by accessing the craftable."));
            return false;
        }
        int32 TotalNeeded = Requirement.RequiredAmount * Amount;
        int32 TotalFound = 0;

        for (auto Inventory : Inventories) {
            if (Inventory) {
                TotalFound += Inventory->GetItemCount(RequiredItem);
            }
        }

        if (TotalFound < TotalNeeded) {
            UE_LOG(Myth, Warning, TEXT("Insufficient %s. Required: %d, Found: %d"),
                   *RequiredItem->Name.ToString(), TotalNeeded, TotalFound);
            return false;
        }
    }

    return true; // Only verification, no consumption
}

bool UCraftingComponent::ConsumeRequirements(UItemDefinition *Item, int32 Amount, TArray<UMythicInventoryComponent *> Inventories,
                                             const UAbilitySystemComponent *SchematicsASC) {
    // First verify we have everything
    if (!VerifyRequirements(Item, Amount, Inventories, SchematicsASC)) {
        return false;
    }

    auto CraftableFragment = UCraftableFragment::GetCraftableFragmentFromDefinition(Item);

    // Now consume the materials
    for (auto Requirement : CraftableFragment->CraftingRequirements) {
        auto RequiredItem = Requirement.RequiredItem.Get();
        if (!RequiredItem) {
            continue; // Already verified in VerifyRequirements
        }
        int32 AmountToRemove = Requirement.RequiredAmount * Amount;

        // Remove from inventories
        for (auto Inventory : Inventories) {
            if (!Inventory || AmountToRemove <= 0)
                continue;

            auto AmountInInventory = Inventory->GetItemCount(RequiredItem);
            auto AmountToRemoveFromInventory = FMath::Min(AmountToRemove, AmountInInventory);

            Inventory->ServerRemoveItemByDefinition(RequiredItem, AmountToRemoveFromInventory);
            AmountToRemove -= AmountToRemoveFromInventory;
        }

        if (AmountToRemove > 0) {
            UE_LOG(Myth, Error, TEXT("Failed to consume all required items for crafting. This should not happen."));
            return false;
        }
    }

    return true;
}

void UCraftingComponent::ServerAddToCraftingQueue_Implementation(UItemDefinition *ItemDefinition, int32 Quantity, const AActor *RequestingActor) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    UE_LOG(Myth, Log, TEXT("UCraftingComponent:: Adding %d of %s to crafting queue."), Quantity,
           ItemDefinition ? *ItemDefinition->Name.ToString() : TEXT("null"));

    if (!ItemDefinition || Quantity <= 0) {
        UE_LOG(Myth, Warning, TEXT("UCraftingComponent::ServerAddToCraftingQueue_Implementation: Invalid parameters."));
        return;
    }

    // Confirm actor has IInventoryProviderInterface
    const IInventoryProviderInterface *InventoryProvider = Cast<IInventoryProviderInterface>(RequestingActor);
    if (!InventoryProvider) {
        UE_LOG(Myth, Warning,
               TEXT("UCraftingComponent::ServerAddToCraftingQueue_Implementation: Requesting actor does not implement IInventoryProviderInterface."));
        return;
    }
    const TArray<UMythicInventoryComponent *> Inventories = InventoryProvider->GetAllInventoryComponents();
    const auto *SchematicsASC = InventoryProvider->GetSchematicsASC();
    if (Inventories.Num() == 0 || !SchematicsASC) {
        UE_LOG(Myth, Warning,
               TEXT("UCraftingComponent::ServerAddToCraftingQueue_Implementation: Requesting actor does not have inventories or ability system component."));
        return;
    }

    if (!ConsumeRequirements(ItemDefinition, Quantity, Inventories, SchematicsASC)) {
        UE_LOG(Myth, Warning, TEXT("UCraftingComponent::ServerAddToCraftingQueue_Implementation: Requirements not met."));
        return;
    };

    const bool bWasEmpty = (CraftingQueue.GetItems().Num() == 0);
    CraftingQueue.AddItem(ItemDefinition, Quantity);

    // If nothing was crafting, start the next item immediately
    if (bWasEmpty) {
        QueueNext();
    }

    UE_LOG(Myth, Log, TEXT("UCraftingComponent:: ✅ Added %d of %s to crafting queue."), Quantity, *ItemDefinition->Name.ToString());
}

void UCraftingComponent::ServerRemoveFromCraftingQueue_Implementation(int32 index) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }

    const TArray<FCraftingQueueEntry> &QueuedItems = CraftingQueue.GetItems();
    if (!QueuedItems.IsValidIndex(index)) {
        UE_LOG(Myth, Error, TEXT("UCraftingComponent::ServerRemoveFromCraftingQueue_Implementation: Invalid index."));
        return;
    }

    // if its the first item and the timer is active, clear the timer, refund, and start the next item
    if (index == 0 && GetWorld() && GetWorld()->GetTimerManager().IsTimerActive(CurrentCraftingTimerHandle)) {
        GetWorld()->GetTimerManager().ClearTimer(CurrentCraftingTimerHandle);
        // Refund the item being removed
        OnItemCanceled.Broadcast(QueuedItems[0]);
    }

    auto IndicesToRemove = TArrayView<int32>(&index, 1);
    CraftingQueue.RemoveItems(IndicesToRemove);

    // Start next if we just removed the active item
    if (index == 0) {
        QueueNext();
    }
}
