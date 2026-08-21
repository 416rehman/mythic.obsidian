
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"

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
    TestEqual(TEXT("defaults to 1.0 so an unmodified character moves at base speed"), Utility->GetMovementSpeedMultiplier(), 1.0f);

    auto Clamped = [Utility](float In) {
        float Value = In;
        Utility->PreAttributeChange(UMythicAttributeSet_Utility::GetMovementSpeedMultiplierAttribute(), Value);
        return Value;
    };

    TestEqual(TEXT("a buff passes through untouched"), Clamped(1.5f), 1.5f);
    TestEqual(TEXT("a heavy slow passes through untouched"), Clamped(0.25f), 0.25f);
    TestEqual(TEXT("stacked slows bottom out at a standstill"), Clamped(0.0f), 0.0f);
    TestEqual(TEXT("an over-stacked slow cannot invert movement"), Clamped(-2.0f), 0.0f);

    return true;
}
