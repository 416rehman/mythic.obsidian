#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ConversionSubsystem.generated.h"

class UConversionRecipe;
class UMythicItemInstance;
class UAbilitySystemComponent;

/**
 * Registry of all UConversionRecipe assets (cooked, and later mod-supplied). Mirrors UItemizationSubsystem:
 * a two-pass async load (recipes, then their referenced item defs), then index building. Exists on clients
 * too (no ShouldCreateSubsystem override) so a replicated RecipeId can be resolved to its asset locally.
 */
UCLASS()
class MYTHIC_API UConversionSubsystem : public UGameInstanceSubsystem {
    GENERATED_BODY()

    UPROPERTY()
    TArray<FPrimaryAssetId> AllRecipeIds;

    UPROPERTY()
    TArray<TObjectPtr<UConversionRecipe>> CachedRecipes;

    // Exact id -> recipe (essential: resolves replicated RecipeId on every peer).
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
    // Broadcast once index building completes. Late subscribers should check AreRecipesReady() first.
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

    // Priority-honoring auto-match: the single recipe (within RestrictTo, or all recipes if null) that matches
    // the station, the instigator gate, and the given inputs. Ties are broken deterministically (higher
    // Process.Priority wins, then lexicographic RecipeId), so the result is always uniquely determined — auto
    // stations rely on this rather than stalling (overlapping same-Priority signatures are an IsDataValid warning).
    UConversionRecipe *FindMatchingRecipe(const FGameplayTagContainer &StationTags, const TArray<UMythicItemInstance *> &Inputs,
                                          const UAbilitySystemComponent *InstigatorASC,
                                          const TArray<UConversionRecipe *> *RestrictTo) const;

    const TArray<TObjectPtr<UConversionRecipe>> &GetAllRecipes() const { return CachedRecipes; }
};
