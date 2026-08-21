
#include "Misc/AutomationTest.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Proficiencies.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicWorldTierXpTest,
    "Mythic.Progression.WorldTierXp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicWorldTierXpTest::RunTest(const FString &Parameters) {
    using P = UMythicAttributeSet_Proficiencies;

    TestEqual(TEXT("multiplier 1.0 is a no-op"), P::ApplyWorldTierXpMultiplier(100.0f, 1.0f), 100.0f);

    TestEqual(TEXT("1.5x world-tier rate scales up"), P::ApplyWorldTierXpMultiplier(100.0f, 1.5f), 150.0f);
    TestEqual(TEXT("2x world-tier rate doubles"), P::ApplyWorldTierXpMultiplier(80.0f, 2.0f), 160.0f);

    TestEqual(TEXT("0.5x rate halves"), P::ApplyWorldTierXpMultiplier(100.0f, 0.5f), 50.0f);

    TestEqual(TEXT("zero multiplier (uninitialized) does NOT zero XP"), P::ApplyWorldTierXpMultiplier(100.0f, 0.0f), 100.0f);
    TestEqual(TEXT("negative multiplier is treated as 1.0"), P::ApplyWorldTierXpMultiplier(100.0f, -3.0f), 100.0f);

    TestEqual(TEXT("no base XP stays zero"), P::ApplyWorldTierXpMultiplier(0.0f, 2.0f), 0.0f);

    return true;
}
