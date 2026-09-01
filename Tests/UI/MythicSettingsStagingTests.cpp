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

    // Dirty means different, not merely touched. Returning a control to its opening value must turn Apply
    // back off and restore the footer's plain Back action without requiring a screen reopen.
    UMythicSettingAccess::WriteValue(Def, Before);
    TestFalse(TEXT("returning to the committed value clears dirty state"),
              UMythicSettingAccess::HasStagedChanges());
    TestEqual(TEXT("the row reads the committed value after undoing the edit"),
              UMythicSettingAccess::ReadValue(Def), Before);
    TestEqual(TEXT("undoing a staged edit still does not touch the live cvar"), LiveCVar(Name), Before);

    UMythicSettingAccess::WriteValue(Def, Proposed);

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
    FMythicSettingsRestoreDefaultsStageTest,
    "Mythic.UI.SettingsRestoreDefaultsStaging",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsRestoreDefaultsStageTest::RunTest(const FString &Parameters) {
    const TCHAR *Name = TEXT("r.Tonemapper.Sharpen");
    if (!IConsoleManager::Get().FindConsoleVariable(Name)) {
        AddWarning(TEXT("r.Tonemapper.Sharpen is not registered in this build; staged defaults untested"));
        return true;
    }

    const float Before = LiveCVar(Name);
    FMythicSettingDefinition Def = MakeCVarSetting(Name);
    Def.DefaultValue = Before < 0.5f ? 0.75f : 0.25f;

    UMythicSettingsCatalog *Catalog = NewObject<UMythicSettingsCatalog>(GetTransientPackage());
    Catalog->Settings.Add(Def);

    UMythicSettingAccess::BeginStaging();
    UMythicSettingAccess::RestoreDefaults(Catalog);

    TestTrue(TEXT("Reset Defaults creates a pending proposal"),
             UMythicSettingAccess::HasStagedChanges());
    TestEqual(TEXT("the row previews its authored default"),
              UMythicSettingAccess::ReadValue(Def), Def.DefaultValue);
    TestEqual(TEXT("Reset Defaults does not mutate the running game before Apply"),
              LiveCVar(Name), Before);

    UMythicSettingAccess::RevertStaged();
    TestFalse(TEXT("Cancel clears a staged reset"), UMythicSettingAccess::HasStagedChanges());
    TestEqual(TEXT("Cancel restores the row to its committed value"),
              UMythicSettingAccess::ReadValue(Def), Before);
    TestEqual(TEXT("Cancel after Reset Defaults leaves the live cvar unchanged"),
              LiveCVar(Name), Before);

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
    A.ExtraCVars.Add(TEXT("r.MotionBlurQuality"), 0.0f);
    FMythicSettingOption B;
    B.Label = INVTEXT("Shared B");
    B.Value = 0.5f;   // deliberately the same primary value
    B.ExtraCVars.Add(TEXT("r.MotionBlurQuality"), 1.0f);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSettingsProfileDefaultTest,
    "Mythic.UI.SettingsProfileDefaultStaging",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSettingsProfileDefaultTest::RunTest(const FString &Parameters) {
    const FName PrimaryName(TEXT("r.Tonemapper.Sharpen"));
    const FName CompanionName(TEXT("r.MotionBlurQuality"));
    IConsoleVariable *Primary = IConsoleManager::Get().FindConsoleVariable(*PrimaryName.ToString());
    IConsoleVariable *Companion = IConsoleManager::Get().FindConsoleVariable(*CompanionName.ToString());
    if (!Primary || !Companion) {
        AddWarning(TEXT("Profile-default cvars are not registered in this build"));
        return true;
    }

    FMythicSettingDefinition Def = MakeCVarSetting(*PrimaryName.ToString());
    Def.Control = EMythicSettingControl::Select;
    Def.DefaultValue = Primary->GetFloat();

    FMythicSettingOption DefaultProfile;
    DefaultProfile.Label = INVTEXT("Default profile");
    DefaultProfile.Value = Def.DefaultValue;
    DefaultProfile.ExtraCVars.Add(CompanionName, Companion->GetFloat() + 100.0f);
    DefaultProfile.bIsDefault = true;

    FMythicSettingOption LiveProfile;
    LiveProfile.Label = INVTEXT("Live profile");
    LiveProfile.Value = Def.DefaultValue;
    LiveProfile.ExtraCVars.Add(CompanionName, Companion->GetFloat());
    Def.Options = {DefaultProfile, LiveProfile};

    UMythicSettingAccess::BeginStaging();
    TestFalse(TEXT("a matching primary value does not hide companion-profile drift"),
              UMythicSettingAccess::IsAtDefault(Def));

    UMythicSettingAccess::StageDefault(Def);
    TestTrue(TEXT("the explicit default profile is staged"), UMythicSettingAccess::HasStagedChanges());
    TestEqual(TEXT("staging preserves the explicit profile identity"),
              UMythicSettingAccess::ReadOptionIndex(Def), 0);
    TestTrue(TEXT("the staged default profile reads as default"), UMythicSettingAccess::IsAtDefault(Def));
    TestEqual(TEXT("staging a profile does not mutate its companion cvar"),
              Companion->GetFloat(), LiveProfile.ExtraCVars[CompanionName]);

    UMythicSettingAccess::RevertStaged();
    return true;
}
