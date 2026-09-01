#include "Misc/AutomationTest.h"

#if WITH_EDITOR

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Itemization/Inventory/Fragments/Actionable/AttackFragment.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Itemization/MythicTags_Inventory.h"
#include "Misc/DataValidation.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemDefinitionFragmentValidationTest,
    "Mythic.Itemization.ItemDefinition.FragmentValidation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemDefinitionFragmentValidationTest::RunTest(const FString &Parameters) {
    UItemDefinition *Definition = NewObject<UItemDefinition>(GetTransientPackage());
    if (!TestNotNull(TEXT("a transient Item Definition can be created"), Definition)) {
        return false;
    }
    Definition->ItemType = ITEMIZATION_TYPE_MISC;

    UAttackFragment *InvalidAttack = NewObject<UAttackFragment>(Definition);
    if (!TestNotNull(TEXT("a transient Attack Fragment can be created"), InvalidAttack)) {
        return false;
    }
    Definition->Fragments.Add(InvalidAttack);

    FDataValidationContext Context;
    TestTrue(TEXT("an invalid nested fragment invalidates its Item Definition"),
             Definition->IsDataValid(Context) == EDataValidationResult::Invalid);

    bool bFoundIndexedFragmentDiagnostic = false;
    for (const FDataValidationContext::FIssue &Issue : Context.GetIssues()) {
        const FString Message = Issue.Message.ToString();
        bFoundIndexedFragmentDiagnostic |= Message.Contains(TEXT("Fragment 0"))
            && Message.Contains(TEXT("AttackFragment"))
            && Message.Contains(TEXT("is invalid"));
    }
    TestTrue(TEXT("the diagnostic identifies the fragment index, type, and nested validation failure"),
             bFoundIndexedFragmentDiagnostic);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicItemDefinitionExactWeaponClassValidationTest,
    "Mythic.Itemization.ItemDefinition.ExactWeaponClassValidation",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicItemDefinitionExactWeaponClassValidationTest::RunTest(
    const FString &Parameters) {
    UItemDefinition *Definition =
        NewObject<UItemDefinition>(GetTransientPackage());
    if (!TestNotNull(TEXT("a transient weapon definition can be created"),
                     Definition)) {
        return false;
    }

    Definition->ItemType = ITEMIZATION_TYPE_EQUIPMENT_WEAPON;
    FDataValidationContext ParentOnlyContext;
    TestEqual(TEXT("the generic Weapon parent is invalid content"),
              Definition->IsDataValid(ParentOnlyContext),
              EDataValidationResult::Invalid);

    bool bFoundExactClassDiagnostic = false;
    for (const FDataValidationContext::FIssue &Issue :
         ParentOnlyContext.GetIssues()) {
        bFoundExactClassDiagnostic |= Issue.Message.ToString().Contains(
            TEXT("not an exact supported weapon class"));
    }
    TestTrue(TEXT("the diagnostic tells the designer to choose an exact class"),
             bFoundExactClassDiagnostic);

    Definition->ItemType = ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE;
    FDataValidationContext ConcreteContext;
    Definition->IsDataValid(ConcreteContext);
    for (const FDataValidationContext::FIssue &Issue :
         ConcreteContext.GetIssues()) {
        TestFalse(
            TEXT("an exact Axe class does not emit the weapon-class diagnostic"),
            Issue.Message.ToString().Contains(
                TEXT("not an exact supported weapon class")));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicShippedWeaponDefinitionValidationTest,
    "Mythic.Itemization.ItemDefinition.ShippedWeaponDefinitions",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicShippedWeaponDefinitionValidationTest::RunTest(
    const FString &Parameters) {
    IAssetRegistry &Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(
            TEXT("AssetRegistry")).Get();
    FARFilter Filter;
    Filter.PackagePaths.Add(
        TEXT("/Game/Mythic/Itemization/ItemDefinitions/Weapons"));
    Filter.ClassPaths.Add(UItemDefinition::StaticClass()->GetClassPathName());
    Filter.bRecursivePaths = true;
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> WeaponAssets;
    Registry.GetAssets(Filter, WeaponAssets);
    if (!TestTrue(TEXT("the project has shipped weapon definitions"),
                  !WeaponAssets.IsEmpty())) {
        return false;
    }

    for (const FAssetData &AssetData : WeaponAssets) {
        const UItemDefinition *Definition =
            Cast<UItemDefinition>(AssetData.GetAsset());
        if (!TestNotNull(
                *FString::Printf(TEXT("%s loads"),
                                 *AssetData.PackageName.ToString()),
                Definition)) {
            continue;
        }

        FDataValidationContext Context;
        const EDataValidationResult Result = Definition->IsDataValid(Context);
        for (const FDataValidationContext::FIssue &Issue : Context.GetIssues()) {
            AddError(FString::Printf(TEXT("%s: %s"),
                                     *AssetData.PackageName.ToString(),
                                     *Issue.Message.ToString()));
        }
        TestTrue(
            *FString::Printf(TEXT("%s is valid shipped weapon content"),
                             *AssetData.PackageName.ToString()),
            Result != EDataValidationResult::Invalid);
    }

    const UItemDefinition *StarterAxe = LoadObject<UItemDefinition>(
        nullptr,
        TEXT("/Game/Mythic/Itemization/ItemDefinitions/Weapons/Axe/Axe.Axe"));
    const UItemDefinition *TrainingBlade = LoadObject<UItemDefinition>(
        nullptr,
        TEXT("/Game/Mythic/Itemization/ItemDefinitions/Weapons/JohnsWeapon.JohnsWeapon"));
    TestTrue(TEXT("the starter Axe is classified as an Axe"),
             StarterAxe
                 && StarterAxe->ItemType
                        == ITEMIZATION_TYPE_EQUIPMENT_WEAPON_AXE);
    TestTrue(TEXT("the Training Blade is classified as a Sword"),
             TrainingBlade
                 && TrainingBlade->ItemType
                        == ITEMIZATION_TYPE_EQUIPMENT_WEAPON_SWORD);
    return true;
}

#endif // WITH_EDITOR
