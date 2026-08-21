
#include "Misc/AutomationTest.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicXpScalingTest,
    "Mythic.Progression.XpScaling",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicXpScalingTest::RunTest(const FString &Parameters) {
    auto Compose = [](float Bonus, float Enlighten, float Rested) {
        return UMythicAttributeSet_Proficiencies::ComposeXpMultipliers(Bonus, Enlighten, Rested);
    };

    TestEqual(TEXT("nothing applied leaves xp alone"), Compose(0.0f, 0.0f, 1.0f), 1.0f);
    TestEqual(TEXT("the xp bonus stat adds"), Compose(0.25f, 0.0f, 1.0f), 1.25f);
    TestEqual(TEXT("enlighten adds"), Compose(0.0f, 0.5f, 1.0f), 1.5f);
    TestEqual(TEXT("the two stats add together rather than multiplying"), Compose(0.25f, 0.5f, 1.0f), 1.75f);

    // Rested multiplies the rest, so it is worth more to a character who already stacked xp bonus.
    TestEqual(TEXT("rested multiplies the total"), Compose(0.0f, 0.0f, 1.5f), 1.5f);
    TestEqual(TEXT("rested compounds with the stats"), Compose(0.25f, 0.5f, 1.5f), 2.625f);

    // Every one of these is a rolled or configured value, so a negative is reachable through bad data.
    TestEqual(TEXT("a negative stat cannot invert xp"), Compose(-5.0f, 0.0f, 1.0f), 0.0f);
    TestEqual(TEXT("a negative rested multiplier cannot invert xp"), Compose(0.5f, 0.0f, -2.0f), 0.0f);
    TestTrue(TEXT("no combination produces negative xp"), Compose(-3.0f, -3.0f, -3.0f) >= 0.0f);

    return true;
}
