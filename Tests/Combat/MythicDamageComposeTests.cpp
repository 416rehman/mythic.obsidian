#include "Misc/AutomationTest.h"

#include "GAS/Executions/MythicDamageCompose.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDamageComposeTest,
    "Mythic.Combat.DamageCompose",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDamageComposeTest::RunTest(const FString &Parameters) {
    using Composer = FMythicDamageComposer;

    // No modifiers leaves the base untouched.
    TestTrue(TEXT("no modifiers leaves base damage unchanged"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 0.0f, {}, 0.0f), 100.0f));

    // Increased is ADDITIVE: the caller sums the bucket, and it lifts the base once. Two +50% Increased => x2.0.
    TestTrue(TEXT("summed Increased is additive (two +50% -> x2.0)"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 1.0f, {}, 0.0f), 200.0f));

    // More is MULTIPLICATIVE: two +50% More compound to x2.25, NOT x2.0 - the whole point of the two buckets.
    const TArray<float> TwoMore = {0.5f, 0.5f};
    TestTrue(TEXT("stacked More is multiplicative (two +50% -> x2.25)"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 0.0f, TwoMore, 0.0f), 225.0f));

    // The distinction holds when both are present: +50% Increased and +50% More is 1.5 * 1.5, not 2.0.
    const TArray<float> OneMore = {0.5f};
    TestTrue(TEXT("Increased and More compose separately (1.5 * 1.5 = 2.25x)"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 0.5f, OneMore, 0.0f), 225.0f));

    // A positive MoreStackCap clamps only the More product: two +200% More would be x9, capped to x4.
    const TArray<float> BigMore = {2.0f, 2.0f};
    TestTrue(TEXT("a positive More cap clamps the More product"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 0.0f, BigMore, 4.0f), 400.0f));

    // A cap of zero means uncapped, so the same stack rides free to x9.
    TestTrue(TEXT("a zero cap leaves the More product uncapped"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 0.0f, BigMore, 0.0f), 900.0f));

    // The cap never lifts a small product up to it - it is a ceiling, not a target.
    TestTrue(TEXT("the cap is a ceiling, not a floor"),
             FMath::IsNearlyEqual(Composer::ComposeDamage(100.0f, 0.0f, OneMore, 4.0f), 150.0f));

    return true;
}
