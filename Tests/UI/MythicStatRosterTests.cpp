#include "AttributeSet.h"
#include "Engine/AssetManager.h"
#include "Misc/AutomationTest.h"
#include "UObject/UObjectIterator.h"

#include "Stats/MythicStatCategoryDefinition.h"
#include "Stats/MythicStatDefinition.h"
#include "Stats/MythicStatRegistry.h"
#include "System/MythicAssetManager.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {
void CollectMythicAttributes(TArray<FGameplayAttribute>& Out) {
    for (TObjectIterator<UClass> It; It; ++It) {
        UClass* Class = *It;
        if (!Class->IsChildOf(UAttributeSet::StaticClass()) || Class->HasAnyClassFlags(CLASS_Abstract)
            || !Class->GetName().StartsWith(TEXT("MythicAttributeSet"))) {
            continue;
        }
        for (TFieldIterator<FProperty> Prop(Class, EFieldIteratorFlags::ExcludeSuper); Prop; ++Prop) {
            const FStructProperty* Struct = CastField<FStructProperty>(*Prop);
            if (Struct && Struct->Struct == FGameplayAttributeData::StaticStruct()) {
                Out.Add(FGameplayAttribute(*Prop));
            }
        }
    }
}

template <typename AssetType>
void LoadPrimaryAssetsForTest(FPrimaryAssetType Type, TArray<AssetType*>& Out) {
    UAssetManager& AssetManager = UAssetManager::Get();
    TArray<FPrimaryAssetId> Ids;
    AssetManager.GetPrimaryAssetIdList(Type, Ids);
    Ids.Sort([](const FPrimaryAssetId& Left, const FPrimaryAssetId& Right) {
        return Left.ToString() < Right.ToString();
    });
    for (const FPrimaryAssetId& Id : Ids) {
        if (AssetType* Asset = Cast<AssetType>(AssetManager.GetPrimaryAssetPath(Id).TryLoad())) {
            Out.Add(Asset);
        }
    }
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicStatRosterTest,
                                 "Mythic.UI.StatRoster",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FMythicStatRosterTest::RunTest(const FString& Parameters) {
    TArray<UMythicStatCategoryDefinition*> Categories;
    TArray<UMythicStatDefinition*> Stats;
    LoadPrimaryAssetsForTest(UMythicAssetManager::StatCategoryDefinitionType, Categories);
    LoadPrimaryAssetsForTest(UMythicAssetManager::StatDefinitionType, Stats);

    TestEqual(TEXT("the migrated semantic category roster is exact"), Categories.Num(), 7);
    TestEqual(TEXT("105 authored rows plus 13 real capacity properties are canonical assets"), Stats.Num(), 118);

    FMythicStatRegistry Registry;
    TArray<FText> Errors;
    if (!Registry.Build(Categories, Stats, Errors)) {
        for (const FText& Error : Errors) {
            AddError(Error.ToString());
        }
        return false;
    }

    const FGameplayTag ProficiencyCategoryTag =
        FGameplayTag::RequestGameplayTag(TEXT("Stat.Category.Proficiency"));
    const UMythicStatCategoryDefinition* ProficiencyCategory =
        Registry.FindCategory(ProficiencyCategoryTag);
    if (TestNotNull(TEXT("the progression-only proficiency category remains canonical"),
                    ProficiencyCategory)) {
        int32 ProficiencyStatCount = 0;
        TArray<FString> VisibleProficiencyStats;
        for (const UMythicStatDefinition* Stat : Stats) {
            if (!Stat || Stat->Category.GetPrimaryAssetId()
                    != ProficiencyCategory->GetPrimaryAssetId()) {
                continue;
            }
            ++ProficiencyStatCount;
            if (Stat->SheetVisibility != EMythicStatSheetVisibility::Hidden) {
                VisibleProficiencyStats.Add(Stat->DeveloperName.ToString());
            }
        }
        VisibleProficiencyStats.Sort();
        TestEqual(TEXT("all proficiency and overall progression current/capacity definitions are covered"),
                  ProficiencyStatCount, 26);
        if (!VisibleProficiencyStats.IsEmpty()) {
            AddError(FString::Printf(
                TEXT("Proficiency progression definitions must not appear on the character stat sheet: %s"),
                *FString::Join(VisibleProficiencyStats, TEXT(", "))));
        }
    }

    TArray<FGameplayAttribute> Attributes;
    CollectMythicAttributes(Attributes);
    if (!TestTrue(TEXT("the GAS sweep found the real roster"), Attributes.Num() > 50)) {
        return false;
    }

    TArray<FString> Missing;
    for (const FGameplayAttribute& Attribute : Attributes) {
        if (!Registry.FindStat(Attribute)) {
            Missing.Add(Attribute.GetName());
        }
    }
    Missing.Sort();
    if (!Missing.IsEmpty()) {
        AddError(FString::Printf(TEXT("%d GAS attributes have no StatDefinition: %s"),
                                 Missing.Num(), *FString::Join(Missing, TEXT(", "))));
    }

    TestNull(TEXT("an invalid attribute cannot acquire invented semantics"), Registry.FindStat(FGameplayAttribute()));
    AddInfo(FString::Printf(TEXT("%d GAS attributes resolved through %d canonical StatDefinitions."),
                            Attributes.Num(), Stats.Num()));
    return Missing.IsEmpty();
}

#endif
