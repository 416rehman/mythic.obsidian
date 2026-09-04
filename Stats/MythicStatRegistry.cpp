// Copyright Stellar Games. All Rights Reserved.

#include "Stats/MythicStatRegistry.h"

#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"

namespace {
FText AssetError(const UObject* Asset, const FText& Error) {
    return FText::Format(NSLOCTEXT("MythicStatRegistry", "AssetError", "{0}: {1}"),
                         FText::FromString(GetNameSafe(Asset)), Error);
}

FText DuplicateError(const TCHAR* Kind, const FString& Identity, const UObject* First, const UObject* Second) {
    return FText::Format(
        NSLOCTEXT("MythicStatRegistry", "DuplicateIdentity", "Duplicate {0} '{1}' on {2} and {3}."),
        FText::FromString(Kind), FText::FromString(Identity), FText::FromString(GetNameSafe(First)),
        FText::FromString(GetNameSafe(Second)));
}
}

void FMythicStatRegistry::Reset() {
    CategoriesById.Reset();
    CategoriesByTag.Reset();
    OrderedCategories.Reset();
    StatsById.Reset();
    StatsByTag.Reset();
    StatsByAttribute.Reset();
    StatsByCategory.Reset();
    OrderedStats.Reset();
    bBuilt = false;
}

bool FMythicStatRegistry::Build(TConstArrayView<UMythicStatCategoryDefinition*> Categories,
                                TConstArrayView<UMythicStatDefinition*> Stats,
                                TArray<FText>& OutErrors) {
    Reset();
    const int32 InitialErrorCount = OutErrors.Num();

    TMap<FPrimaryAssetId, const UMythicStatCategoryDefinition*> CandidateCategoriesById;
    TMap<FGameplayTag, const UMythicStatCategoryDefinition*> CandidateCategoriesByTag;
    TMap<int32, const UMythicStatCategoryDefinition*> CategoryBySheetOrder;
    TArray<const UMythicStatCategoryDefinition*> CandidateOrderedCategories;

    for (const UMythicStatCategoryDefinition* Category : Categories) {
        if (!Category) {
            OutErrors.Add(NSLOCTEXT("MythicStatRegistry", "NullCategory", "The category closure contains a null asset."));
            continue;
        }

        TArray<FText> LocalErrors;
        Category->AppendValidationErrors(LocalErrors);
        for (const FText& Error : LocalErrors) {
            OutErrors.Add(AssetError(Category, Error));
        }

        const FPrimaryAssetId CategoryId = Category->GetPrimaryAssetId();
        if (const UMythicStatCategoryDefinition* const* Existing = CandidateCategoriesById.Find(CategoryId)) {
            OutErrors.Add(DuplicateError(TEXT("category primary ID"), CategoryId.ToString(), *Existing, Category));
        }
        else if (CategoryId.IsValid()) {
            CandidateCategoriesById.Add(CategoryId, Category);
        }

        if (const UMythicStatCategoryDefinition* const* Existing = CandidateCategoriesByTag.Find(Category->CategoryTag)) {
            OutErrors.Add(DuplicateError(TEXT("category tag"), Category->CategoryTag.ToString(), *Existing, Category));
        }
        else if (Category->CategoryTag.IsValid()) {
            CandidateCategoriesByTag.Add(Category->CategoryTag, Category);
        }

        if (const UMythicStatCategoryDefinition* const* Existing = CategoryBySheetOrder.Find(Category->SheetOrder)) {
            OutErrors.Add(DuplicateError(TEXT("category SheetOrder"), FString::FromInt(Category->SheetOrder),
                                         *Existing, Category));
        }
        else {
            CategoryBySheetOrder.Add(Category->SheetOrder, Category);
        }

        CandidateOrderedCategories.Add(Category);
    }

    TMap<FPrimaryAssetId, const UMythicStatDefinition*> CandidateStatsById;
    TMap<FGameplayTag, const UMythicStatDefinition*> CandidateStatsByTag;
    TMap<FGameplayAttribute, const UMythicStatDefinition*> CandidateStatsByAttribute;
    TArray<const UMythicStatDefinition*> CandidateOrderedStats;

    for (const UMythicStatDefinition* Stat : Stats) {
        if (!Stat) {
            OutErrors.Add(NSLOCTEXT("MythicStatRegistry", "NullStat", "The stat closure contains a null asset."));
            continue;
        }

        TArray<FText> LocalErrors;
        Stat->AppendValidationErrors(LocalErrors);
        for (const FText& Error : LocalErrors) {
            OutErrors.Add(AssetError(Stat, Error));
        }

        const FPrimaryAssetId StatId = Stat->GetPrimaryAssetId();
        if (const UMythicStatDefinition* const* Existing = CandidateStatsById.Find(StatId)) {
            OutErrors.Add(DuplicateError(TEXT("stat primary ID"), StatId.ToString(), *Existing, Stat));
        }
        else if (StatId.IsValid()) {
            CandidateStatsById.Add(StatId, Stat);
        }

        if (const UMythicStatDefinition* const* Existing = CandidateStatsByTag.Find(Stat->StatTag)) {
            OutErrors.Add(DuplicateError(TEXT("stat tag"), Stat->StatTag.ToString(), *Existing, Stat));
        }
        else if (Stat->StatTag.IsValid()) {
            CandidateStatsByTag.Add(Stat->StatTag, Stat);
        }

        if (const UMythicStatDefinition* const* Existing = CandidateStatsByAttribute.Find(Stat->Attribute)) {
            OutErrors.Add(DuplicateError(TEXT("GAS attribute"), Stat->Attribute.GetName(), *Existing, Stat));
        }
        else if (Stat->Attribute.IsValid()) {
            CandidateStatsByAttribute.Add(Stat->Attribute, Stat);
        }

        const FPrimaryAssetId CategoryId = Stat->Category.GetPrimaryAssetId();
        if (!CandidateCategoriesById.Contains(CategoryId)) {
            OutErrors.Add(AssetError(
                Stat,
                FText::Format(NSLOCTEXT("MythicStatRegistry", "MissingCategory", "Category '{0}' is not in the loaded semantic closure."),
                              FText::FromString(CategoryId.ToString()))));
        }

        CandidateOrderedStats.Add(Stat);
    }

    for (const UMythicStatDefinition* Stat : CandidateOrderedStats) {
        if (!Stat || Stat->PairRole == EMythicStatPairRole::None || !Stat->PairedStat.IsValid()) {
            continue;
        }

        const FPrimaryAssetId PairedStatId = Stat->PairedStat.GetPrimaryAssetId();
        const UMythicStatDefinition* const* PairPtr = CandidateStatsById.Find(PairedStatId);
        if (!PairPtr || !*PairPtr) {
            OutErrors.Add(AssetError(
                Stat,
                FText::Format(NSLOCTEXT("MythicStatRegistry", "MissingPair", "PairedStat '{0}' is not in the loaded semantic closure."),
                              FText::FromString(PairedStatId.ToString()))));
            continue;
        }

        const UMythicStatDefinition* Pair = *PairPtr;
        if (Pair->PairedStat.GetPrimaryAssetId() != Stat->GetPrimaryAssetId()) {
            OutErrors.Add(AssetError(Stat, NSLOCTEXT("MythicStatRegistry", "NonReciprocalPair", "PairedStat must link back to this stat.")));
        }

        const bool bComplementary =
            (Stat->PairRole == EMythicStatPairRole::Current && Pair->PairRole == EMythicStatPairRole::Capacity)
            || (Stat->PairRole == EMythicStatPairRole::Capacity && Pair->PairRole == EMythicStatPairRole::Current);
        if (!bComplementary) {
            OutErrors.Add(AssetError(Stat, NSLOCTEXT("MythicStatRegistry", "NonComplementaryPair", "Stat pairs must have complementary Current and Capacity roles.")));
        }
    }

    if (OutErrors.Num() != InitialErrorCount) {
        return false;
    }

    CandidateOrderedCategories.Sort([](const UMythicStatCategoryDefinition& Left,
                                       const UMythicStatCategoryDefinition& Right) {
        return Left.SheetOrder != Right.SheetOrder
            ? Left.SheetOrder < Right.SheetOrder
            : Left.CategoryTag.ToString() < Right.CategoryTag.ToString();
    });

    TMap<FPrimaryAssetId, int32> CategoryOrder;
    for (int32 Index = 0; Index < CandidateOrderedCategories.Num(); ++Index) {
        CategoryOrder.Add(CandidateOrderedCategories[Index]->GetPrimaryAssetId(), Index);
    }

    CandidateOrderedStats.Sort([&CategoryOrder](const UMythicStatDefinition& Left,
                                                const UMythicStatDefinition& Right) {
        const int32 LeftCategory = CategoryOrder.FindRef(Left.Category.GetPrimaryAssetId());
        const int32 RightCategory = CategoryOrder.FindRef(Right.Category.GetPrimaryAssetId());
        if (LeftCategory != RightCategory) {
            return LeftCategory < RightCategory;
        }
        if (Left.SheetOrder != Right.SheetOrder) {
            return Left.SheetOrder < Right.SheetOrder;
        }
        return Left.StatTag.ToString() < Right.StatTag.ToString();
    });

    CategoriesById = MoveTemp(CandidateCategoriesById);
    CategoriesByTag = MoveTemp(CandidateCategoriesByTag);
    OrderedCategories = MoveTemp(CandidateOrderedCategories);
    StatsById = MoveTemp(CandidateStatsById);
    StatsByTag = MoveTemp(CandidateStatsByTag);
    StatsByAttribute = MoveTemp(CandidateStatsByAttribute);
    OrderedStats = MoveTemp(CandidateOrderedStats);

    // OrderedStats is already sorted category-major, so bucketing preserves each category's sheet order.
    for (const UMythicStatDefinition* Stat : OrderedStats) {
        StatsByCategory.FindOrAdd(Stat->Category.GetPrimaryAssetId()).Add(Stat);
    }

    bBuilt = true;
    return true;
}

const UMythicStatCategoryDefinition* FMythicStatRegistry::FindCategory(const FPrimaryAssetId& CategoryId) const {
    const UMythicStatCategoryDefinition* const* Found = CategoriesById.Find(CategoryId);
    return Found ? *Found : nullptr;
}

const UMythicStatCategoryDefinition* FMythicStatRegistry::FindCategory(FGameplayTag CategoryTag) const {
    const UMythicStatCategoryDefinition* const* Found = CategoriesByTag.Find(CategoryTag);
    return Found ? *Found : nullptr;
}

const UMythicStatDefinition* FMythicStatRegistry::FindStat(const FPrimaryAssetId& StatId) const {
    const UMythicStatDefinition* const* Found = StatsById.Find(StatId);
    return Found ? *Found : nullptr;
}

const UMythicStatDefinition* FMythicStatRegistry::FindStat(FGameplayTag StatTag) const {
    const UMythicStatDefinition* const* Found = StatsByTag.Find(StatTag);
    return Found ? *Found : nullptr;
}

const UMythicStatDefinition* FMythicStatRegistry::FindStat(const FGameplayAttribute& Attribute) const {
    const UMythicStatDefinition* const* Found = StatsByAttribute.Find(Attribute);
    return Found ? *Found : nullptr;
}

void FMythicStatRegistry::GetAllCategories(TArray<const UMythicStatCategoryDefinition*>& OutCategories) const {
    OutCategories = OrderedCategories;
}

void FMythicStatRegistry::GetAllStatDefinitions(TArray<const UMythicStatDefinition*>& OutStats) const {
    OutStats = OrderedStats;
}

TConstArrayView<const UMythicStatDefinition*> FMythicStatRegistry::GetStatsInCategory(
    const FPrimaryAssetId& CategoryId) const {
    const TArray<const UMythicStatDefinition*>* Found = StatsByCategory.Find(CategoryId);
    return Found ? TConstArrayView<const UMythicStatDefinition*>(*Found) : TConstArrayView<const UMythicStatDefinition*>();
}
