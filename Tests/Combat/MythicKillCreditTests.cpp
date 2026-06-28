// Mythic — co-op shared kill-credit unit tests.
// Covers the pure eligibility rule the server-side XP distribution (UMythicLifeComponent::StartDeath) is built on: the
// killer is always credited; with sharing enabled (RangeSq > 0) any other player within range of the victim also gets
// credit. The live multi-player grant is server-driven + PIE-verified; this locks the decision.
// Run via: Session Frontend → Automation → Mythic.Combat.KillCredit

#include "Misc/AutomationTest.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicKillCreditTest,
    "Mythic.Combat.KillCredit",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicKillCreditTest::RunTest(const FString &Parameters) {
    auto Eligible = [&](bool bIsKiller, float DistSq, float RangeSq) {
        return UMythicLifeComponent::IsEligibleForSharedKillCredit(bIsKiller, DistSq, RangeSq);
    };

    // The killer is ALWAYS credited — even out of range, even with sharing disabled.
    TestTrue(TEXT("killer credited within range"), Eligible(true, 100.0f, 10000.0f));
    TestTrue(TEXT("killer credited out of range"), Eligible(true, 9999999.0f, 10000.0f));
    TestTrue(TEXT("killer credited with sharing off (RangeSq=0)"), Eligible(true, 0.0f, 0.0f));

    // A non-killer player shares only when sharing is enabled AND within range.
    TestTrue(TEXT("ally within range shares"), Eligible(false, 100.0f, 10000.0f));
    TestTrue(TEXT("ally exactly at the range boundary shares (inclusive)"), Eligible(false, 10000.0f, 10000.0f));
    TestFalse(TEXT("ally beyond range does not share"), Eligible(false, 10001.0f, 10000.0f));

    // Sharing disabled (RangeSq <= 0) is strictly killer-only — a non-killer never leeches, even at the exact victim spot.
    TestFalse(TEXT("sharing off: ally at the victim (DistSq=0) still does NOT leech"), Eligible(false, 0.0f, 0.0f));
    TestFalse(TEXT("sharing off: distant ally does not share"), Eligible(false, 500.0f, 0.0f));

    return true;
}
