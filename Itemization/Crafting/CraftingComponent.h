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

    void AddItem(UItemDefinition *ItemDefinition, int32 Quantity) {
        FCraftingQueueEntry NewEntry;
        NewEntry.ItemDefinition = ItemDefinition;
        NewEntry.Quantity = Quantity;

        Items.Add(NewEntry);
        MarkItemDirty(NewEntry);

        // Manually call PostReplicatedAdd on server as it won't be called automatically
        TArray<int32> AddedIndices;
        AddedIndices.Add(Items.Num() - 1);
        PostReplicatedAdd(AddedIndices, Items.Num());
    }

    void RemoveItems(const TArrayView<int32> Indices) {
        PreReplicatedRemove(Indices, Items.Num() - Indices.Num());
        for (int32 Index : Indices) {
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
