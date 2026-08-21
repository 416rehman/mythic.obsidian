#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConversionSubsystem.generated.h"

class UConversionRecipe;
class UMythicItemInstance;
class UAbilitySystemComponent;

UCLASS()
class MYTHIC_API UConversionSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPrimaryAssetId> AllRecipeIds;

    UPROPERTY()
    TArray<TObjectPtr<UConversionRecipe>> CachedRecipes;

    TMap<FGameplayTag, TObjectPtr<UConversionRecipe>> RecipesById;

    bool bIndicesBuilt = false;

protected:
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;

    void OnAllRecipesLoaded();
    void OnRecipeDependenciesLoaded();
    void BuildIndices();

public:
    DECLARE_MULTICAST_DELEGATE(FOnRecipesReady);
    FOnRecipesReady OnRecipesReady;

    bool AreRecipesReady() const { return bIndicesBuilt; }

    // Resolve a recipe by its stable id. Returns null if unknown or not yet loaded (callers MUST null-check).
    UFUNCTION(BlueprintCallable, Category="Conversion")
    UConversionRecipe *GetRecipeById(FGameplayTag RecipeId) const;

    // All recipes whose station gate is satisfied by StationTags (instigator gating is presentation; locked
    // recipes are still returned so the UI can show them greyed-out). InstigatorASC reserved for future use.
    UFUNCTION(BlueprintCallable, Category="Conversion")
    void GetRecipesForStation(const FGameplayTagContainer &StationTags, const UAbilitySystemComponent *InstigatorASC,
                              TArray<UConversionRecipe *> &OutRecipes) const;

    UConversionRecipe *FindMatchingRecipe(const FGameplayTagContainer &StationTags, const TArray<UMythicItemInstance *> &Inputs,
                                          const UAbilitySystemComponent *InstigatorASC,
                                          const TArray<UConversionRecipe *> *RestrictTo) const;

    const TArray<TObjectPtr<UConversionRecipe>> &GetAllRecipes() const { return CachedRecipes; }
};
