// 

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Net/UnrealNetwork.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "Player/MythicPlayerController.h"
#include "CraftingComponent.generated.h"

// Fast array serializer for the crafting queue
// Each entry is a UItemDefinition* representing an item being crafted
USTRUCT(BlueprintType, Blueprintable)
struct FCraftingQueueEntry : public FFastArraySerializerItem {
    GENERATED_BODY()

    // The item being crafted
    UPROPERTY()
    UItemDefinition *ItemDefinition = nullptr;

    // How many of this item are being crafted
    UPROPERTY()
    int32 Quantity = 1;

    // The actor that queued this craft (the material payer). Server-side anchor for refunding the consumed materials on
    // cancel — the only place that knows the per-requirement amounts. Auto-nulls if the actor is destroyed (logout);
    // replicates harmlessly as a NetGUID.
    UPROPERTY()
    TObjectPtr<AActor> RequestingActor = nullptr;
};

USTRUCT()
struct FCraftingQueue : public FFastArraySerializer {
    GENERATED_BODY()

protected:
    UPROPERTY()
    TArray<FCraftingQueueEntry> Items = TArray<FCraftingQueueEntry>();

public:
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FCraftingQueueEntry, FCraftingQueue>(Items, DeltaParms, *this);
    }

    void AddItem(UItemDefinition *ItemDefinition, int32 Quantity, AActor *RequestingActor = nullptr) {
        FCraftingQueueEntry NewEntry;
        NewEntry.ItemDefinition = ItemDefinition;
        NewEntry.Quantity = Quantity;
        NewEntry.RequestingActor = RequestingActor;

        Items.Add(NewEntry);
        MarkItemDirty(NewEntry);

        // Manually call PostReplicatedAdd on server as it won't be called automatically
        TArray<int32> AddedIndices;
        AddedIndices.Add(Items.Num() - 1);
        PostReplicatedAdd(AddedIndices, Items.Num());
    }

    void RemoveItems(const TArrayView<int32> Indices) {
        PreReplicatedRemove(Indices, Items.Num() - Indices.Num());
        // Remove in DESCENDING index order: RemoveAt shifts later elements down, so removing ascending indices in-order
        // deletes the WRONG entries once 2+ are given (mirrors the fix in ResourceManager::RemoveItems / R18-H2). All
        // current callers pass a single index, but the TArrayView batch contract must be correct for any future caller.
        TArray<int32> Sorted(Indices.GetData(), Indices.Num());
        Sorted.Sort([](int32 A, int32 B) { return A > B; });
        for (int32 Index : Sorted) {
            if (Items.IsValidIndex(Index)) {
                Items.RemoveAt(Index);
            }
        }
        MarkArrayDirty();
    }

    const TArray<FCraftingQueueEntry> &GetItems() const {
        return Items;
    }

    // overrides
    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
};

template <>
struct TStructOpsTypeTraits<FCraftingQueue> : TStructOpsTypeTraitsBase2<FCraftingQueue> {
    enum {
        WithNetDeltaSerializer = true,
    };
};


// Event for crafted/refunded item
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnCraftingItemEvent, FCraftingQueueEntry, Item);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UCraftingComponent : public UActorComponent {
    GENERATED_BODY()

public:
    // Sets default values for this component's properties
    UCraftingComponent();

protected:
    // Called when the game starts
    virtual void BeginPlay() override;
    bool QueueNext();

    // Crafting queue
    UPROPERTY(ReplicatedUsing=OnRep_CraftingQueue)
    FCraftingQueue CraftingQueue;

    UFUNCTION()
    void OnRep_CraftingQueue();

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override {
        Super::GetLifetimeReplicatedProps(OutLifetimeProps);
        DOREPLIFETIME(UCraftingComponent, CraftingQueue);
    }

    // Current item's timer
    FTimerHandle CurrentCraftingTimerHandle;

    // Callback when the current item is finished crafting
    void OnCurrentItemCrafted();
    bool VerifyRequirements(UItemDefinition *Item, int32 Amount, TArray<UMythicInventoryComponent *> Inventories,
                            const UAbilitySystemComponent *SchematicsASC) const;

    bool ConsumeRequirements(UItemDefinition *Item, int32 Amount, TArray<UMythicInventoryComponent *> Inventories,
                             const UAbilitySystemComponent *SchematicsASC);

    // Refund a cancelled entry's consumed materials back to the actor that queued it. The component owns this (only it
    // knows the per-requirement amounts) instead of relying on the unlistened OnItemCanceled delegate.
    void RefundRequirements(const FCraftingQueueEntry &Entry);

public:
    UPROPERTY(BlueprintAssignable, Category="Crafting")
    FOnCraftingItemEvent OnItemCrafted;

    UPROPERTY(BlueprintAssignable, Category="Crafting")
    FOnCraftingItemEvent OnItemCanceled;

    // Add item to crafting queue
    UFUNCTION(Blueprintable, BlueprintCallable, Server, Reliable)
    void ServerAddToCraftingQueue(UItemDefinition *ItemDefinition, int32 Quantity = 1, const AActor *RequestingActor = nullptr);

    // Remove item from crafting queue
    UFUNCTION(Server, Reliable)
    void ServerRemoveFromCraftingQueue(int32 index);
};
