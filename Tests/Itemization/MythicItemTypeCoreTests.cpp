#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Misc/AutomationTest.h"

#include "Itemization/Affixes/MythicAffixCatalogue.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace {
    struct FExpectedCore {
        const TCHAR *ItemType;
        const TCHAR *Attribute;
        float Min;
        float Max;
    };

    // What "it is a sword" should be enough to guarantee, without the item restating it.
    const FExpectedCore ExpectedCores[] = {
        {TEXT("Itemization.Type.Equipment.Weapon.Sword"), TEXT("BonusSwordDamage"), 0.10f, 0.25f},
        {TEXT("Itemization.Type.Equipment.Weapon.Axe"), TEXT("BonusAxeDamage"), 0.10f, 0.25f},
        {TEXT("Itemization.Type.Equipment.Weapon.Daggers"), TEXT("BonusDaggerDamage"), 0.10f, 0.25f},
        {TEXT("Itemization.Type.Equipment.Weapon.Sickle"), TEXT("BonusSickleDamage"), 0.10f, 0.25f},
        {TEXT("Itemization.Type.Equipment.Weapon.Spear"), TEXT("BonusSpearDamage"), 0.10f, 0.25f},
        {TEXT("Itemization.Type.Equipment.Weapon.Hammer"), TEXT("BonusHammerDamage"), 0.10f, 0.25f},
        {TEXT("Itemization.Type.Equipment.Gear.Head"), TEXT("Armor"), 8.0f, 18.0f},
        {TEXT("Itemization.Type.Equipment.Gear.Chest"), TEXT("Armor"), 14.0f, 30.0f},
        {TEXT("Itemization.Type.Equipment.Gear.Legs"), TEXT("Armor"), 10.0f, 22.0f},
    };

    UMythicAffixCatalogue *LoadSharedCatalogue() {
        FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
        Module.Get().SearchAllAssets(true);
        TArray<FAssetData> Assets;
        Module.Get().GetAssetsByClass(UMythicAffixCatalogue::StaticClass()->GetClassPathName(), Assets, true);
        for (const FAssetData &Asset : Assets) {
            if (Asset.AssetName == FName(TEXT("DA_AffixCatalogue"))) {
                return Cast<UMythicAffixCatalogue>(Asset.GetAsset());
            }
        }
        return nullptr;
    }
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicItemTypeCoreTest,
                                 "Mythic.Itemization.ItemTypeCore",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

/**
 * An item's type is enough to know its core signature.
 *
 * Every weapon used to restate its own family damage and every armour piece its own Armor band, which meant the
 * same number lived in two places and drifted the moment one was edited. The armour bands differ by slot on
 * purpose - a chest is 14-30 where a helm is 8-18 - so the rules carry the slot, not one flattened band.
 *
 * The chain resolves deepest-first and the roll dedups on first-wins, so the slot rule beats the broad
 * Equipment.Gear rule while that broad rule still covers any slot nobody wrote a rule for.
 */
bool FMythicItemTypeCoreTest::RunTest(const FString &Parameters) {
    UMythicAffixCatalogue *Catalogue = LoadSharedCatalogue();
    if (!TestNotNull(TEXT("the shared affix catalogue loads"), Catalogue)) {
        return false;
    }

    int32 Checked = 0;
    for (const FExpectedCore &Want : ExpectedCores) {
        const FGameplayTag Type = FGameplayTag::RequestGameplayTag(FName(Want.ItemType), false);
        if (!TestTrue(*FString::Printf(TEXT("%s is a registered tag"), Want.ItemType), Type.IsValid())) {
            continue;
        }

        TArray<FMythicTieredAffixDef> Defs;
        Catalogue->BuildCoreDefs(Type, FGameplayTagContainer(), Defs);

        const FMythicTieredAffixDef *Found = Defs.FindByPredicate([&Want](const FMythicTieredAffixDef &Def) {
            return Def.Attribute.GetName() == Want.Attribute;
        });

        if (!TestNotNull(*FString::Printf(TEXT("%s guarantees %s without the item authoring it"),
                                          Want.ItemType, Want.Attribute), Found)) {
            continue;
        }
        ++Checked;

        // The slot band is the whole reason these are separate entries; a generic band here means slot identity
        // was flattened and every piece silently rebalanced.
        if (TestTrue(*FString::Printf(TEXT("%s's %s carries a tier"), Want.ItemType, Want.Attribute),
                     Found->Tiers.Num() > 0)) {
            TestEqual(*FString::Printf(TEXT("%s's %s keeps its authored floor"), Want.ItemType, Want.Attribute),
                      Found->Tiers[0].Min, Want.Min);
            TestEqual(*FString::Printf(TEXT("%s's %s keeps its authored ceiling"), Want.ItemType, Want.Attribute),
                      Found->Tiers[0].Max, Want.Max);
        }
    }

    AddInfo(FString::Printf(TEXT("%d of %d item types resolved a core signature from their type alone"),
                            Checked, UE_ARRAY_COUNT(ExpectedCores)));
    TestEqual(TEXT("every listed item type resolved its signature"), Checked, (int32)UE_ARRAY_COUNT(ExpectedCores));

    // Slots differing by band is the property that makes this worth doing; if they collapse to one number the
    // rules stopped carrying the slot.
    TArray<FMythicTieredAffixDef> HeadDefs;
    TArray<FMythicTieredAffixDef> ChestDefs;
    Catalogue->BuildCoreDefs(FGameplayTag::RequestGameplayTag(FName(TEXT("Itemization.Type.Equipment.Gear.Head")), false),
                             FGameplayTagContainer(), HeadDefs);
    Catalogue->BuildCoreDefs(FGameplayTag::RequestGameplayTag(FName(TEXT("Itemization.Type.Equipment.Gear.Chest")), false),
                             FGameplayTagContainer(), ChestDefs);
    const FMythicTieredAffixDef *Head = HeadDefs.FindByPredicate([](const FMythicTieredAffixDef &D) { return D.Attribute.GetName() == TEXT("Armor"); });
    const FMythicTieredAffixDef *Chest = ChestDefs.FindByPredicate([](const FMythicTieredAffixDef &D) { return D.Attribute.GetName() == TEXT("Armor"); });
    if (Head && Chest && Head->Tiers.Num() > 0 && Chest->Tiers.Num() > 0) {
        TestTrue(TEXT("a chest still armours more than a helm"), Chest->Tiers[0].Max > Head->Tiers[0].Max);
    }

    return true;
}

#endif
