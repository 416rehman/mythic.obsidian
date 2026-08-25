// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Itemization/Affixes/MythicAffixPoolDataAsset.h"
#include "Itemization/Inventory/Fragments/FragmentTypes.h"
#include "Itemization/Inventory/Fragments/Passive/AffixesFragment.h"
#include "Settings/MythicCombatSettings.h"
#include "Settings/MythicDeveloperSettings.h"

namespace {
float GrowthAt(const UMythicCombatSettings *Settings, const FCurveTableRowHandle &Handle, const float Level) {
    return MythicCombat::SampleOpenEnded(Handle, Level, Settings->PlayerPrimaryTailGrowth);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicPrimaryGrowthCurvesTest,
                                 "Mythic.Combat.PrimaryGrowth.AuthoredCurves",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicPrimaryGrowthCurvesTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();

    // Both primaries have authored, strictly rising growth. A missing row here means the model regressed
    // to flat primaries silently.
    for (const FCurveTableRowHandle *Handle : {&Settings->PlayerPowerCurve, &Settings->PlayerStrengthCurve}) {
        if (!TestFalse(TEXT("Primary growth curve is authored"), Handle->IsNull())) {
            return false;
        }
        const float At1 = GrowthAt(Settings, *Handle, 1.0f);
        const float At30 = GrowthAt(Settings, *Handle, 30.0f);
        const float At60 = GrowthAt(Settings, *Handle, 60.0f);
        TestTrue(TEXT("Growth rises through the levelling band"), At30 > At1 && At60 > At30);
    }

    // Power and Strength scale independently - the central requirement of the rework. Identical spans
    // would silently collapse the two knobs into one.
    const float PowerSpan = GrowthAt(Settings, Settings->PlayerPowerCurve, 60.0f) - GrowthAt(Settings, Settings->PlayerPowerCurve, 1.0f);
    const float StrengthSpan = GrowthAt(Settings, Settings->PlayerStrengthCurve, 60.0f) - GrowthAt(Settings, Settings->PlayerStrengthCurve, 1.0f);
    TestTrue(TEXT("The two primaries grow by different authored amounts"),
             !FMath::IsNearlyEqual(PowerSpan, StrengthSpan, 0.01f));

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicHitsToKillBandTest,
                                 "Mythic.Combat.PrimaryGrowth.HitsToKillBand",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicHitsToKillBandTest::RunTest(const FString &Parameters) {
    /**
     * The assertion that protects the game from its open-ended tails: as player level, gear level and
     * combatant level advance together, hits-to-kill must stay inside a band - never trivialising and
     * never bricking. Every input is the real authored artifact the game reads: Power from the player
     * growth curve, its damage contribution through the authored row and the same Diminish the MMC uses,
     * the central core-affix weapon band, and the combatant health curves off the shipped game state.
     */
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();

    const FMythicStatContribution *PowerToDamage = nullptr;
    for (const FMythicStatContribution &Row : Settings->StatContributions.Contributions) {
        if (Row.SourceStat.GetName() == TEXT("Power") && Row.TargetAttribute.GetName() == TEXT("DamagePerHit")) {
            PowerToDamage = &Row;
        }
    }
    if (!TestNotNull(TEXT("Power->DamagePerHit contribution is authored"), PowerToDamage)) {
        return false;
    }

    const UClass *GameStateClass = LoadClass<AMythicGameState>(
        nullptr, TEXT("/Game/Mythic/Gameplay/GameModes/BP_MythicGameState.BP_MythicGameState_C"));
    if (!TestNotNull(TEXT("Shipped game state loads"), static_cast<const UObject *>(GameStateClass))) {
        return false;
    }
    const AMythicGameState *GS = GameStateClass->GetDefaultObject<AMythicGameState>();
    if (!TestFalse(TEXT("Combatant health curves are authored"), GS->HealthMinCurveRowHandle.IsNull())) {
        return false;
    }

    auto PlayerOutputAt = [&](const float Level) -> float {
        const float Power = GrowthAt(Settings, Settings->PlayerPowerCurve, Level);
        float Bonus = Power * PowerToDamage->PerPoint;
        if (PowerToDamage->CeilingBonus > 0.0f) {
            Bonus = MythicCombat::Diminish(Bonus, PowerToDamage->SoftCapBonus, PowerToDamage->CeilingBonus);
        }
        const float WeaponBand = MythicCombat::SampleOpenEnded(
            Settings->CoreAffixLevelCurve, Level, Settings->CoreAffixTailGrowth);
        return (1.0f + Bonus) * WeaponBand;
    };
    auto CombatantHealthAt = [&](const float Level) -> float {
        const float Low = MythicCombat::SampleOpenEnded(GS->HealthMinCurveRowHandle, Level, Settings->CombatantHealthTailGrowth);
        const float High = MythicCombat::SampleOpenEnded(GS->HealthMaxCurveRowHandle, Level, Settings->CombatantHealthTailGrowth);
        return (Low + High) * 0.5f;
    };

    const float Output1 = PlayerOutputAt(1.0f);
    const float Health1 = CombatantHealthAt(1.0f);
    float MinRatio = TNumericLimits<float>::Max();
    float MaxRatio = 0.0f;
    for (float Level = 1.0f; Level <= 120.0f; Level += 1.0f) {
        const float HitsRatio = (CombatantHealthAt(Level) / Health1) / (PlayerOutputAt(Level) / Output1);
        MinRatio = FMath::Min(MinRatio, HitsRatio);
        MaxRatio = FMath::Max(MaxRatio, HitsRatio);
    }

    AddInfo(FString::Printf(TEXT("hits-to-kill ratio across levels 1-120: %.2f .. %.2f of the level-1 pace"), MinRatio, MaxRatio));
    TestTrue(TEXT("Fights never trivialise (floor)"), MinRatio > 0.25f);
    TestTrue(TEXT("Fights never brick (ceiling)"), MaxRatio < 4.0f);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicOverallLevelFormulaTest,
                                 "Mythic.Combat.PrimaryGrowth.OverallLevelFormula",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicOverallLevelFormulaTest::RunTest(const FString &Parameters) {
    const UMythicDeveloperSettings *Dev = GetDefault<UMythicDeveloperSettings>();
    const float MaxXp = 100000.0f;

    // Bounds and monotonicity across the whole band. The formula inverts a geometric curve, so early
    // XP must buy levels visibly faster than a flat ratio would (the flat ratio parked a fresh
    // character at level 1 for hours).
    TestEqual(TEXT("Zero XP is level 1"), UMythicAttributeSet_Proficiencies::LevelFromXp(0.0f, MaxXp), 1);
    TestEqual(TEXT("Full XP is max level"), UMythicAttributeSet_Proficiencies::LevelFromXp(MaxXp, MaxXp), Dev->MaxLevel);
    TestEqual(TEXT("No denominator means level 1"), UMythicAttributeSet_Proficiencies::LevelFromXp(500.0f, 0.0f), 1);

    int32 Prev = 0;
    for (float Xp = 0.0f; Xp <= MaxXp; Xp += MaxXp / 200.0f) {
        const int32 L = UMythicAttributeSet_Proficiencies::LevelFromXp(Xp, MaxXp);
        TestTrue(TEXT("Level never decreases with XP"), L >= Prev);
        Prev = L;
    }

    // The window formula must agree with the level formula and hold its invariants everywhere.
    for (float Xp = 0.0f; Xp <= MaxXp; Xp += MaxXp / 40.0f) {
        int32 Level = 0;
        float Into = 0.0f;
        float Span = 0.0f;
        UMythicAttributeSet_Proficiencies::GetLevelXpWindow(Xp, MaxXp, Level, Into, Span);
        TestEqual(TEXT("Window level matches the level formula"), Level, UMythicAttributeSet_Proficiencies::LevelFromXp(Xp, MaxXp));
        if (Level < Dev->MaxLevel) {
            TestTrue(TEXT("Progress sits inside the level span"), Into >= 0.0f && Span > 0.0f && Into <= Span + 1.0f);
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicWholeNumberRollTest,
                                 "Mythic.Itemization.Affixes.WholeNumberRolls",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicWholeNumberRollTest::RunTest(const FString &Parameters) {
    // The whole-number flag is authored per roll definition and binds the rolled VALUE, not its display.
    FRollDefinition Whole;
    Whole.Min = 10.0f;
    Whole.Max = 25.0f;
    Whole.LevelScaling = 0.37f;
    Whole.bWholeNumber = true;
    Whole.bIsPercentage = false;
    const FGameplayAttribute Power = UMythicAttributeSet_Offense::GetPowerAttribute();
    for (int32 i = 0; i < 64; ++i) {
        const FRolledAttributeSpec Spec(Power, 7, Whole);
        TestTrue(FString::Printf(TEXT("Whole-number definition rolls an integer (got %f)"), Spec.Value),
                 FMath::IsNearlyEqual(Spec.Value, FMath::RoundToFloat(Spec.Value)));
        TestTrue(TEXT("Roll stays inside the scaled band"),
                 Spec.Value >= FMath::FloorToFloat(Whole.GetScaledMin(7)) && Spec.Value <= FMath::CeilToFloat(Whole.GetScaledMax(7)));
    }

    FRollDefinition Fractional = Whole;
    Fractional.bWholeNumber = false;
    bool bSawFraction = false;
    for (int32 i = 0; i < 64 && !bSawFraction; ++i) {
        const FRolledAttributeSpec Spec(Power, 7, Fractional);
        bSawFraction = !FMath::IsNearlyEqual(Spec.Value, FMath::RoundToFloat(Spec.Value));
    }
    TestTrue(TEXT("A continuous definition still rolls fractions (the flag is opt-in, not global)"), bSawFraction);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMythicWholeNumberTieredRollTest,
                                 "Mythic.Itemization.Affixes.WholeNumberTieredRolls",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::ProductFilter)

bool FMythicWholeNumberTieredRollTest::RunTest(const FString &Parameters) {
    // The tiered pool is how every shipped item rolls. Its roll OVERWRITES the ctor value, so the
    // whole-number snap has to hold on that path specifically, and the flag must land on the stored
    // Definition so rerolls stay whole too.
    UMythicAffixPoolDataAsset *Pool = NewObject<UMythicAffixPoolDataAsset>();
    FMythicTieredAffixDef Def;
    Def.Attribute = UMythicAttributeSet_Offense::GetPowerAttribute();
    Def.Group = EMythicAffixGroup::Prefix;
    Def.bWholeNumber = true;
    FMythicAffixTier Tier;
    Tier.MinItemLevel = 1;
    Tier.Weight = 1.0f;
    Tier.Min = 3.25f;
    Tier.Max = 9.75f;
    Tier.LevelScaling = 0.37f;
    Def.Tiers.Add(Tier);
    Pool->Defs.Add(Def);

    for (int32 i = 0; i < 32; ++i) {
        UAffixesFragment *Fragment = NewObject<UAffixesFragment>();
        Fragment->RollAffixesTiered(7, 1, FGameplayTagContainer(), Pool);
        if (!TestEqual(TEXT("Tiered roll produced one affix"), Fragment->AffixesRuntimeReplicatedData.RolledAffixes.Num(), 1)) {
            return false;
        }
        const FRolledAffix &Affix = Fragment->AffixesRuntimeReplicatedData.RolledAffixes[0];
        TestTrue(FString::Printf(TEXT("Tiered whole-number roll is an integer (got %f)"), Affix.Value),
                 FMath::IsNearlyEqual(Affix.Value, FMath::RoundToFloat(Affix.Value)));
        TestTrue(TEXT("The flag lands on the stored Definition so rerolls stay whole"), Affix.Definition.bWholeNumber);
    }
    return true;
}

#endif
