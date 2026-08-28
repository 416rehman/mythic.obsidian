#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MythicLootSettings.generated.h"

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Loot"))
class MYTHIC_API UMythicLootSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    /**
     * Number of TALENTS rolled on an item, indexed by EItemRarity (Common=0, Rare=1, Epic=2, Legendary=3, Mythic=4).
     * Owner-corrected model: Talents are passive abilities innate to HIGH-RARITY weapons, so any Rare-or-better item
     * rolls at least one (Common=0; Rare/Epic/Legendary=1; Mythic=2). An out-of-range index falls back to the prior
     * (Legendary=1, Mythic=2, else 0) mapping at the call site, so adding rarities or shortening the array can never
     * crash or mis-roll.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Talents")
    TArray<int32> TalentCountByRarity = {0, 1, 1, 1, 2};

    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
};
