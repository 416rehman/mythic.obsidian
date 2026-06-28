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
