#include "ConversionSubsystem.h"

#include "AbilitySystemComponent.h"
#include "ConversionRecipe.h"
#include "Mythic.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/Inventory/MythicItemInstance.h"
#include "System/MythicAssetManager.h"

void UConversionSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    check(UMythicAssetManager::IsInitialized());
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    AssetManager.GetPrimaryAssetIdList(UMythicAssetManager::ConversionRecipeType, AllRecipeIds);
    UE_LOG(Myth, Log, TEXT("ConversionSubsystem: loading %d recipe assets"), AllRecipeIds.Num());

    AssetManager.LoadPrimaryAssets(AllRecipeIds, TArray<FName>(),
                                   FStreamableDelegate::CreateUObject(this, &UConversionSubsystem::OnAllRecipesLoaded));
}

void UConversionSubsystem::OnAllRecipesLoaded() {
    UMythicAssetManager &AssetManager = UMythicAssetManager::Get();

    CachedRecipes.Reset();
    TSet<FPrimaryAssetId> DepIds;

    auto AddDep = [&](const TSoftObjectPtr<UItemDefinition> &Ref) {
        if (Ref.IsNull()) {
            return;
        }
        const FPrimaryAssetId Id = AssetManager.GetPrimaryAssetIdForPath(Ref.ToSoftObjectPath());
        if (Id.IsValid()) {
            DepIds.Add(Id);
        }
    };

    for (const FPrimaryAssetId &AssetId : AllRecipeIds) {
        UObject *Obj = AssetManager.GetPrimaryAssetObject(AssetId);
        UConversionRecipe *Recipe = Cast<UConversionRecipe>(Obj);
        if (!IsValid(Recipe)) {
            UE_LOG(Myth, Warning, TEXT("ConversionSubsystem: asset %s is not a UConversionRecipe"), *AssetId.ToString());
            continue;
        }

        CachedRecipes.Add(Recipe);

        for (const FConversionIngredient &Ing : Recipe->Inputs) {
            AddDep(Ing.ExactItem);
        }
        for (const FConversionProduct &P : Recipe->Products) {
            AddDep(P.ItemDefinition);
            AddDep(P.TransformToDefinition);
        }
        for (const FFuelDefinition &Fuel : Recipe->Process.AcceptedFuels) {
            AddDep(Fuel.FuelMatch.ExactItem);
        }
    }

    // Only load deps not already part of the recipe set.
    DepIds = DepIds.Difference(TSet<FPrimaryAssetId>(AllRecipeIds));

    if (DepIds.Num() > 0) {
        AssetManager.LoadPrimaryAssets(DepIds.Array(), TArray<FName>(),
                                       FStreamableDelegate::CreateUObject(this, &UConversionSubsystem::OnRecipeDependenciesLoaded));
    }
    else {
        BuildIndices();
    }
}

void UConversionSubsystem::OnRecipeDependenciesLoaded() {
    BuildIndices();
}

void UConversionSubsystem::BuildIndices() {
    RecipesById.Reset();

    for (const TObjectPtr<UConversionRecipe> &Recipe : CachedRecipes) {
        if (!Recipe) {
            continue;
        }
        if (!Recipe->RecipeId.IsValid()) {
            UE_LOG(Myth, Error, TEXT("ConversionSubsystem: recipe '%s' has no RecipeId; skipped."), *Recipe->GetName());
            continue;
        }
        if (RecipesById.Contains(Recipe->RecipeId)) {
            UE_LOG(Myth, Error, TEXT("ConversionSubsystem: duplicate RecipeId '%s' (recipe '%s'); keeping the first."),
                   *Recipe->RecipeId.ToString(), *Recipe->GetName());
            continue;
        }
        RecipesById.Add(Recipe->RecipeId, Recipe);
    }

    bIndicesBuilt = true;
    UE_LOG(Myth, Log, TEXT("ConversionSubsystem: %d recipes indexed"), RecipesById.Num());
    OnRecipesReady.Broadcast();
}

void UConversionSubsystem::Deinitialize() {
    Super::Deinitialize();
    if (UMythicAssetManager::IsInitialized()) {
        UMythicAssetManager::Get().UnloadPrimaryAssets(AllRecipeIds);
    }
}

UConversionRecipe *UConversionSubsystem::GetRecipeById(FGameplayTag RecipeId) const {
    const TObjectPtr<UConversionRecipe> *Found = RecipesById.Find(RecipeId);
    return Found ? *Found : nullptr;
}

void UConversionSubsystem::GetRecipesForStation(const FGameplayTagContainer &StationTags,
                                                const UAbilitySystemComponent *InstigatorASC,
                                                TArray<UConversionRecipe *> &OutRecipes) const {
    OutRecipes.Reset();
    for (const TObjectPtr<UConversionRecipe> &Recipe : CachedRecipes) {
        if (Recipe && Recipe->MatchesStation(StationTags)) {
            OutRecipes.Add(Recipe);
        }
    }
}

UConversionRecipe *UConversionSubsystem::FindMatchingRecipe(const FGameplayTagContainer &StationTags,
                                                            const TArray<UMythicItemInstance *> &Inputs,
                                                            const UAbilitySystemComponent *InstigatorASC,
                                                            const TArray<UConversionRecipe *> *RestrictTo) const {
    FGameplayTagContainer InstigatorTags;
    if (InstigatorASC) {
        InstigatorASC->GetOwnedGameplayTags(InstigatorTags);
    }

    UConversionRecipe *Best = nullptr;

    auto Consider = [&](UConversionRecipe *R) {
        if (!R || !R->MatchesStation(StationTags)) {
            return;
        }
        if (!R->Requirements.InstigatorTagQuery.IsEmpty()) {
            if (!InstigatorASC || !R->Requirements.InstigatorTagQuery.Matches(InstigatorTags)) {
                return;
            }
        }
        if (!R->MatchesInputs(Inputs)) {
            return;
        }

        if (!Best) {
            Best = R;
            return;
        }
        // Higher Priority wins; ties broken deterministically by RecipeId (author-stable, rename-safe), so an
        // overlap never deadlocks the station.
        if (R->Process.Priority > Best->Process.Priority) {
            Best = R;
        }
        else if (R->Process.Priority == Best->Process.Priority && R != Best
            && R->RecipeId.ToString() < Best->RecipeId.ToString()) {
            Best = R;
        }
    };

    if (RestrictTo) {
        for (UConversionRecipe *R : *RestrictTo) {
            Consider(R);
        }
    }
    else {
        for (const TObjectPtr<UConversionRecipe> &R : CachedRecipes) {
            Consider(R);
        }
    }

    return Best;
}
