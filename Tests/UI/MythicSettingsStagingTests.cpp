#include "Misc/AutomationTest.h"

#include "HAL/IConsoleManager.h"
#include "UI/Settings/MythicSettingAccess.h"
#include "UI/Settings/MythicSettingDefinition.h"

namespace {
/** A cvar-backed setting pointed at a real engine cvar with a harmless value. */
FMythicSettingDefinition MakeCVarSetting(const TCHAR *CVarName) {
    FMythicSettingDefinition Def;
    Def.Source = EMythicSettingSource::CVar;
    Def.SourceName = FName(CVarName);
    Def.Control = EMythicSettingControl::Slider;
    Def.MinValue = 0.0f;
    Def.MaxValue = 1.0f;
    return Def;
}

float LiveCVar(const TCHAR *Name) {
    const IConsoleVariable *CVar = IConsoleManager::Get().FindConsoleVariable(Name);
    return CVar ? CVar->GetFloat() : TNumericLimits<float>::Lowest();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsStageTest,
    "Mythic.UI.SettingsStaging",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsStageTest::RunTest(const FString &Parameters) {
    // THE BEHAVIOUR THIS EXISTS FOR: a settings screen must be a proposal. Changing a row may not touch
    // the running game - it is what makes the screen safe to explore, and it is why stepping through five
    // quality levels costs five map writes instead of five renderer rebuilds.
    const TCHAR *Name = TEXT("r.Tonemapper.Sharpen");
    if (!IConsoleManager::Get().FindConsoleVariable(Name)) {
        AddWarning(TEXT("r.Tonemapper.Sharpen is not registered in this build; staging untested"));
        return true;
    }

    const FMythicSettingDefinition Def = MakeCVarSetting(Name);
    const float Before = LiveCVar(Name);

    UMythicSettingAccess::BeginStaging();
    TestFalse(TEXT("a fresh screen has nothing pending"), UMythicSettingAccess::HasStagedChanges());

    const float Proposed = Before + 0.25f;
    UMythicSettingAccess::WriteValue(Def, Proposed);

    TestTrue(TEXT("the change is now pending"), UMythicSettingAccess::HasStagedChanges());
    TestEqual(TEXT("the row shows what the player asked for"),
              UMythicSettingAccess::ReadValue(Def), Proposed);
    // The whole point: the game has not heard about it.
    TestEqual(TEXT("the live cvar is untouched before Apply"), LiveCVar(Name), Before);

    // Backing out discards, and because nothing was applied there is no restore pass to get wrong.
    UMythicSettingAccess::RevertStaged();
    TestFalse(TEXT("discarding clears the buffer"), UMythicSettingAccess::HasStagedChanges());
    TestEqual(TEXT("and the cvar is still what it always was"), LiveCVar(Name), Before);
    TestEqual(TEXT("the row reads the live value again"), UMythicSettingAccess::ReadValue(Def), Before);

    // Apply is the only thing that reaches the renderer.
    UMythicSettingAccess::BeginStaging();
    UMythicSettingAccess::WriteValue(Def, Proposed);
    UMythicSettingAccess::CommitStaged();
    TestEqual(TEXT("Apply writes the value for real"), LiveCVar(Name), Proposed);
    TestFalse(TEXT("and nothing is left pending"), UMythicSettingAccess::HasStagedChanges());

    // Leave the machine as we found it.
    UMythicSettingAccess::BeginStaging();
    UMythicSettingAccess::WriteValue(Def, Before);
    UMythicSettingAccess::CommitStaged();

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsOptionStageTest,
    "Mythic.UI.SettingsOptionStaging",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsOptionStageTest::RunTest(const FString &Parameters) {
    // Two options can share a primary value and differ only by the profile riding them - Software and
    // Hardware Ray Tracing are both r.DynamicGlobalIlluminationMethod 1. Staging has to remember WHICH
    // option was chosen, or Apply replays the wrong extras and the setting silently reverts.
    const TCHAR *Name = TEXT("r.Tonemapper.Sharpen");
    if (!IConsoleManager::Get().FindConsoleVariable(Name)) {
        return true;
    }

    FMythicSettingDefinition Def = MakeCVarSetting(Name);
    Def.Control = EMythicSettingControl::Select;

    FMythicSettingOption A;
    A.Label = INVTEXT("Shared A");
    A.Value = 0.5f;
    FMythicSettingOption B;
    B.Label = INVTEXT("Shared B");
    B.Value = 0.5f;   // deliberately the same primary value
    Def.Options = {A, B};

    const float Before = LiveCVar(Name);

    UMythicSettingAccess::BeginStaging();
    UMythicSettingAccess::WriteOptionIndex(Def, 1);

    TestEqual(TEXT("the second option stays chosen even though it shares a value"),
              UMythicSettingAccess::ReadOptionIndex(Def), 1);
    TestEqual(TEXT("and still nothing reached the cvar"), LiveCVar(Name), Before);

    UMythicSettingAccess::RevertStaged();
    UMythicSettingAccess::BeginStaging();
    UMythicSettingAccess::WriteValue(Def, Before);
    UMythicSettingAccess::CommitStaged();

    return true;
}
