// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MythicLootSource.generated.h"

class UMythicLootTable;

UENUM()
enum class ELootType : uint8 {
    // HitReward - Spawn items on hit (i.e mining, chopping, etc)
    HitReward,
    // DestroyReward - Spawn items on destroy (i.e. breaking a container, killing an enemy, etc)
    DestroyReward,
    // Inventory - Open an inventory with items in it (i.e. a backpack, chest, etc)
    Inventory,
    // Interact - Spawn items in the world on interact (i.e. interacting with a rack, a pile of bones, etc)
    Interact
};

USTRUCT(BlueprintType)
struct FLootSource {
    GENERATED_BODY()
    
    UPROPERTY(EditAnywhere)
    UMythicLootTable* LootTable;

    // Max number of items to spawn from this table, not including Guaranteed items. This does not guarantee that this many items will spawn, it's just the max.
    UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="100"))
    int32 MaxItems = 1;

    // Chance to spawn items from this table - 0.0 to 1.0 (1.0 will always spawn)
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0"))
    float DropChance = 1.0f;

    // The type of loot source
    UPROPERTY(EditAnywhere)
    ELootType Type = ELootType::HitReward;

    // If true, each player will get their own separate loot from this source
    // i.e if a player mines a rock, only one instance of the rock is spawned for all players, once someone loots it, it's gone.
    // but if this is true, a player mines the rock, and each player gets their own rock to loot, player 1 can loot their rock, and player 2 can loot their rock.
    UPROPERTY(EditAnywhere)
    bool IndividualLoot = false;
};

/**
 * 
 */
UCLASS(BlueprintType)
class MYTHIC_API UMythicLootSource : public UDataAsset {
    GENERATED_BODY()

public:
    // The list of item sources to be used for spawning loot
    UPROPERTY(EditAnywhere)
    TArray<FLootSource> Sources;

    // Flag to indicate whether all sources should be rolled or just one
    // If true, will evaluate the drop chance for each source and spawn items from ALL SOURCES that pass the drop chance
    // If false, will evaluate the drop chance for each source and spawn items from the FIRST SOURCE that passes the drop chance
    UPROPERTY(EditAnywhere)
    bool bRollAllSources = false;
};
