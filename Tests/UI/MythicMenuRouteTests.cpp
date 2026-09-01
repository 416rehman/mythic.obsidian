
#include "Misc/AutomationTest.h"

#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/IAssetRegistry.h"
#include "CommonActivatableWidget.h"
#include "EnhancedActionKeyMapping.h"
#include "Engine/Blueprint.h"
#include "InputMappingContext.h"
#include "InputCoreTypes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/UnrealType.h"

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

bool IsAuthoredBackHandler(const UCommonActivatableWidget *Widget) {
    static const FBoolProperty *BackHandlerProperty =
        FindFProperty<FBoolProperty>(UCommonActivatableWidget::StaticClass(), TEXT("bIsBackHandler"));
    return Widget && BackHandlerProperty
        && BackHandlerProperty->GetPropertyValue_InContainer(Widget);
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

    int32 TotalPages = 0;
    for (const UMythicMenuShell *Shell : Shells) {
        TestTrue(*FString::Printf(TEXT("%s owns the pushed screen Back action"), *Shell->GetName()),
                 IsAuthoredBackHandler(Shell));
        TSet<FName> SeenHere;
        for (const FMythicMenuPage &Page : Shell->GetPages()) {
            ++TotalPages;
            const FString Where = FString::Printf(TEXT("%s page '%s'"), *Shell->GetName(), *Page.PageId.ToString());

            TestFalse(*FString::Printf(TEXT("%s has an id"), *Where), Page.PageId.IsNone());

            const UCommonActivatableWidget *PageDefaults =
                Page.PageClass ? Page.PageClass->GetDefaultObject<UCommonActivatableWidget>() : nullptr;
            TestNotNull(*FString::Printf(TEXT("%s has loadable defaults"), *Where), PageDefaults);
            TestFalse(
                *FString::Printf(TEXT("%s leaves Back ownership with its pushed menu shell"), *Where),
                IsAuthoredBackHandler(PageDefaults));

            // Two pages sharing an id means one of them is unreachable, and which one is an ordering accident.
            TestFalse(*FString::Printf(TEXT("%s id is not a duplicate"), *Where), SeenHere.Contains(Page.PageId));
            SeenHere.Add(Page.PageId);
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

    int32 ValidMenuEntries = 0;
    for (const UMythicHUDLayout *Layout : Layouts) {
        const FGameplayTag &EntryAction = Layout->GetOpenMenuAction();
        const FString Where = FString::Printf(TEXT("%s shell entry"), *Layout->GetName());
        const bool bIsMenuAction = EntryAction.IsValid()
            && EntryAction.ToString().Equals(TEXT("UI.Action.Menu"), ESearchCase::CaseSensitive);
        TestTrue(*FString::Printf(TEXT("%s uses only UI.Action.Menu (Tab / View)"), *Where), bIsMenuAction);
        ValidMenuEntries += bIsMenuAction ? 1 : 0;
    }

    AddInfo(FString::Printf(TEXT("shells: %d, pages: %d, layouts: %d, valid menu entries: %d"),
                            Shells.Num(), TotalPages, Layouts.Num(), ValidMenuEntries));
    TestTrue(TEXT("the shell has pages"), TotalPages > 0);
    TestEqual(TEXT("every authored HUD has the one shell entry action"), ValidMenuEntries, Layouts.Num());
    return true;
}

namespace {
int32 CountAuthoredMapping(const UInputMappingContext *Context, const FName ActionName, const FKey Key) {
    if (!Context) {
        return 0;
    }

    int32 MatchingMappings = 0;
    for (const FEnhancedActionKeyMapping &Mapping : Context->GetMappings()) {
        if (Mapping.Action && Mapping.Action->GetFName() == ActionName && Mapping.Key == Key) {
            ++MatchingMappings;
        }
    }
    return MatchingMappings;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMenuAndInventoryInputContractTest,
    "Mythic.Content.MenuAndInventoryInputContract",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMenuAndInventoryInputContractTest::RunTest(const FString &Parameters) {
    FString DefaultInput;
    const FString DefaultInputPath = FPaths::Combine(FPaths::ProjectConfigDir(), TEXT("DefaultInput.ini"));
    if (!TestTrue(TEXT("DefaultInput.ini can be read"),
                  FFileHelper::LoadFileToString(DefaultInput, *DefaultInputPath))) {
        return false;
    }

    const TCHAR *ForbiddenDirectPageActions[] = {
        TEXT("UI.Action.Inventory"),
        TEXT("UI.Action.Character"),
        TEXT("UI.Action.Crafting"),
        TEXT("UI.Action.Proficiencies"),
        TEXT("UI.Action.Map"),
        TEXT("UI.Action.Journal"),
        TEXT("UI.Action.Witchcraft"),
        TEXT("UI.Action.Powers"),
        TEXT("UI.Action.Sockets"),
        TEXT("UI.Action.Settings"),
    };
    for (const TCHAR *ActionTag : ForbiddenDirectPageActions) {
        TestFalse(*FString::Printf(TEXT("%s has no direct CommonUI key mapping"), ActionTag),
                  DefaultInput.Contains(FString::Printf(TEXT("ActionTag=%s,"), ActionTag),
                                        ESearchCase::CaseSensitive));
    }
    TestTrue(TEXT("Tab / View is the authored shell entry"),
             DefaultInput.Contains(
                 TEXT("ActionTag=UI.Action.Menu,DefaultDisplayName=NSLOCTEXT"),
                 ESearchCase::CaseSensitive)
             && DefaultInput.Contains(TEXT("KeyMappings=((Key=Tab),(Key=Gamepad_Special_Left))"),
                                      ESearchCase::CaseSensitive));

    const UInputMappingContext *DefaultContext = LoadObject<UInputMappingContext>(
        nullptr, TEXT("/Game/Mythic/Player/Input/IMC_Default.IMC_Default"));
    const UInputMappingContext *CameraContext = LoadObject<UInputMappingContext>(
        nullptr, TEXT("/Game/Mythic/Player/Input/IMC_Camera.IMC_Camera"));
    const UInputMappingContext *InventoryContext = LoadObject<UInputMappingContext>(
        nullptr, TEXT("/Game/Mythic/Player/Input/IMC_CharacterInventory.IMC_CharacterInventory"));
    if (!TestNotNull(TEXT("the gameplay mapping context exists"), DefaultContext)
        || !TestNotNull(TEXT("the camera mapping context exists"), CameraContext)
        || !TestNotNull(TEXT("the character-inventory mapping context exists"), InventoryContext)) {
        return false;
    }

    for (const FEnhancedActionKeyMapping &Mapping : DefaultContext->GetMappings()) {
        TestFalse(TEXT("the gameplay context does not reopen inventory directly"),
                  Mapping.Action && Mapping.Action->GetFName() == TEXT("IA_Inventory"));
        TestFalse(TEXT("C is reserved out of the gameplay context"), Mapping.Key == EKeys::C);
    }
    TestEqual(TEXT("primary attack is mapped once to left mouse"),
              CountAuthoredMapping(DefaultContext, TEXT("IA_PrimaryAction"),
                                   EKeys::LeftMouseButton),
              1);
    TestEqual(TEXT("primary attack is mapped once to the right trigger"),
              CountAuthoredMapping(DefaultContext, TEXT("IA_PrimaryAction"),
                                   EKeys::Gamepad_RightTrigger),
              1);
    TestEqual(TEXT("C changes camera style"),
              CountAuthoredMapping(CameraContext, TEXT("IA_SwitchCamera"), EKeys::C), 1);

    IAssetRegistry &Registry =
        FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry")).Get();
    TArray<FName> LegacyInventoryContextReferencers;
    Registry.GetReferencers(
        TEXT("/Game/Mythic/Player/Input/IMC_Inventory"),
        LegacyInventoryContextReferencers);
    for (const FName Referencer : LegacyInventoryContextReferencers) {
        AddError(FString::Printf(
            TEXT("legacy global IMC_Inventory is still activated or referenced by %s"),
            *Referencer.ToString()));
    }
    TestEqual(TEXT("the retired global inventory mapping context has no referencers"),
              LegacyInventoryContextReferencers.Num(), 0);

    struct FExpectedInventoryMapping {
        FName ActionName;
        FKey Key;
    };
    const FExpectedInventoryMapping ExpectedMappings[] = {
        {TEXT("IA_InventoryPrimary"), EKeys::Enter},
        {TEXT("IA_InventoryPrimary"), EKeys::Gamepad_FaceButton_Bottom},
        {TEXT("IA_InventoryActions"), EKeys::F},
        {TEXT("IA_InventoryActions"), EKeys::Gamepad_FaceButton_Left},
        {TEXT("IA_InventoryCompare"), EKeys::LeftShift},
        {TEXT("IA_InventoryCompare"), EKeys::Gamepad_FaceButton_Top},
        {TEXT("IA_InventoryPreviousCategory"), EKeys::LeftBracket},
        {TEXT("IA_InventoryPreviousCategory"), EKeys::Gamepad_LeftTrigger},
        {TEXT("IA_InventoryNextCategory"), EKeys::RightBracket},
        {TEXT("IA_InventoryNextCategory"), EKeys::Gamepad_RightTrigger},
        {TEXT("IA_InventorySort"), EKeys::R},
        {TEXT("IA_InventorySort"), EKeys::Gamepad_LeftThumbstick},
    };
    TestEqual(TEXT("the page-local inventory context has no hidden mappings"),
              InventoryContext->GetMappings().Num(), static_cast<int32>(UE_ARRAY_COUNT(ExpectedMappings)));
    for (const FExpectedInventoryMapping &Expected : ExpectedMappings) {
        TestEqual(
            *FString::Printf(TEXT("%s is mapped once to %s"),
                             *Expected.ActionName.ToString(), *Expected.Key.ToString()),
            CountAuthoredMapping(InventoryContext, Expected.ActionName, Expected.Key), 1);
    }
    return true;
}
