// Mythic Party — Companion loyalty-balance unit tests
// Covers: UMythicPartySubsystem::ComputeLoyaltyDelta (the pure severity -> loyalty-delta curve extracted from
//         EvaluateLoyaltyImpact so the balance values are regression-locked).
// Run via: Session Frontend -> Automation -> Mythic.Party

#include "Misc/AutomationTest.h"
#include "AI/Party/PartySubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h" // EMythicMoralSeverity

// ═══════════════════════════════════════════════════════════════
// Companion loyalty delta — UMythicPartySubsystem::ComputeLoyaltyDelta
// (severity-scaled penalties for violations; mild approval for benign acts; a merciful act scales with empathy/Tend)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyLoyaltyDeltaTest,
    "Mythic.Party.LoyaltyDelta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyLoyaltyDeltaTest::RunTest(const FString &Parameters) {
    auto Delta = &UMythicPartySubsystem::ComputeLoyaltyDelta;
    const float Tol = 1.e-4f;

    // Violations: severity-scaled penalties, INDEPENDENT of the mercy/tend inputs (severity already encodes the
    // companion's faction judgement). Pass strong mercy + empathy to prove they do not soften a violation.
    TestEqual(TEXT("Hostile -> -0.10"), Delta(EMythicMoralSeverity::Hostile, 1.0f, 1.0f), -0.10f, Tol);
    TestEqual(TEXT("Condemn -> -0.06"), Delta(EMythicMoralSeverity::Condemn, 1.0f, 1.0f), -0.06f, Tol);
    TestEqual(TEXT("Disapprove -> -0.02"), Delta(EMythicMoralSeverity::Disapprove, 1.0f, 1.0f), -0.02f, Tol);

    // Benign (Ignore) + no mercy: mild flat approval regardless of empathy.
    TestEqual(TEXT("Ignore, no mercy -> 0.01"), Delta(EMythicMoralSeverity::Ignore, 0.0f, 1.0f), 0.01f, Tol);
    TestEqual(TEXT("Ignore, negative mercy -> 0.01 (mercy must be > 0)"),
              Delta(EMythicMoralSeverity::Ignore, -0.5f, 1.0f), 0.01f, Tol);

    // Benign (Ignore) + mercy: 0.03 * (0.5 + TendWeight) — scales with the companion's empathy.
    TestEqual(TEXT("Ignore, mercy, Tend=0 -> 0.015"), Delta(EMythicMoralSeverity::Ignore, 0.5f, 0.0f), 0.015f, Tol);
    TestEqual(TEXT("Ignore, mercy, Tend=0.5 -> 0.030"), Delta(EMythicMoralSeverity::Ignore, 0.5f, 0.5f), 0.030f, Tol);
    TestEqual(TEXT("Ignore, mercy, Tend=1 -> 0.045"), Delta(EMythicMoralSeverity::Ignore, 0.5f, 1.0f), 0.045f, Tol);

    // The mercy reward is gated strictly on mercy > 0 (exact-zero mercy is NOT rewarded as mercy).
    TestEqual(TEXT("Ignore, mercy exactly 0 -> 0.01 (not the mercy branch)"),
              Delta(EMythicMoralSeverity::Ignore, 0.0f, 0.0f), 0.01f, Tol);

    // An empathetic companion rewards mercy MORE than a callous one (monotonic in Tend).
    TestTrue(TEXT("higher Tend -> larger mercy reward"),
             Delta(EMythicMoralSeverity::Ignore, 1.0f, 1.0f) > Delta(EMythicMoralSeverity::Ignore, 1.0f, 0.0f));

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Rest-phase recovery — UMythicPartySubsystem::ComputeRestedLoyalty / ComputeDecayedBetrayal
// (campfire bonding adds loyalty up to the 1.0 ceiling; betrayal pressure cools off down to the 0 floor)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyRestRecoveryTest,
    "Mythic.Party.RestRecovery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyRestRecoveryTest::RunTest(const FString &Parameters) {
    auto Rested = &UMythicPartySubsystem::ComputeRestedLoyalty;
    auto Decayed = &UMythicPartySubsystem::ComputeDecayedBetrayal;
    const float Tol = 1.e-4f;

    // Loyalty recovery: linear in the interior, clamped to the 1.0 ceiling (never exceeds full loyalty).
    TestEqual(TEXT("interior recovery adds"), Rested(0.50f, 0.02f), 0.52f, Tol);
    TestEqual(TEXT("recovery clamps at 1.0"), Rested(0.99f, 0.02f), 1.0f, Tol);
    TestEqual(TEXT("already-full stays 1.0"), Rested(1.0f, 0.02f), 1.0f, Tol);
    TestEqual(TEXT("zero recovery is a no-op"), Rested(0.40f, 0.0f), 0.40f, Tol);

    // Betrayal decay: linear in the interior, clamped to the 0 floor (never negative).
    TestEqual(TEXT("interior pressure cools"), Decayed(5.0f, 0.1f), 4.9f, Tol);
    TestEqual(TEXT("decay clamps at 0"), Decayed(0.05f, 0.1f), 0.0f, Tol);
    TestEqual(TEXT("already-zero stays 0"), Decayed(0.0f, 0.1f), 0.0f, Tol);
    TestEqual(TEXT("zero decay is a no-op"), Decayed(3.0f, 0.0f), 3.0f, Tol);

    return true;
}

// ═══════════════════════════════════════════════════════════════
// Betrayal build-up — UMythicPartySubsystem::ComputeBetrayalPressureGain
// (only acts that damage loyalty BELOW the trigger threshold build pressure, scaled by |delta| * multiplier)
// ═══════════════════════════════════════════════════════════════

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyBetrayalPressureGainTest,
    "Mythic.Party.BetrayalPressureGain",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyBetrayalPressureGainTest::RunTest(const FString &Parameters) {
    auto Gain = &UMythicPartySubsystem::ComputeBetrayalPressureGain;
    const float Tol = 1.e-4f;
    const float Trigger = -0.1f, Mult = 2.0f; // current defaults

    // Below the trigger (sufficiently objectionable): pressure scales with |delta| * multiplier.
    TestEqual(TEXT("delta -0.2 -> 0.4"), Gain(-0.2f, Trigger, Mult), 0.4f, Tol);
    TestEqual(TEXT("delta -0.5 -> 1.0"), Gain(-0.5f, Trigger, Mult), 1.0f, Tol);

    // At/above the trigger: no pressure (forgiving region). Boundary is STRICT (< trigger).
    TestEqual(TEXT("delta exactly -0.1 (not < -0.1) -> 0"), Gain(-0.1f, Trigger, Mult), 0.0f, Tol);
    TestEqual(TEXT("mild negative -0.05 -> 0"), Gain(-0.05f, Trigger, Mult), 0.0f, Tol);

    // Aligned / positive acts never build pressure.
    TestEqual(TEXT("positive delta +0.3 -> 0"), Gain(0.3f, Trigger, Mult), 0.0f, Tol);
    TestEqual(TEXT("zero delta -> 0"), Gain(0.0f, Trigger, Mult), 0.0f, Tol);

    // Tuning levers behave: a more forgiving trigger (-0.3) ignores a -0.2 act the default would punish.
    TestEqual(TEXT("forgiving trigger -0.3 ignores -0.2"), Gain(-0.2f, -0.3f, Mult), 0.0f, Tol);
    // A higher multiplier scales the gain.
    TestEqual(TEXT("multiplier 3.0 on -0.2 -> 0.6"), Gain(-0.2f, Trigger, 3.0f), 0.6f, Tol);

    return true;
}
