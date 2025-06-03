// 

#pragma once

#include "CoreMinimal.h"
#include "RewardBase.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "ItemReward.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FItemRewardContext : public FRewardContext {
    GENERATED_BODY()

    // Item Level - If 0, will use the player's Combat Level.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    int32 ItemLevel = 0;

    // Whether the drop is shared or private (hidden from and not interactable by other players)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    bool bIsPrivate = true;

    // The inventory in which to place the item. If not specified, the item will be dropped in the drop location instead.
    // If the inventory is full, the item will drop at the inventory owner's location.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    TObjectPtr<UMythicInventoryComponent> PutInInventory = nullptr;

    // The item will be spawned here. If not specified, the item will be spawned at the player's location.
    // Defaults to the player's location.
    // ONLY USED IF PutInInventory is not specified.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    FVector SpawnLocation = FVector::ZeroVector;
};

/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UItemReward : public URewardBase {
    GENERATED_BODY()

    virtual bool Give(FRewardContext &Context) const override;

public:
    // The quantity of the item to give.
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    int32 Quantity = 1;

    // The item to give.
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    TObjectPtr<UItemDefinition> Item;

    // Should special celebration be played when the item is given?
    // TODO: No effect yet
    UPROPERTY(BlueprintReadWrite, EditAnywhere)
    bool bCelebrate = true;

    // Give the item to an inventory
    // TO DROP AN ITEM: Do NOT provide an inventory, and optionally provide a location to drop the item at, if no location is provided, player's location will be used.
    // TO PUT AN ITEM IN AN INVENTORY: Provide an inventory. If inventory is full, inventory owner's location will be used to drop the item.
    UFUNCTION(BlueprintCallable)
    static bool GiveItemReward(UItemReward *Reward, FItemRewardContext Context);
};
