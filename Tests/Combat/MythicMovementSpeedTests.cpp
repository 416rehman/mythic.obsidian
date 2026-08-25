
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "Settings/MythicCombatSettings.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicMovementSpeedAttributeTest,
    "Mythic.Combat.MovementSpeedAttribute",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicMovementSpeedAttributeTest::RunTest(const FString &Parameters) {
    UMythicAttributeSet_Utility *Utility = NewObject<UMythicAttributeSet_Utility>();
    if (!TestNotNull(TEXT("utility attribute set constructs"), Utility)) {
        return false;
    }

    TestTrue(TEXT("MovementSpeedMultiplier attribute resolves"), UMythicAttributeSet_Utility::GetMovementSpeedMultiplierAttribute().IsValid());
    TestEqual(TEXT("defaults to 1.0, which the sheet reads as 100%"), Utility->GetMovementSpeedMultiplier(), 1.0f);

    // The collapsed model: one speed attribute, so the sprint-only fraction must be gone from the set entirely.
    TArray<FGameplayAttribute> Attributes;
    Utility->GetAttributes(Attributes);
    bool bFoundSprintFraction = false;
    for (const FGameplayAttribute &Attribute : Attributes) {
        bFoundSprintFraction |= Attribute.GetName() == TEXT("BonusSprintSpeed");
    }
    TestTrue(TEXT("the sweep found the set's attributes"), Attributes.Num() > 5);
    TestFalse(TEXT("no second speed attribute survives on the set"), bFoundSprintFraction);

    const float Floor = MythicCombat::GetMinSpeedScale();
    TestTrue(TEXT("the authored floor is above a standstill"), Floor > 0.0f);

    auto Clamped = [Utility](float In) {
        float Value = In;
        Utility->PreAttributeChange(UMythicAttributeSet_Utility::GetMovementSpeedMultiplierAttribute(), Value);
        return Value;
    };

    TestEqual(TEXT("a buff passes through untouched"), Clamped(1.5f), 1.5f);
    TestEqual(TEXT("a heavy slow passes through untouched"), Clamped(0.25f), 0.25f);
    TestEqual(TEXT("stacked slows bottom out at the authored floor, not a standstill"), Clamped(0.0f), Floor);
    TestEqual(TEXT("an over-stacked slow cannot invert movement"), Clamped(-2.0f), Floor);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicSpeedCompositionTest,
    "Mythic.Combat.SpeedComposition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicSpeedCompositionTest::RunTest(const FString &Parameters) {
    const UMythicCombatSettings *Settings = GetDefault<UMythicCombatSettings>();
    if (!TestNotNull(TEXT("combat settings resolve"), Settings)) {
        return false;
    }

    const float Sprint = Settings->SprintSpeedMultiplier;
    const float Floor = MythicCombat::GetMinSpeedScale();

    TestTrue(TEXT("sprinting is authored as faster than walking"), Sprint > 1.0f);

    TestEqual(TEXT("an unmodified character walks at base speed"),
              MythicCombat::ComposeSpeedScale(1.0f, 1.0f, false), 1.0f);
    TestEqual(TEXT("sprinting pays the authored multiplier with no gear at all"),
              MythicCombat::ComposeSpeedScale(1.0f, 1.0f, true), Sprint);
    TestEqual(TEXT("the speed attribute scales the sprint as well as the walk"),
              MythicCombat::ComposeSpeedScale(2.0f, 1.0f, true), 2.0f * Sprint);
    TestEqual(TEXT("situational scales compose multiplicatively"),
              MythicCombat::ComposeSpeedScale(1.2f, 0.5f, false), 0.6f);
    TestEqual(TEXT("no stack of penalties reaches a standstill"),
              MythicCombat::ComposeSpeedScale(0.0f, 0.0f, false), Floor);
    TestEqual(TEXT("no stack of penalties inverts movement"),
              MythicCombat::ComposeSpeedScale(-3.0f, 2.0f, true), Floor);

    return true;
}
