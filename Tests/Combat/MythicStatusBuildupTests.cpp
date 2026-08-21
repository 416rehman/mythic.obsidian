
#include "Misc/AutomationTest.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStatusBuildupDecayTest,
    "Mythic.Combat.StatusBuildupDecay",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStatusBuildupDecayTest::RunTest(const FString &Parameters) {
    TestEqual(TEXT("decays decay×dt"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(40.0f, 10.0f, 0.5f), 35.0f);
    TestEqual(TEXT("clamps to 0 — never negative"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(3.0f, 10.0f, 0.5f), 0.0f);
    TestEqual(TEXT("exactly empties at the boundary"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(5.0f, 10.0f, 0.5f), 0.0f);
    TestEqual(TEXT("zero rate is a no-op (today's behaviour)"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(40.0f, 0.0f, 0.5f), 40.0f);
    TestEqual(TEXT("zero buildup stays 0"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(0.0f, 10.0f, 0.5f), 0.0f);
    TestEqual(TEXT("negative rate clamped to 0 (never ADDS buildup)"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(40.0f, -10.0f, 0.5f), 40.0f);
    TestEqual(TEXT("negative dt clamped to 0"), UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(40.0f, 10.0f, -0.5f), 40.0f);

    float B = 30.0f;
    for (int32 i = 0; i < 10; ++i) {
        B = UMythicAttributeSet_Defense::ComputeBuildupAfterDecay(B, 10.0f, 0.5f);
    }
    TestEqual(TEXT("a lone hit fully decays within 6 ticks (3s)"), B, 0.0f);

    return true;
}
