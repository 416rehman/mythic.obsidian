
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Crime/MythicCrimeConsequenceSubsystem.h"
#include "World/LivingWorld/LivingWorldTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCrimeNotorietyTest,
    "Mythic.LivingWorld.CrimeNotoriety",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCrimeNotorietyTest::RunTest(const FString &Parameters) {
    const float Base = 15.0f;
    const float DIgnore = UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity::Ignore, Base);
    const float DDisapprove = UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity::Disapprove, Base);
    const float DCondemn = UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity::Condemn, Base);
    const float DHostile = UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity::Hostile, Base);

    TestEqual(TEXT("Ignore contributes zero notoriety"), DIgnore, 0.0f);
    TestNearlyEqual(TEXT("Disapprove = 0.5x base"), DDisapprove, 7.5f);
    TestNearlyEqual(TEXT("Condemn = 1x base"), DCondemn, 15.0f);
    TestNearlyEqual(TEXT("Hostile = 2x base"), DHostile, 30.0f);

    TestTrue(TEXT("Ignore <= Disapprove"), DIgnore <= DDisapprove);
    TestTrue(TEXT("Disapprove < Condemn"), DDisapprove < DCondemn);
    TestTrue(TEXT("Condemn < Hostile"), DCondemn < DHostile);

    TestNearlyEqual(TEXT("Condemn scales with base"),
                    UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity::Condemn, 30.0f), 30.0f);
    TestEqual(TEXT("Zero base -> zero delta"),
              UMythicCrimeConsequenceSubsystem::ComputeNotorietyDelta(EMythicMoralSeverity::Hostile, 0.0f), 0.0f);

    const float Threshold = 50.0f;
    TestFalse(TEXT("Below threshold -> no dispatch"),
              UMythicCrimeConsequenceSubsystem::ShouldDispatchGuards(49.0f, Threshold,false));
    TestTrue(TEXT("At threshold -> dispatch"),
             UMythicCrimeConsequenceSubsystem::ShouldDispatchGuards(50.0f, Threshold,false));
    TestTrue(TEXT("Above threshold -> dispatch"),
             UMythicCrimeConsequenceSubsystem::ShouldDispatchGuards(80.0f, Threshold,false));

    TestFalse(TEXT("Already dispatched -> no re-dispatch even above threshold"),
              UMythicCrimeConsequenceSubsystem::ShouldDispatchGuards(80.0f, Threshold,true));
    TestFalse(TEXT("Already dispatched AND below threshold -> no dispatch"),
              UMythicCrimeConsequenceSubsystem::ShouldDispatchGuards(10.0f, Threshold,true));

    return true;
}
