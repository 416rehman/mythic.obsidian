#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "MythicLootSettings.generated.h"

class UMythicAffixCatalogue;

UCLASS(Config = Game, DefaultConfig, meta = (DisplayName = "Mythic Loot"))
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
     * Number of TALENTS rolled on an item, indexed by EItemRarity (Common=0, Rare=1, Epic=2, Legendary=3, Mythic=4).
     * Owner-corrected model: Talents are passive abilities innate to HIGH-RARITY weapons, so any Rare-or-better item
     * rolls at least one (Common=0; Rare/Epic/Legendary=1; Mythic=2). An out-of-range index falls back to the prior
     * (Legendary=1, Mythic=2, else 0) mapping at the call site, so adding rarities or shortening the array can never
     * crash or mis-roll.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Talents")
    TArray<int32> TalentCountByRarity = {0, 1, 1, 1, 2};

    /**
     * The shared affix catalogue: every affix that exists, and which item types may roll each one. An item whose
     * Affixes fragment authors no pool of its own falls back to this, matched on the item's Itemization.Type tag.
     * Unset (the default) means an item rolls only what its own fragment authors, so content behaves exactly as it
     * did before the catalogue existed.
     */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, Category = "Affixes")
    TSoftObjectPtr<UMythicAffixCatalogue> AffixCatalogue;

    /**
     * The loaded catalogue, or null when none is configured OR the load failed. Loads at most ONCE per assignment
     * and holds it: a soft pointer is not a GC reachability edge, so loading per item drop would let the package
     * unload and make the next drop block the game thread on a package load. A failed load is remembered too, so a
     * bad path costs one synchronous load rather than one per item drop. Repointing AffixCatalogue in Project
     * Settings clears the cache, so the next drop rolls from the new asset without an editor restart.
     */
    UFUNCTION(BlueprintCallable, Category = "Affixes")
    const UMythicAffixCatalogue *GetAffixCatalogue() const;

    virtual FName GetCategoryName() const override { return FName(TEXT("Game")); }

#if WITH_EDITOR
    virtual void PostEditChangeProperty(struct FPropertyChangedEvent &PropertyChangedEvent) override;
#endif

private:
    // The hard reference that keeps the catalogue alive. Transient: the ini authors AffixCatalogue, never this.
    UPROPERTY(Transient)
    TObjectPtr<UMythicAffixCatalogue> LoadedAffixCatalogue;

    // Separate from a null LoadedAffixCatalogue so a load that FAILED is not retried on every item drop.
    bool bAffixCatalogueLoadAttempted = false;
};
