
#include "Misc/AutomationTest.h"

#include "Engine/Engine.h"
#include "UI/Menu/MythicSettingsPageWidget.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsCategoryTest,
    "Mythic.UI.SettingsCategories",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsCategoryTest::RunTest(const FString &Parameters) {
    using Page = UMythicSettingsPageWidget;

    const FText Display = NSLOCTEXT("MythicTest", "Display", "DISPLAY");
    const FText Audio = NSLOCTEXT("MythicTest", "Audio", "AUDIO");
    const TArray<FText> Categories = {Display, Audio};

    // THE REGRESSION THIS EXISTS FOR: a page whose Blueprint has no tab strip yet must show everything.
    // Defaulting the active category to 0 instead hid every category but the first, which silently made
    // most of the page unreachable while still looking like it worked.
    TestTrue(TEXT("untabbed, a display row shows"), Page::IsRowVisible(false, Display, INDEX_NONE, Categories));
    TestTrue(TEXT("untabbed, an audio row shows too"), Page::IsRowVisible(false, Audio, INDEX_NONE, Categories));
    TestTrue(TEXT("untabbed, headings show, because they are then the only labels"),
             Page::IsRowVisible(true, Display, INDEX_NONE, Categories));

    // An index past the end is the same case, not a silently empty page.
    TestTrue(TEXT("an out-of-range tab shows everything"), Page::IsRowVisible(false, Audio, 7, Categories));
    TestTrue(TEXT("a negative tab shows everything"), Page::IsRowVisible(false, Audio, -3, Categories));

    // With no categories authored, nothing can be tabbed.
    const TArray<FText> None;
    TestTrue(TEXT("no categories means no filtering"), Page::IsRowVisible(false, Display, 0, None));

    // Tabbed: only the active category's settings.
    TestTrue(TEXT("the active tab shows its own rows"), Page::IsRowVisible(false, Display, 0, Categories));
    TestFalse(TEXT("and hides another tab's rows"), Page::IsRowVisible(false, Audio, 0, Categories));
    TestTrue(TEXT("switching tab shows the other set"), Page::IsRowVisible(false, Audio, 1, Categories));

    // The tab already names the category, so repeating it as a heading row is the label twice.
    TestFalse(TEXT("the active tab hides its own heading"), Page::IsRowVisible(true, Display, 0, Categories));

    // The rule above was never the bug. The bug was the DEFAULT: shipping ActiveCategory = 0 meant a page
    // whose Blueprint had no tab strip engaged tabbing anyway and hid 31 of its 36 settings. Asserting the
    // rule would not have caught that, so assert the default a fresh page starts at.
    if (UMythicSettingsPageWidget *Fresh = NewObject<UMythicSettingsPageWidget>()) {
        TestEqual(TEXT("a fresh page starts untabbed, so every setting is reachable"),
                  Fresh->GetActiveCategory(), (int32)INDEX_NONE);
        TestTrue(TEXT("and that default really does show a row from a later category"),
                 Page::IsRowVisible(false, Audio, Fresh->GetActiveCategory(), Categories));
    }

    return true;
}
