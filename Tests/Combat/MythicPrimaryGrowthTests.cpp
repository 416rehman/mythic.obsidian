// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/Executions/MythicCombatRoll.h"
#include "GameModes/GameState/MythicGameState.h"
#include "Settings/MythicCombatSettings.h"

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

#endif
