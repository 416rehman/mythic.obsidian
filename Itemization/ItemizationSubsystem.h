// 

#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemizationSubsystem.generated.h"
/**
 * focused on managing the static data (item definitions) that describe what items are, their properties, and how they might be used (such as for crafting)
 */
UCLASS()
class MYTHIC_API UItemizationSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

    // Array of loaded item defs Ids - can be used to get the actual item defs
    UPROPERTY()
    TArray<FPrimaryAssetId> AllItemDefIds;

    // Array of all Item Definitions
    UPROPERTY()
    TArray<UItemDefinition *> CachedItemDefs;

    // Items that are used as crafting ingredients to craft other items
    UPROPERTY()
    TSet<FPrimaryAssetId> CraftingIngredientsIds;

protected:
    // Cached list of all craftable items
    UPROPERTY()
    TArray<UItemDefinition *> CachedCraftableItems;

    // Cached map of craftable items by ItemType
    TMap<FGameplayTag, TArray<UItemDefinition *>> CachedCraftableItemsByType;

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    void OnAllItemDefsLoaded();
    void OnCraftingRequirementsLoaded();
    void ProcessCraftingRequirements();
    virtual void Deinitialize() override;

public:
    // Get all the Item Definitions
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    const TArray<UItemDefinition *> &GetItemDefinitions() const { return CachedItemDefs; }

    // Check if an item is used as a crafting ingredient for crafting other items
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    bool IsCraftingIngredient(UItemDefinition *Item) const;

    // Get an item definition by its ID
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    UItemDefinition *GetItemDefinition(FPrimaryAssetId ItemId) const;

    // Get all craftable items
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    const TArray<UItemDefinition *> &GetAllCraftableItems() const { return CachedCraftableItems; }

    // Get craftable items by ItemType
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    void GetCraftableItemsByType(FGameplayTag ItemType, TArray<UItemDefinition *> &OutItems) const;
};
