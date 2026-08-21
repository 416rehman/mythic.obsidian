
#include "Misc/AutomationTest.h"
#include "Mass/Processors/WitnessPerceptionProcessor.h"
#include "AI/Party/PartySubsystem.h"


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicLivingWorldStealthTest,
    "Mythic.LivingWorld.Stealth",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicLivingWorldStealthTest::RunTest(const FString &Parameters) {
    constexpr float BaseRange = 10.0f;

    TestEqual(TEXT("Scale 1.0 leaves the range unchanged"),
              UMythicWitnessPerceptionProcessor::ComputeStealthPerceptionRange(BaseRange, 1.0f), BaseRange);

    const float Sneaking = UMythicWitnessPerceptionProcessor::ComputeStealthPerceptionRange(BaseRange, 0.4f);
    TestTrue(TEXT("Scale 0.4 shrinks the range below Base"), Sneaking < BaseRange);
    TestEqual(TEXT("Scale 0.4 scales the range proportionally"), Sneaking, BaseRange * 0.4f);

    const float Deeper = UMythicWitnessPerceptionProcessor::ComputeStealthPerceptionRange(BaseRange, 0.2f);
    TestTrue(TEXT("A deeper stealth scale shrinks the range further"), Deeper < Sneaking);

    TestEqual(TEXT("Scale > 1.0 clamps to Base (never amplifies)"),
              UMythicWitnessPerceptionProcessor::ComputeStealthPerceptionRange(BaseRange, 2.0f), BaseRange);
    TestEqual(TEXT("Scale 0.0 yields 0 range"),
              UMythicWitnessPerceptionProcessor::ComputeStealthPerceptionRange(BaseRange, 0.0f), 0.0f);

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicPartyCompanionOrdersTest,
    "Mythic.Party.CompanionOrders",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicPartyCompanionOrdersTest::RunTest(const FString &Parameters) {
    constexpr float TriggerDelta = -0.1f;
    constexpr float BaseMultiplier = 2.0f;
    constexpr float ForcedScale = 2.0f;

    const float BadDelta = -0.20f;
    const float OrganicGain = UMythicPartySubsystem::ComputeBetrayalPressureGain(BadDelta, TriggerDelta, BaseMultiplier);
    const float ForcedGain = UMythicPartySubsystem::ComputeForcedComplianceGain(BadDelta, TriggerDelta, BaseMultiplier, ForcedScale);

    TestTrue(TEXT("Organic betrayal gain is non-zero for a conflicting act"), OrganicGain > 0.0f);
    TestTrue(TEXT("Forced-compliance gain >= organic gain for a conflicting order"), ForcedGain >= OrganicGain);
    TestTrue(TEXT("Forced-compliance gain strictly exceeds organic gain when scale > 1"), ForcedGain > OrganicGain);
    TestEqual(TEXT("Forced gain equals organic gain scaled by ForcedComplianceScale"), ForcedGain, OrganicGain * ForcedScale);

    TestEqual(TEXT("Forced gain == organic gain at scale 1.0"),
              UMythicPartySubsystem::ComputeForcedComplianceGain(BadDelta, TriggerDelta, BaseMultiplier, 1.0f), OrganicGain);

    const float BenignDelta = 0.02f;
    TestEqual(TEXT("Non-conflicting order builds no organic pressure"),
              UMythicPartySubsystem::ComputeBetrayalPressureGain(BenignDelta, TriggerDelta, BaseMultiplier), 0.0f);
    TestEqual(TEXT("Non-conflicting order builds no forced pressure"),
              UMythicPartySubsystem::ComputeForcedComplianceGain(BenignDelta, TriggerDelta, BaseMultiplier, ForcedScale), 0.0f);

    return true;
}
