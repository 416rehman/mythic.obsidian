// Mythic — crafting proficiency-scaled product level unit tests.
// Covers the pure rule the ProficiencyScaled conversion mode uses: a crafter's proficiency level raises the crafted
// item's level off a base, capped. The live snapshot+produce path is server-driven and PIE-verified; this locks the math.
// Run via: Session Frontend → Automation → Mythic.Itemization.Conversion.ProficiencyScaled

#include "Misc/AutomationTest.h"
#include "Itemization/Conversion/ConversionStationComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCraftingProficiencyTest,
    "Mythic.Itemization.Conversion.ProficiencyScaled",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCraftingProficiencyTest::RunTest(const FString &Parameters) {
    // ComputeProficiencyScaledLevel(CrafterProfLevel, BaseLevel, PerLevelBonus, MaxLevel)
    // = clamp_lo0(BaseLevel + max(0,prof) × max(0,bonus)), then cap at MaxLevel when MaxLevel > 0.
    using S = UConversionStationComponent;

    TestEqual(TEXT("a novice (level 0) gets just the base"), S::ComputeProficiencyScaledLevel(0, 5, 2, 0), 5);
    TestEqual(TEXT("scales: base 5 + 10×2 = 25"), S::ComputeProficiencyScaledLevel(10, 5, 2, 0), 25);
    TestEqual(TEXT("capped at MaxLevel"), S::ComputeProficiencyScaledLevel(10, 5, 2, 20), 20);
    TestEqual(TEXT("MaxLevel 0 = uncapped"), S::ComputeProficiencyScaledLevel(100, 1, 1, 0), 101);
    TestEqual(TEXT("negative proficiency clamps to 0 (base only)"), S::ComputeProficiencyScaledLevel(-5, 5, 2, 0), 5);
    TestEqual(TEXT("negative bonus clamps to 0 (no scaling)"), S::ComputeProficiencyScaledLevel(10, 5, -2, 0), 5);
    TestEqual(TEXT("a base above the cap is still capped"), S::ComputeProficiencyScaledLevel(0, 50, 1, 10), 10);
    TestEqual(TEXT("exact-at-cap"), S::ComputeProficiencyScaledLevel(5, 10, 2, 20), 20);

    // Overflow-hardening: an absurd PerLevelBonus that would overflow int32 in 32-bit math must still respect the cap
    // (monotonic), NOT wrap negative and floor to 0. With the int64 intermediate, prof×bonus is clamped, then capped.
    TestEqual(TEXT("huge bonus respects the cap, not wrap-to-0"), S::ComputeProficiencyScaledLevel(30, 5, 100000000, 50), 50);
    TestEqual(TEXT("huge bonus uncapped clamps to MAX_int32, not negative"), S::ComputeProficiencyScaledLevel(30, 5, 100000000, 0), MAX_int32);

    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicCraftingXpTest,
    "Mythic.Itemization.Conversion.CraftingXp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicCraftingXpTest::RunTest(const FString &Parameters) {
    // ComputeCraftingXpReward(BaseXpPerCraft, Cycles, CrafterLevel, NoGainAtOrAboveLevel)
    // = 0 if BaseXp<=0 or Cycles<=0; 0 if NoGainAtOrAboveLevel>0 && CrafterLevel>=it (anti-grind); else BaseXp×Cycles.
    using S = UConversionStationComponent;

    TestEqual(TEXT("no reward when the recipe grants 0 XP (conservative default)"), S::ComputeCraftingXpReward(0.0f, 1, 5, 0), 0.0f);
    TestEqual(TEXT("base reward, one cycle, no cap"), S::ComputeCraftingXpReward(10.0f, 1, 5, 0), 10.0f);
    TestEqual(TEXT("scales with cycles"), S::ComputeCraftingXpReward(10.0f, 3, 5, 0), 30.0f);
    TestEqual(TEXT("anti-grind: at the cap level, no XP"), S::ComputeCraftingXpReward(10.0f, 1, 10, 10), 0.0f);
    TestEqual(TEXT("anti-grind: above the cap level, no XP"), S::ComputeCraftingXpReward(10.0f, 1, 25, 10), 0.0f);
    TestEqual(TEXT("just under the cap still pays"), S::ComputeCraftingXpReward(10.0f, 1, 9, 10), 10.0f);
    TestEqual(TEXT("cap 0 = no cap, pays at any level"), S::ComputeCraftingXpReward(10.0f, 1, 999, 0), 10.0f);
    TestEqual(TEXT("zero cycles → no reward"), S::ComputeCraftingXpReward(10.0f, 0, 5, 0), 0.0f);
    TestEqual(TEXT("negative cycles → no reward"), S::ComputeCraftingXpReward(10.0f, -2, 5, 0), 0.0f);

    return true;
}
