// 

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemizationSubsystem.generated.h"

class UItemDefinition;
/**
 * focused on managing the static data (item definitions) that describe what items are, their properties, and how they might be used (such as for crafting)
 */
UCLASS()
class MYTHIC_API UItemizationSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

    // Array of loaded item defs Ids - can be used to get the actual item defs
    UPROPERTY()
    TArray<FPrimaryAssetId> LoadedItemDefIds;

    // Array of all Item Definitions
    UPROPERTY()
    TArray<UItemDefinition *> CachedItemDefs;

    // Items that are used as crafting ingredients to craft other items
    UPROPERTY()
    TSet<FPrimaryAssetId> CraftingIngredientsIds;

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    void OnAssetsLoaded();
    virtual void Deinitialize() override;

public:
    // Get all the Item Definitions
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    const TArray<UItemDefinition *> &GetItemDefinitions() const { return CachedItemDefs; }

    // Check if an item is a crafting item
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    bool IsCraftingIngredient(UItemDefinition *Item) const;

    // Get an item definition by its ID
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    UItemDefinition *GetItemDefinition(FPrimaryAssetId ItemId) const;
};
