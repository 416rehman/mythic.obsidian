// 

#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "Rewards/LootReward.h"
#include "MythicDevSettings.generated.h"

/**
 * 
 */
UCLASS(config=Game, DefaultConfig)
class MYTHIC_API UMythicDevSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    /// Singleton Access
    static UMythicDevSettings *Get();

    UMythicDevSettings(const FObjectInitializer& ObjectInitializer);
    
    // Maximum Level derived from the sum of all proficiencies
    UPROPERTY(Config, EditAnywhere, Category = "Experience")
    int32 MaxLevel = 60;

    // Global Loot table
    UPROPERTY(Config, EditAnywhere, Category = "Loot")
    TSoftObjectPtr<UMythicLootTable> GlobalLootTable;

    // Global Drop Rate Multiplier
    UPROPERTY(Config, EditAnywhere, Category = "Loot")
    float DropRateMultiplier = 1.0f;
};
