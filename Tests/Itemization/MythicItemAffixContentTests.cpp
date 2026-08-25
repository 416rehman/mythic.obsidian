#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AutomationTest.h"

#include "Itemization/Affixes/MythicAffixCatalogue.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicItemAffixContentTest,
                                 "Mythic.Itemization.ItemAffixContent",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * What items author about their own affixes, checked against what still exists.
 *
 * An FGameplayAttribute stored as a map key resolves by name on load. Delete the attribute and the key does not
 * error - it comes back invalid, the affix silently never rolls, and the item quietly loses a stat. Nothing in a
 * build or a rules test can see it, because the damage is entirely in content.
 */
bool FMythicItemAffixContentTest::RunTest(const FString &Parameters) {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();
    Registry.SearchAllAssets(true);

    TArray<FAssetData> ItemAssets;
    Registry.GetAssetsByClass(UItemDefinition::StaticClass()->GetClassPathName(), ItemAssets, true);
    if (!TestTrue(TEXT("the project has item definitions to check"), ItemAssets.Num() > 0)) {
        return false;
    }

    int32 WithFragment = 0;
    int32 CoreKeys = 0;
    int32 PoolKeys = 0;
    int32 Dangling = 0;

    for (const FAssetData &Asset : ItemAssets) {
        const UItemDefinition *Item = Cast<UItemDefinition>(Asset.GetAsset());
        if (!Item) {
            continue;
        }
        const FString Name = Asset.AssetName.ToString();

        for (const TObjectPtr<UItemFragment> &Fragment : Item->Fragments) {
            const UAffixesFragment *Affixes = Cast<UAffixesFragment>(Fragment);
            if (!Affixes) {
                continue;
            }
            ++WithFragment;

            auto CheckMap = [&](const TMap<FGameplayAttribute, FRollDefinition> &Map, const TCHAR *Which, int32 &Counter) {
                for (const TPair<FGameplayAttribute, FRollDefinition> &Pair : Map) {
                    ++Counter;
                    if (!Pair.Key.IsValid()) {
                        ++Dangling;
                        AddError(FString::Printf(
                            TEXT("%s authors a %s affix on an attribute that no longer exists ('%s'), so it silently never rolls"),
                            *Name, Which, *Pair.Key.GetName()));
                    }
                }
            };

            CheckMap(Affixes->AffixesBuildData.CoreAffixes, TEXT("core"), CoreKeys);
            CheckMap(Affixes->AffixesBuildData.AffixPoolMap, TEXT("pool"), PoolKeys);
        }
    }

    AddInfo(FString::Printf(TEXT("%d items, %d with an affixes fragment, %d core keys and %d pool keys checked, %d dangling"),
                            ItemAssets.Num(), WithFragment, CoreKeys, PoolKeys, Dangling));

    // A sweep that inspected nothing passes everything above, which is the same shape as the bug it guards.
    TestTrue(TEXT("items carrying an affixes fragment were found"), WithFragment > 0);
    TestTrue(TEXT("authored affix keys were found to check"), CoreKeys > 0);

    return Dangling == 0;
}

#endif
