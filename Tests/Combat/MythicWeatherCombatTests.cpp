
#include "Misc/AutomationTest.h"
#include "GAS/MythicWeatherCombatRules.h"
#include "World/EnvironmentController/EnvironmentTags.h"
#include "GAS/MythicTags_GAS.h"

namespace {
FMythicWeatherDamageMod MakeMod(FGameplayTag Weather, FGameplayTag DamageType, float Mult, FGameplayTag AddStatus = FGameplayTag()) {
    FMythicWeatherDamageMod Mod;
    Mod.WeatherTag = Weather;
    Mod.DamageTypeTag = DamageType;
    Mod.Multiplier = Mult;
    Mod.AddStatusTag = AddStatus;
    return Mod;
}
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeatherCombatMultiplierTest,
    "Mythic.Combat.WeatherCombat.Multiplier",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeatherCombatMultiplierTest::RunTest(const FString &Parameters) {
    const FGameplayTag Rain = Environment_Weather_Rain;
    const FGameplayTag Snow = Environment_Weather_Snow;
    const FGameplayTag Clear = Environment_Weather_Clear;
    const FGameplayTag WeatherParent = Environment_Weather;
    const FGameplayTag Fire = GAS_DEBUFF_BURNING;
    const FGameplayTag Frost = GAS_DEBUFF_FROZEN;

    FGameplayTagContainer FireHit;
    FireHit.AddTag(Fire);
    FGameplayTagContainer FrostHit;
    FrostHit.AddTag(Frost);

    {
        const TArray<FMythicWeatherDamageMod> Empty;
        TestEqual(TEXT("empty config → exactly 1.0"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Empty, Rain, FireHit), 1.0f);
        TestEqual(TEXT("empty config + invalid weather → 1.0"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Empty, FGameplayTag(), FireHit), 1.0f);
    }

    TArray<FMythicWeatherDamageMod> Mods;
    Mods.Add(MakeMod(Rain, Fire, 0.75f));
    Mods.Add(MakeMod(Snow, Frost, 1.15f));
    Mods.Add(MakeMod(Clear, Fire, 1.10f));

    TestEqual(TEXT("rain + fire hit → 0.75"),
              FMythicWeatherCombatRules::ResolveWeatherMultiplier(Mods, Rain, FireHit), 0.75f);
    TestEqual(TEXT("snow + frost hit → 1.15"),
              FMythicWeatherCombatRules::ResolveWeatherMultiplier(Mods, Snow, FrostHit), 1.15f);

    TestEqual(TEXT("snow + fire hit → 1.0 (no snow-fire row)"),
              FMythicWeatherCombatRules::ResolveWeatherMultiplier(Mods, Snow, FireHit), 1.0f);
    TestEqual(TEXT("rain + frost hit → 1.0 (no rain-frost row)"),
              FMythicWeatherCombatRules::ResolveWeatherMultiplier(Mods, Rain, FrostHit), 1.0f);
    TestEqual(TEXT("invalid current weather → 1.0"),
              FMythicWeatherCombatRules::ResolveWeatherMultiplier(Mods, FGameplayTag(), FireHit), 1.0f);
    {
        const FGameplayTagContainer NoTags;
        TestEqual(TEXT("hit with no damage-type tags → 1.0"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Mods, Rain, NoTags), 1.0f);
    }

    {
        TArray<FMythicWeatherDamageMod> Wide;
        Wide.Add(MakeMod(Rain, FGameplayTag(), 0.9f));
        const FGameplayTagContainer NoTags;
        TestEqual(TEXT("weather-wide row matches an untagged hit"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Wide, Rain, NoTags), 0.9f);

        TArray<FMythicWeatherDamageMod> Parent;
        Parent.Add(MakeMod(WeatherParent, Fire, 1.2f));
        TestEqual(TEXT("parent-authored weather row matches child weather (rain)"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Parent, Rain, FireHit), 1.2f);
        TestEqual(TEXT("parent-authored weather row matches child weather (snow)"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Parent, Snow, FireHit), 1.2f);
    }

    {
        TArray<FMythicWeatherDamageMod> Stack;
        Stack.Add(MakeMod(Rain, Fire, 0.5f));
        Stack.Add(MakeMod(Rain, FGameplayTag(), 0.5f));
        TestEqual(TEXT("two matching rows multiply (0.5 * 0.5)"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Stack, Rain, FireHit), 0.25f);

        TArray<FMythicWeatherDamageMod> Neg;
        Neg.Add(MakeMod(Rain, Fire, -2.0f));
        TestEqual(TEXT("pathological negative multiplier clamps to 0 (never sign-flips damage)"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(Neg, Rain, FireHit), 0.0f);
    }

    {
        TArray<FMythicWeatherDamageMod> BadRow;
        BadRow.Add(MakeMod(FGameplayTag(), Fire, 5.0f));
        TestEqual(TEXT("invalid-weather row never matches"),
                  FMythicWeatherCombatRules::ResolveWeatherMultiplier(BadRow, Rain, FireHit), 1.0f);
    }

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWeatherCombatStatusBonusTest,
    "Mythic.Combat.WeatherCombat.StatusBonus",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWeatherCombatStatusBonusTest::RunTest(const FString &Parameters) {
    const FGameplayTag Rain = Environment_Weather_Rain;
    const FGameplayTag Snow = Environment_Weather_Snow;
    const FGameplayTag Shock = GAS_DEBUFF_STUNNED;
    const FGameplayTag Slowed = GAS_DEBUFF_SLOWED;
    const FGameplayTag Frozen = GAS_DEBUFF_FROZEN;

    FGameplayTagContainer ShockHit;
    ShockHit.AddTag(Shock);

    {
        const TArray<FMythicWeatherDamageMod> Empty;
        TestFalse(TEXT("empty config → no status bonus"),
                  FMythicWeatherCombatRules::ResolveWeatherStatusBonus(Empty, Rain, ShockHit).IsValid());
    }

    TArray<FMythicWeatherDamageMod> Mods;
    Mods.Add(MakeMod(Rain, Shock, 1.25f, Slowed));
    Mods.Add(MakeMod(Snow, FGameplayTag(), 1.0f, Frozen));

    TestEqual(TEXT("rain + shock hit → bonus Slowed buildup"),
              FMythicWeatherCombatRules::ResolveWeatherStatusBonus(Mods, Rain, ShockHit), Slowed);
    TestEqual(TEXT("snow + shock hit → the snow-wide Frozen bonus"),
              FMythicWeatherCombatRules::ResolveWeatherStatusBonus(Mods, Snow, ShockHit), Frozen);

    {
        FGameplayTagContainer FireHit;
        FireHit.AddTag(GAS_DEBUFF_BURNING);
        TestFalse(TEXT("rain + fire hit → no bonus (no matching bonus row)"),
                  FMythicWeatherCombatRules::ResolveWeatherStatusBonus(Mods, Rain, FireHit).IsValid());
        TestFalse(TEXT("invalid weather → no bonus"),
                  FMythicWeatherCombatRules::ResolveWeatherStatusBonus(Mods, FGameplayTag(), ShockHit).IsValid());

        TArray<FMythicWeatherDamageMod> NoBonus;
        NoBonus.Add(MakeMod(Rain, Shock, 1.25f));
        TestFalse(TEXT("matching multiplier-only row carries no bonus"),
                  FMythicWeatherCombatRules::ResolveWeatherStatusBonus(NoBonus, Rain, ShockHit).IsValid());
    }

    {
        TArray<FMythicWeatherDamageMod> TwoBonuses;
        TwoBonuses.Add(MakeMod(Rain, Shock, 1.0f, Slowed));
        TwoBonuses.Add(MakeMod(Rain, Shock, 1.0f, Frozen));
        TestEqual(TEXT("first matching bonus row wins"),
                  FMythicWeatherCombatRules::ResolveWeatherStatusBonus(TwoBonuses, Rain, ShockHit), Slowed);
    }

    return true;
}
