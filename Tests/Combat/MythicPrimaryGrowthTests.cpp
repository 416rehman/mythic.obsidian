// Copyright Stellar Games. All Rights Reserved.

#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"
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

#endif
