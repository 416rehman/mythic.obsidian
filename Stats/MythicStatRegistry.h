// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "Containers/ArrayView.h"
#include "Engine/AssetManagerTypes.h"
#include "GameplayTagContainer.h"

class UMythicStatCategoryDefinition;
class UMythicStatDefinition;

/**
 * Immutable indexes over an already-loaded stat semantic closure.
 *
 * This class never loads assets and deliberately owns no package paths. Its owning subsystem must retain the
 * streamable handles/strong UObject references for the lifetime of a published registry. Build is transactional:
 * invalid input leaves this object empty rather than exposing a partial map.
 */
class MYTHIC_API FMythicStatRegistry {
public:
    bool Build(TConstArrayView<UMythicStatCategoryDefinition*> Categories,
               TConstArrayView<UMythicStatDefinition*> Stats,
               TArray<FText>& OutErrors);

    void Reset();

    bool IsBuilt() const {
        return bBuilt;
    }

    const UMythicStatCategoryDefinition* FindCategory(const FPrimaryAssetId& CategoryId) const;
    const UMythicStatCategoryDefinition* FindCategory(FGameplayTag CategoryTag) const;

    const UMythicStatDefinition* FindStat(const FPrimaryAssetId& StatId) const;
    const UMythicStatDefinition* FindStat(FGameplayTag StatTag) const;
    const UMythicStatDefinition* FindStat(const FGameplayAttribute& Attribute) const;

    void GetAllCategories(TArray<const UMythicStatCategoryDefinition*>& OutCategories) const;
    void GetAllStatDefinitions(TArray<const UMythicStatDefinition*>& OutStats) const;

private:
    TMap<FPrimaryAssetId, const UMythicStatCategoryDefinition*> CategoriesById;
    TMap<FGameplayTag, const UMythicStatCategoryDefinition*> CategoriesByTag;
    TArray<const UMythicStatCategoryDefinition*> OrderedCategories;

    TMap<FPrimaryAssetId, const UMythicStatDefinition*> StatsById;
    TMap<FGameplayTag, const UMythicStatDefinition*> StatsByTag;
    TMap<FGameplayAttribute, const UMythicStatDefinition*> StatsByAttribute;
    TArray<const UMythicStatDefinition*> OrderedStats;

    bool bBuilt = false;
};
