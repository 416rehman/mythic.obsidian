
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "Engine/Blueprint.h"

#include "UI/MythicHUDLayout.h"
#include "UI/Menu/MythicMenuShell.h"

namespace {
/**
 * The CDO of every Blueprint under /Game/Mythic deriving from T, so authored defaults are what gets checked.
 *
 * Scoped to the project's own content on purpose. Loading every Blueprint in the project drags in plugin and
 * demo assets, and any error they log lands inside this test's window and is reported as this test failing.
 */
template <typename T>
void GatherAuthoredDefaults(TArray<const T *> &Out) {
    FAssetRegistryModule &Module = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
    IAssetRegistry &Registry = Module.Get();

    FARFilter Filter;
    Filter.ClassPaths.Add(UBlueprint::StaticClass()->GetClassPathName());
    Filter.PackagePaths.Add(TEXT("/Game/Mythic"));
    Filter.bRecursivePaths = true;
    // Widget blueprints derive from UBlueprint rather than being it, and every asset here is one.
    Filter.bRecursiveClasses = true;

    TArray<FAssetData> Blueprints;
    Registry.GetAssets(Filter, Blueprints);
    for (const FAssetData &Asset : Blueprints) {
        // Ask the registry which native parent this Blueprint has before loading it, so only candidates pay
        // the load.
        const FString ParentPath = Asset.GetTagValueRef<FString>(FBlueprintTags::NativeParentClassPath);
        if (!ParentPath.IsEmpty() && !ParentPath.Contains(T::StaticClass()->GetName())) {
            continue;
        }
        const UBlueprint *BP = Cast<UBlueprint>(Asset.GetAsset());
        if (!BP || !BP->GeneratedClass || !BP->GeneratedClass->IsChildOf(T::StaticClass())) {
            continue;
        }
        if (const T *Defaults = Cast<T>(BP->GeneratedClass->GetDefaultObject())) {
            Out.Add(Defaults);
        }
    }
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMenuRouteTest,
    "Mythic.Content.MenuRoutes",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMenuRouteTest::RunTest(const FString &Parameters) {
    TArray<const UMythicMenuShell *> Shells;
    GatherAuthoredDefaults(Shells);
    if (!TestTrue(TEXT("the project has a menu shell to check - an empty scan would pass for the wrong reason"),
                  Shells.Num() > 0)) {
        return false;
    }

    TSet<FName> KnownPages;
    int32 TotalPages = 0;
    for (const UMythicMenuShell *Shell : Shells) {
        TSet<FName> SeenHere;
        for (const FMythicMenuPage &Page : Shell->GetPages()) {
            ++TotalPages;
            const FString Where = FString::Printf(TEXT("%s page '%s'"), *Shell->GetName(), *Page.PageId.ToString());

            TestFalse(*FString::Printf(TEXT("%s has an id"), *Where), Page.PageId.IsNone());

            // Two pages sharing an id means one of them is unreachable, and which one is an ordering accident.
            TestFalse(*FString::Printf(TEXT("%s id is not a duplicate"), *Where), SeenHere.Contains(Page.PageId));
            SeenHere.Add(Page.PageId);
            KnownPages.Add(Page.PageId);

            // A tab reading one word while its id says another is the defect that started #127: the strip said
            // Sockets and the page id said Runes, so the page opened was never the page named.
            if (!Page.TabLabel.IsEmpty() && !Page.PageId.IsNone()) {
                const FString Label = Page.TabLabel.ToString().Replace(TEXT(" "), TEXT(""));
                TestTrue(*FString::Printf(TEXT("%s label '%s' names its id"), *Where, *Page.TabLabel.ToString()),
                         Label.Equals(Page.PageId.ToString(), ESearchCase::IgnoreCase));
            }
        }
    }

    TArray<const UMythicHUDLayout *> Layouts;
    GatherAuthoredDefaults(Layouts);
    if (!TestTrue(TEXT("the project has a HUD layout to check"), Layouts.Num() > 0)) {
        return false;
    }

    int32 TotalHotkeys = 0;
    for (const UMythicHUDLayout *Layout : Layouts) {
        for (const FMythicMenuHotkey &Hotkey : Layout->GetMenuHotkeys()) {
            ++TotalHotkeys;
            const FString Where = FString::Printf(TEXT("%s hotkey '%s'"), *Layout->GetName(),
                                                  *Hotkey.ActionTag.ToString());

            TestTrue(*FString::Printf(TEXT("%s is bound to an action"), *Where), Hotkey.ActionTag.IsValid());

            // OpenPage falls back to the first tab for an unknown id, so a mis-wired hotkey opens the wrong
            // screen rather than failing. Nothing at runtime can tell the player it happened.
            TestTrue(*FString::Printf(TEXT("%s opens a page that exists ('%s')"), *Where, *Hotkey.PageId.ToString()),
                     KnownPages.Contains(Hotkey.PageId));
        }
    }

    AddInfo(FString::Printf(TEXT("shells: %d, pages: %d, layouts: %d, hotkeys: %d"),
                            Shells.Num(), TotalPages, Layouts.Num(), TotalHotkeys));
    TestTrue(TEXT("the shell has pages"), TotalPages > 0);
    return true;
}
