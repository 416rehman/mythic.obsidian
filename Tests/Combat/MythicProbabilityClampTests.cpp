
#include "Misc/AutomationTest.h"

#include "GAS/Executions/MythicCombatRoll.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicProbabilityClampTest,
    "Mythic.Combat.ProbabilityClamp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicProbabilityClampTest::RunTest(const FString &Parameters) {
    using namespace MythicCombat;

    TestEqual(TEXT("a chance inside the range is untouched"), ClampProbability(0.3f, 0.75f), 0.3f);
    TestEqual(TEXT("a stacked chance is capped"), ClampProbability(1.7f), 1.0f);
    TestEqual(TEXT("a stacked chance is capped to a lower ceiling"), ClampProbability(1.7f, 0.75f), 0.75f);
    TestEqual(TEXT("a negative chance floors at zero"), ClampProbability(-0.5f), 0.0f);
    TestEqual(TEXT("a ceiling above 1 cannot raise the cap"), ClampProbability(5.0f, 5.0f), 1.0f);
    TestEqual(TEXT("a ceiling of zero disables the roll"), ClampProbability(0.9f, 0.0f), 0.0f);

    // Affix pools store these as fractions and have already shipped tiers meaning several hundred percent, so a
    // captured magnitude well above 1 is reachable, not hypothetical.
    const float StackedDodge = 12.0f;
    TestTrue(TEXT("unclamped, a stacked chance beats even the highest roll"), RollSucceeds(StackedDodge, 0.99f));
    TestFalse(TEXT("clamped to the dodge ceiling, the highest roll gets through"),
              RollSucceeds(ClampProbability(StackedDodge, 0.75f), 0.99f));

    // A ceiling of exactly 1 is still total immunity, which is why dodge must cap below it.
    TestTrue(TEXT("a chance of 1 always succeeds"), RollSucceeds(ClampProbability(1.0f), 0.999999f));

    TestFalse(TEXT("a chance of zero never succeeds"), RollSucceeds(ClampProbability(0.0f), 0.0f));

    return true;
}
