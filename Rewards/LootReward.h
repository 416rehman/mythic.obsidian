// 

#pragma once

#include "CoreMinimal.h"
#include "RewardBase.h"
#include "LootReward.generated.h"

class IInventoryProviderInterface;
class UMythicLootManagerSubsystem;
class UMythicInventoryComponent;
class UItemDefinition;

USTRUCT(BlueprintType)
struct FLootTableEntry {
    GENERATED_BODY()

    // Soft reference to the class of the item to spawn
    UPROPERTY(EditAnywhere)
    UItemDefinition *Item = nullptr;

    // The qty range to spawn, inclusive. (random amount between min and max will be spawned)
    // If item is not stackable, this will be ignored
    UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="100"))
    FInt32Interval StackRange = FInt32Interval(1, 1);

    // Override the drop chance of this item. If greater than 0, player's exp will not affect the drop chance of this item and will use this value instead.
    // If false, the global drop chance will be used (which is affected by player's Exp in relation to the Item's Rarity).
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
    float OverrideDropChance = 0.0f;
};

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicLootTable : public UDataAsset {
    GENERATED_BODY()

public:
    // The list of items to spawn
    UPROPERTY(EditAnywhere)
    TArray<FLootTableEntry> Entries;

    // Max number of items to spawn from this table. This does not guarantee that this many items will spawn, it's just the max.
    UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="100"))
    int32 MaxItems = 1;

    // Chance to spawn items from this table - 0.0 to 1.0 (1.0 will always spawn)
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
    float DropChance = 0.3f;
};

USTRUCT(BlueprintType, Blueprintable)
struct FLootTableOverride {
    GENERATED_BODY()

    // Array of loot tables to use for this source. For each table, a draw will be made to determine if items from that table will be spawned.
    // If table(s) are drawn, the items will be randomly selected from the table(s) based on their drop chance.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TArray<UMythicLootTable *> LootTables;

    // If true, each player will get their own separate loot from this source
    // i.e if a player mines a rock, only one instance of the rock is spawned for all players, once someone loots it, it's gone.
    // but if this is true, a player mines the rock, and each player gets their own rock to loot, player 1 can loot their rock, and player 2 can loot their rock.
    UPROPERTY(EditAnywhere)
    bool IsPrivate = false;

    // If true, this source will skip the global loot table.
    UPROPERTY(EditAnywhere)
    bool bSkipGlobal = false;
};

//--------------------------------------------------------------------------------------------------
// Loot Reward
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType, Blueprintable)
struct FLootRewardContext : public FRewardContext {
    GENERATED_BODY()

    // The inventory in which to place the item. If not specified, the item will be dropped in the drop location instead.
    // If the inventory is full, the item will drop at the inventory owner's location.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    TObjectPtr<UMythicInventoryComponent> PutInInventory = nullptr;

    // The item will be spawned here. If not specified, the item will be spawned at the player's location.
    // Defaults to the player's location.
    // ONLY USED IF PutInInventory is not specified.
    UPROPERTY(BlueprintReadWrite, EditAnywhere, meta=(AllowPrivateAccess=true))
    FVector SpawnLocation = FVector::ZeroVector;

    // Item level to spawn items at.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    int32 ItemLevel = 0;
};


/**
 * 
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API ULootReward : public URewardBase {
    GENERATED_BODY()

    static void RequestLootFromSource(float CommonRate, float RareRate, float EpicRate, float LegendaryRate, float MythicRate,
                                      APlayerController *PlayerController,
                                      int32 DropLevel, UMythicLootTable *LootTable, TScriptInterface<IInventoryProviderInterface> InventoryProvider, bool isPrivate, FVector SpawnLocation,
                                      UMythicLootManagerSubsystem *
                                      MythicLootManager);

public:
    virtual bool Give(FRewardContext &Context) const override;

    // The loot table to use
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Loot Reward Context")
    FLootTableOverride OverridenLootSource;

    // Give items from loot table(s)
    // TO DROP AN ITEM: Do NOT provide an inventory, and optionally provide a location to drop the item at, if no location is provided, player's location will be used.
    // TO PUT AN ITEM IN AN INVENTORY: Provide an inventory. If inventory is full, inventory owner's location will be used to drop the item.
    UFUNCTION(BlueprintCallable, Category = "Loot Reward")
    static bool GiveLootReward(ULootReward *Reward, FLootRewardContext Context);
};
