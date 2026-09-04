#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GameModes/GameState/MythicGameState.h"
#include "Settings/MythicCombatSettings.h"
#include "UObject/UnrealType.h"

namespace {
struct FMythicNamedValue {
    const TCHAR *Name;
    float Authored;
};

struct FMythicMovedValue {
    const TCHAR *Name;
    float Authored;
    float PreMoveDefault;
};

// The twelve GameState baseline values plus the floor that was a literal in the clamp calls.
// MaxAttackSpeedPlayRate deliberately left this table: it no longer keeps its pre-move default, so a row
// here would assert 0 == 0 and prove nothing. Its authored value is asserted on purpose below.
TArray<FMythicMovedValue> MovedValues(const UMythicCombatSettings &Settings) {
    return {
        {TEXT("MinChipDamage"), Settings.MinChipDamage, 1.0f},
        {TEXT("RageDamageBonus"), Settings.RageDamageBonus, 0.25f},
        {TEXT("WeakenedDamagePenalty"), Settings.WeakenedDamagePenalty, 0.25f},
        {TEXT("TerrifiedDamageBonus"), Settings.TerrifiedDamageBonus, 0.25f},
        {TEXT("FortifyDamageReduction"), Settings.FortifyDamageReduction, 0.25f},
        {TEXT("EnlightenProficiencyBonus"), Settings.EnlightenProficiencyBonus, 0.5f},
        {TEXT("MinAttackSpeedPlayRate"), Settings.MinAttackSpeedPlayRate, 0.8f},
        {TEXT("StatusBuildupPerProc"), Settings.StatusBuildupPerProc, 25.0f},
        {TEXT("MaxDodgeChance"), Settings.MaxDodgeChance, 0.75f},
        {TEXT("ProbabilitySoftCap"), Settings.ProbabilitySoftCap, 0.5f},
        {TEXT("MaxCooldownReduction"), Settings.MaxCooldownReduction, 0.8f},
        {TEXT("MaxStaminaCostReduction"), Settings.MaxStaminaCostReduction, 1.0f},
        {TEXT("MaxStatusResistance"), Settings.MaxStatusResistance, 1.0f},
    };
}

const TCHAR *const VacatedGameStateProperties[] = {
    TEXT("MinChipDamage"),
    TEXT("RageDamageBonus"),
    TEXT("WeakenedDamagePenalty"),
    TEXT("TerrifiedDamageBonus"),
    TEXT("FortifyDamageReduction"),
    TEXT("EnlightenProficiencyBonus"),
    TEXT("MinAttackSpeedPlayRate"),
    TEXT("MaxAttackSpeedPlayRate"),
    TEXT("StatusBuildupPerProc"),
    TEXT("MaxDodgeChance"),
    TEXT("ProbabilitySoftCap"),
    TEXT("MaxCooldownReduction"),
};
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatSettingsMovedDefaultsTest,
    "Mythic.Combat.CombatSettings.MovedDefaults",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatSettingsMovedDefaultsTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings &Settings = *GetDefault<UMythicCombatSettings>();

    for (const FMythicMovedValue &Row : MovedValues(Settings)) {
        // Zero tolerance: this refactor moved where a number lives, so any drift is a silent rebalance.
        TestEqual(FString::Printf(TEXT("%s keeps its exact pre-move default"), Row.Name),
                  Row.Authored, Row.PreMoveDefault, 0.0f);

        const FFloatProperty *Property = FindFProperty<FFloatProperty>(
            UMythicCombatSettings::StaticClass(), Row.Name);
        if (TestNotNull(FString::Printf(TEXT("%s is a reflected float on the settings class"), Row.Name),
                        Property)) {
            TestTrue(FString::Printf(TEXT("%s is config-backed and designer-editable"), Row.Name),
                     Property->HasAllPropertyFlags(CPF_Config | CPF_Edit));
        }
    }

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatSettingsGameStateVacatedTest,
    "Mythic.Combat.CombatSettings.GameStateVacated",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatSettingsGameStateVacatedTest::RunTest(const FString &Parameters) {
    const UClass *GameStateClass = AMythicGameState::StaticClass();

    for (const TCHAR *Name : VacatedGameStateProperties) {
        TestNull(FString::Printf(TEXT("AMythicGameState no longer declares %s"), Name),
                 FindFProperty<FProperty>(GameStateClass, Name));
    }

    // Without this control a renamed class or a broken lookup would report all twelve absences as satisfied.
    TestNotNull(TEXT("the same lookup still finds a baseline property that stayed on the GameState"),
                FindFProperty<FProperty>(GameStateClass, TEXT("ArmorMitigationCurveRowHandle")));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCombatSettingsInvariantsTest,
    "Mythic.Combat.CombatSettings.Invariants",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCombatSettingsInvariantsTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings &Settings = *GetDefault<UMythicCombatSettings>();

    TestTrue(TEXT("the attack-speed play-rate floor is a positive rate"),
             Settings.MinAttackSpeedPlayRate > 0.0f);
    // Exactly zero is the authored "no ceiling"; any other value has to sit above the floor to be a band at all.
    TestTrue(TEXT("the attack-speed play-rate band is ordered, or uncapped"),
             Settings.MaxAttackSpeedPlayRate == 0.0f
             || Settings.MinAttackSpeedPlayRate <= Settings.MaxAttackSpeedPlayRate);
    TestEqual(TEXT("attack speed ships with no ceiling, so stacking it never stops paying"),
              Settings.MaxAttackSpeedPlayRate, 0.0f);
    // Carried over from the moved-defaults table this value deliberately left, so leaving it stops proving less.
    const FFloatProperty *CeilingProperty = FindFProperty<FFloatProperty>(
        UMythicCombatSettings::StaticClass(), TEXT("MaxAttackSpeedPlayRate"));
    if (TestNotNull(TEXT("MaxAttackSpeedPlayRate is a reflected float on the settings class"), CeilingProperty)) {
        TestTrue(TEXT("MaxAttackSpeedPlayRate is config-backed and designer-editable"),
                 CeilingProperty->HasAllPropertyFlags(CPF_Config | CPF_Edit));
    }

    const FMythicNamedValue Ceilings[] = {
        {TEXT("MaxDodgeChance"), Settings.MaxDodgeChance},
        {TEXT("ProbabilitySoftCap"), Settings.ProbabilitySoftCap},
        {TEXT("MaxCooldownReduction"), Settings.MaxCooldownReduction},
        {TEXT("MaxStaminaCostReduction"), Settings.MaxStaminaCostReduction},
        {TEXT("MaxStatusResistance"), Settings.MaxStatusResistance},
    };
    for (const FMythicNamedValue &Ceiling : Ceilings) {
        TestTrue(FString::Printf(TEXT("%s is a fraction in [0,1]"), Ceiling.Name),
                 Ceiling.Authored >= 0.0f && Ceiling.Authored <= 1.0f);
    }

    // At 1.0 a stacked dodge build is untouchable, which is the one ceiling that must stay strictly below full.
    TestTrue(TEXT("MaxDodgeChance stays below total evasion"), Settings.MaxDodgeChance < 1.0f);

    TestTrue(TEXT("the status buildup a proc contributes is positive"),
             Settings.StatusBuildupPerProc > 0.0f);
    TestTrue(TEXT("the chip-damage floor is not negative"), Settings.MinChipDamage >= 0.0f);

    return true;
}

#endif
