
#include "Misc/AutomationTest.h"
#include "AI/Party/PartySubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyLoyaltyDeltaTest,
    "Mythic.Party.LoyaltyDelta",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyLoyaltyDeltaTest::RunTest(const FString &Parameters) {
    auto Delta = &UMythicPartySubsystem::ComputeLoyaltyDelta;
    const float Tol = 1.e-4f;

    TestEqual(TEXT("Hostile -> -0.10"), Delta(EMythicMoralSeverity::Hostile, 1.0f, 1.0f), -0.10f, Tol);
    TestEqual(TEXT("Condemn -> -0.06"), Delta(EMythicMoralSeverity::Condemn, 1.0f, 1.0f), -0.06f, Tol);
    TestEqual(TEXT("Disapprove -> -0.02"), Delta(EMythicMoralSeverity::Disapprove, 1.0f, 1.0f), -0.02f, Tol);

    TestEqual(TEXT("Ignore, no mercy -> 0.01"), Delta(EMythicMoralSeverity::Ignore, 0.0f, 1.0f), 0.01f, Tol);
    TestEqual(TEXT("Ignore, negative mercy -> 0.01 (mercy must be > 0)"),
              Delta(EMythicMoralSeverity::Ignore, -0.5f, 1.0f), 0.01f, Tol);

    TestEqual(TEXT("Ignore, mercy, Tend=0 -> 0.015"), Delta(EMythicMoralSeverity::Ignore, 0.5f, 0.0f), 0.015f, Tol);
    TestEqual(TEXT("Ignore, mercy, Tend=0.5 -> 0.030"), Delta(EMythicMoralSeverity::Ignore, 0.5f, 0.5f), 0.030f, Tol);
    TestEqual(TEXT("Ignore, mercy, Tend=1 -> 0.045"), Delta(EMythicMoralSeverity::Ignore, 0.5f, 1.0f), 0.045f, Tol);

    TestEqual(TEXT("Ignore, mercy exactly 0 -> 0.01 (not the mercy branch)"),
              Delta(EMythicMoralSeverity::Ignore, 0.0f, 0.0f), 0.01f, Tol);

    TestTrue(TEXT("higher Tend -> larger mercy reward"),
             Delta(EMythicMoralSeverity::Ignore, 1.0f, 1.0f) > Delta(EMythicMoralSeverity::Ignore, 1.0f, 0.0f));

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyRestRecoveryTest,
    "Mythic.Party.RestRecovery",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyRestRecoveryTest::RunTest(const FString &Parameters) {
    auto Rested = &UMythicPartySubsystem::ComputeRestedLoyalty;
    auto Decayed = &UMythicPartySubsystem::ComputeDecayedBetrayal;
    const float Tol = 1.e-4f;

    TestEqual(TEXT("interior recovery adds"), Rested(0.50f, 0.02f), 0.52f, Tol);
    TestEqual(TEXT("recovery clamps at 1.0"), Rested(0.99f, 0.02f), 1.0f, Tol);
    TestEqual(TEXT("already-full stays 1.0"), Rested(1.0f, 0.02f), 1.0f, Tol);
    TestEqual(TEXT("zero recovery is a no-op"), Rested(0.40f, 0.0f), 0.40f, Tol);

    TestEqual(TEXT("interior pressure cools"), Decayed(5.0f, 0.1f), 4.9f, Tol);
    TestEqual(TEXT("decay clamps at 0"), Decayed(0.05f, 0.1f), 0.0f, Tol);
    TestEqual(TEXT("already-zero stays 0"), Decayed(0.0f, 0.1f), 0.0f, Tol);
    TestEqual(TEXT("zero decay is a no-op"), Decayed(3.0f, 0.0f), 3.0f, Tol);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyBetrayalPressureGainTest,
    "Mythic.Party.BetrayalPressureGain",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyBetrayalPressureGainTest::RunTest(const FString &Parameters) {
    auto Gain = &UMythicPartySubsystem::ComputeBetrayalPressureGain;
    const float Tol = 1.e-4f;
    const float Trigger = -0.1f, Mult = 2.0f;

    TestEqual(TEXT("delta -0.2 -> 0.4"), Gain(-0.2f, Trigger, Mult), 0.4f, Tol);
    TestEqual(TEXT("delta -0.5 -> 1.0"), Gain(-0.5f, Trigger, Mult), 1.0f, Tol);

    TestEqual(TEXT("delta exactly -0.1 (not < -0.1) -> 0"), Gain(-0.1f, Trigger, Mult), 0.0f, Tol);
    TestEqual(TEXT("mild negative -0.05 -> 0"), Gain(-0.05f, Trigger, Mult), 0.0f, Tol);

    TestEqual(TEXT("positive delta +0.3 -> 0"), Gain(0.3f, Trigger, Mult), 0.0f, Tol);
    TestEqual(TEXT("zero delta -> 0"), Gain(0.0f, Trigger, Mult), 0.0f, Tol);

    TestEqual(TEXT("forgiving trigger -0.3 ignores -0.2"), Gain(-0.2f, -0.3f, Mult), 0.0f, Tol);
    TestEqual(TEXT("multiplier 3.0 on -0.2 -> 0.6"), Gain(-0.2f, Trigger, 3.0f), 0.6f, Tol);

    return true;
}
