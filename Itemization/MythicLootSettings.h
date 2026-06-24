#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MythicLootSettings.generated.h"

/**
 * Project-wide loot-generation tuning (Project Settings -> Game -> "Mythic Loot Settings").
 *
 * Accessed via GetDefault<UMythicLootSettings>() — a CDO read with NO world dependency, so it is safe to consult from
 * item-instancing code (UAffixesFragment::OnInstanced etc.) that can run during save-load or in the editor, where a
 * GameState/world lookup would be null. Holds global designer knobs for how loot is generated, kept data-driven instead
 * of hardcoded in fragment code.
 */
UCLASS(config = Game, defaultconfig, meta = (DisplayName = "Mythic Loot Settings"))
class MYTHIC_API UMythicLootSettings : public UDeveloperSettings {
    GENERATED_BODY()

public:
    /**
     * Number of RANDOM affixes rolled on an item, indexed by EItemRarity (Common=0, Rare=1, Epic=2, Legendary=3,
     * Mythic=4). Designers tune loot generosity per tier here. Defaults reproduce the prior hardcoded "1 + rarity"
     * mapping (Common=1 ... Mythic=5). A rarity index outside this array falls back to (1 + rarity) at the call site,
     * so adding rarities or shortening the array can never crash or silently roll a wrong count.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TArray<int32> AffixCountByRarity = {1, 2, 3, 4, 5};

    /**
     * Number of TALENTS rolled on an item, indexed by EItemRarity (Common=0 ... Mythic=4). Defaults reproduce the prior
     * hardcoded mapping (Legendary=1, Mythic=2; lower tiers roll none). An out-of-range index falls back to that mapping
     * at the call site, so adding rarities or shortening the array can never crash or mis-roll.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Talents")
    TArray<int32> TalentCountByRarity = {0, 0, 0, 1, 2};

    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }
};
