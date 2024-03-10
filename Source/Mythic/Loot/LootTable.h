// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "LootTable.generated.h"

class UItemDefinition;

USTRUCT(BlueprintType)
struct FLootTableEntry {
    GENERATED_BODY()
    
    // Soft reference to the class of the item to spawn
    UPROPERTY(EditAnywhere)
    TSubclassOf<UItemDefinition> ItemClass;
    
    // The qty range to spawn, inclusive. (random amount between min and max will be spawned)
    // If item is not stackable, this will be ignored
    UPROPERTY(EditAnywhere, meta=(ClampMin="1", ClampMax="100"))
    FInt32Interval StackRange = FInt32Interval(1, 1);

    // Guaranteed drop - If true, this item will always be spawned if the loot table is rolled
    UPROPERTY(EditAnywhere)
    bool bGuaranteed = false;

    // Override the drop chance of this item. If greater than 0, player's exp will not affect the drop chance of this item and will use this value instead.
    // If false, the global drop chance will be used (which is affected by player's Exp).
    UPROPERTY(EditAnywhere, meta=(ClampMin="0.0", ClampMax="1.0", EditCondition="!bGuaranteed"))
    float OverrideDropChance = 0.0f;
};

/**
 * 
 */
UCLASS()
class MYTHIC_API ULootTable : public UDataAsset {
    GENERATED_BODY()

public:

    // The list of items to spawn
    UPROPERTY(EditAnywhere)
    TArray<FLootTableEntry> Entries;
};
