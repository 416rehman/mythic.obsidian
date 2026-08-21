
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemizationSubsystem.generated.h"
UCLASS()
class MYTHIC_API UItemizationSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPrimaryAssetId> AllItemDefIds;

    UPROPERTY()
    TArray<UItemDefinition *> CachedItemDefs;

    UPROPERTY()
    TSet<FPrimaryAssetId> CraftingIngredientsIds;

protected:
    UPROPERTY()
    TArray<UItemDefinition *> CachedCraftableItems;

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
