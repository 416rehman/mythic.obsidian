#include "Misc/AutomationTest.h"

#include "UI/Menu/MythicEscapeMenuWidget.h"
#include "UI/Menu/MythicMenuShell.h"
#include "UI/Settings/MythicSettingRowBase.h"
#include "UI/Settings/MythicSettingsScreenBase.h"

namespace {
const TCHAR *ShellPath = TEXT("/Game/Mythic/UI/Menu/WBP_MenuShell.WBP_MenuShell_C");
const TCHAR *EscapePath = TEXT("/Game/Mythic/UI/Widgets/EscapeMenu/WBP_EscapeMenu.WBP_EscapeMenu_C");

template <typename T>
const T *LoadDefaults(const TCHAR *Path) {
    const UClass *Loaded = LoadClass<UObject>(nullptr, Path);
    return Loaded ? Cast<T>(Loaded->GetDefaultObject()) : nullptr;
}

/** The settings screen, resolved the way the game resolves it: through the escape menu. */
const UMythicSettingsScreenBase *ResolveSettingsScreen() {
    const UMythicEscapeMenuWidget *Escape = LoadDefaults<UMythicEscapeMenuWidget>(EscapePath);
    const TSubclassOf<UCommonActivatableWidget> Screen = Escape ? Escape->GetSettingsScreenClass() : nullptr;
    return Screen && Screen->IsChildOf(UMythicSettingsScreenBase::StaticClass())
               ? Cast<UMythicSettingsScreenBase>(Screen->GetDefaultObject())
               : nullptr;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsReachableTest,
    "Mythic.UI.SettingsReachable",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsReachableTest::RunTest(const FString &Parameters) {
    // THE REGRESSION THIS EXISTS FOR: the settings screen was built, compiled, saved and verified by
    // readback - and nothing referenced it, so none of the work was reachable from the game. Asserting
    // the asset is correct cannot catch that.
    //
    // The route has since changed, and the test with it. Settings must NOT be a page of the player menu:
    // that menu is character, inventory and map, and it does not exist while the player is dead. Settings
    // has to work from the death screen, so it lives on the escape menu and nowhere else.
    const UMythicEscapeMenuWidget *Escape = LoadDefaults<UMythicEscapeMenuWidget>(EscapePath);
    if (!TestNotNull(TEXT("the escape menu Blueprint loads"), Escape)) {
        return false;
    }
    const TSubclassOf<UCommonActivatableWidget> EscapePage = Escape->GetSettingsScreenClass();
    TestNotNull(TEXT("Escape offers a settings screen"), EscapePage.Get());
    TestTrue(TEXT("and it really is a settings screen"),
             EscapePage && EscapePage->IsChildOf(UMythicSettingsScreenBase::StaticClass()));

    if (const UMythicMenuShell *Shell = LoadDefaults<UMythicMenuShell>(ShellPath)) {
        TestNull(TEXT("the player menu does NOT carry a Settings tab"),
                 Shell->GetRegisteredPageClass(TEXT("Settings")).Get());
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsCatalogDrawableTest,
    "Mythic.UI.SettingsCatalogDrawable",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsCatalogDrawableTest::RunTest(const FString &Parameters) {
    const UMythicSettingsScreenBase *Screen = ResolveSettingsScreen();
    if (!TestNotNull(TEXT("Escape resolves to a settings screen"), Screen)) {
        return false;
    }

    const UMythicSettingsCatalog *Catalog = Screen->GetCatalog();
    if (!TestNotNull(TEXT("the screen points at a catalog"), Catalog)) {
        return false;
    }
    TestTrue(TEXT("the catalog is not empty"), Catalog->Settings.Num() > 0);
    TestTrue(TEXT("the catalog declares tabs"), Catalog->Categories.Num() > 0);

    // A control kind with no row Blueprint draws nothing: the setting exists in data, the page looks
    // fine, and the row is simply absent. That is exactly how Ambient Occlusion went missing before.
    TestNotNull(TEXT("group headings have a row Blueprint"), Screen->GetGroupHeadingClass().Get());
    for (const FMythicSettingDefinition &Def : Catalog->Settings) {
        if (!Screen->GetRowClassFor(Def.Control)) {
            AddError(FString::Printf(TEXT("setting '%s' uses a control the screen cannot draw"),
                                     *Def.Label.ToString()));
        }
    }

    // Every authored setting must land on exactly one tab, or it is unreachable however good the row is.
    int32 Reachable = 0;
    for (int32 Index = 0; Index < Catalog->Categories.Num(); ++Index) {
        for (const FMythicSettingDefinition &Row : Screen->GetRowsForCategory(Index)) {
            if (!UMythicSettingsScreenBase::IsGroupHeading(Row)) {
                ++Reachable;
            }
        }
    }
    TestEqual(TEXT("every catalog setting appears under some tab"), Reachable, Catalog->Settings.Num());

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsGroupingTest,
    "Mythic.UI.SettingsGrouping",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsGroupingTest::RunTest(const FString &Parameters) {
    const UMythicSettingsScreenBase *Screen = ResolveSettingsScreen();
    if (!Screen || !Screen->GetCatalog()) {
        AddError(TEXT("no settings screen with a catalog is reachable"));
        return false;
    }

    for (int32 Index = 0; Index < Screen->GetCategories().Num(); ++Index) {
        const TArray<FMythicSettingDefinition> Rows = Screen->GetRowsForCategory(Index);
        if (Rows.Num() == 0) {
            AddError(FString::Printf(TEXT("tab %d has no rows, so it opens onto nothing"), Index));
            continue;
        }
        // A tab with several groups introduces each one. A tab with a SINGLE group shows no heading at
        // all, because that heading would only restate the tab - a section inside a section of one.
        int32 Headings = 0;
        for (const FMythicSettingDefinition &Row : Rows) {
            if (UMythicSettingsScreenBase::IsGroupHeading(Row)) {
                ++Headings;
            }
        }
        if (Headings > 0) {
            TestTrue(FString::Printf(TEXT("tab %d starts with a group heading"), Index),
                     UMythicSettingsScreenBase::IsGroupHeading(Rows[0]));
            TestTrue(FString::Printf(TEXT("tab %d has more than one group to tell apart"), Index),
                     Headings > 1);
        }

        TSet<FString> SeenGroups;
        FString Current;
        for (const FMythicSettingDefinition &Row : Rows) {
            if (UMythicSettingsScreenBase::IsGroupHeading(Row)) {
                Current = Row.Group.ToString();
                // A group appearing twice means the list was assembled in the wrong order and the
                // player sees the same heading further down with more settings under it.
                TestFalse(FString::Printf(TEXT("group '%s' appears once"), *Current),
                          SeenGroups.Contains(Current));
                SeenGroups.Add(Current);
            }
            else if (Headings > 0) {
                // Only meaningful where headings are drawn. A single-group tab shows none by design, so
                // there is no heading for its settings to sit under.
                TestEqual(FString::Printf(TEXT("setting '%s' sits under its own heading"),
                                          *Row.Label.ToString()),
                          Row.Group.ToString(), Current);
            }
        }
    }

    return true;
}
