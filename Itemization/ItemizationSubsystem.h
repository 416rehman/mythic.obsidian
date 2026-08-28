
#pragma once

#include "CoreMinimal.h"
#include "Inventory/ItemDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ItemizationSubsystem.generated.h"

DECLARE_MULTICAST_DELEGATE_TwoParams(FOnMythicItemCreationClosuresPrewarmed,
                                     bool /* bAllSucceeded */,
                                     int32 /* FailedDefinitionCount */);

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

    int32 PendingCreationClosureCount = 0;
    int32 FailedCreationClosureCount = 0;
    bool bCreationClosurePrewarmComplete = false;
    FOnMythicItemCreationClosuresPrewarmed CreationClosuresPrewarmed;

    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    void OnAllItemDefsLoaded();
    void BeginCreationClosurePrewarm();
    void OnCreationClosurePrewarmComplete(FPrimaryAssetId ItemDefinitionId, bool bSuccess);
    void OnCraftingRequirementsLoaded();
    void ProcessCraftingRequirements();
    virtual void Deinitialize() override;

public:
    /**
     * True once every loaded ItemDefinition has completed its exact profile/grant-closure request.
     * Authoritative startup systems can use this as a coarse readiness barrier; transactional code
     * must still use UMythicItemFactorySubsystem for the final per-definition readiness check.
     */
    bool AreItemCreationClosuresPrewarmed() const { return bCreationClosurePrewarmComplete; }
    bool DidAllItemCreationClosuresPrewarm() const {
        return bCreationClosurePrewarmComplete && FailedCreationClosureCount == 0;
    }
    int32 GetFailedCreationClosureCount() const { return FailedCreationClosureCount; }
    FOnMythicItemCreationClosuresPrewarmed &OnItemCreationClosuresPrewarmed() {
        return CreationClosuresPrewarmed;
    }

    /** Returns the startup-loaded item-definition catalogue; callers receive no ownership of the subsystem cache. */
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    const TArray<UItemDefinition *> &GetItemDefinitions() const { return CachedItemDefs; }

    /** Reports whether any loaded authoritative crafting recipe consumes the supplied item definition. */
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    bool IsCraftingIngredient(UItemDefinition *Item) const;

    /** Resolves an already-loaded item definition by Primary Asset ID without initiating a synchronous asset load. */
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    UItemDefinition *GetItemDefinition(FPrimaryAssetId ItemId) const;

    /** Returns loaded item definitions that have satisfied the subsystem's canonical crafting-requirement scan. */
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    const TArray<UItemDefinition *> &GetAllCraftableItems() const { return CachedCraftableItems; }

    /** Copies loaded craftable definitions matching the requested hierarchical item type into OutItems. */
    UFUNCTION(BlueprintCallable, Category = "Itemization")
    void GetCraftableItemsByType(FGameplayTag ItemType, TArray<UItemDefinition *> &OutItems) const;
};
