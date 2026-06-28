// Mythic — gathering proficiency XP unit tests.
// Covers the pure rule the harvest path uses to reward gathering XP (so the gathering proficiencies that drive bonus
// damage + bonus yield can actually climb from gathering). The live harvest→grant path is server-driven + PIE-verified;
// this locks the math, including the anti-grind cap.
// Run via: Session Frontend → Automation → Mythic.World.GatherXp

#include "Misc/AutomationTest.h"
#include "Resources/MythicResourceManagerComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicGatherXpTest,
    "Mythic.World.GatherXp",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicGatherXpTest::RunTest(const FString &Parameters) {
    // ComputeGatherXpReward(BaseXpPerHarvest, GathererLevel, NoGainAtOrAboveLevel)
    // = 0 if BaseXp<=0; 0 if NoGainAtOrAboveLevel>0 && GathererLevel>=it (anti-grind); else BaseXp.
    using R = UMythicResourceManagerComponent;

    TestEqual(TEXT("no reward when the config grants 0 XP (conservative default)"), R::ComputeGatherXpReward(0.0f, 3, 0), 0.0f);
    TestEqual(TEXT("base reward, no cap"), R::ComputeGatherXpReward(5.0f, 3, 0), 5.0f);
    TestEqual(TEXT("a novice (level 0) still earns XP and can climb from nothing"), R::ComputeGatherXpReward(5.0f, 0, 0), 5.0f);
    TestEqual(TEXT("anti-grind: at the cap level, no XP"), R::ComputeGatherXpReward(5.0f, 10, 10), 0.0f);
    TestEqual(TEXT("anti-grind: above the cap level, no XP"), R::ComputeGatherXpReward(5.0f, 25, 10), 0.0f);
    TestEqual(TEXT("just under the cap still pays"), R::ComputeGatherXpReward(5.0f, 9, 10), 5.0f);
    TestEqual(TEXT("cap 0 = no cap, pays at any level"), R::ComputeGatherXpReward(5.0f, 999, 0), 5.0f);
    TestEqual(TEXT("negative base XP → no reward"), R::ComputeGatherXpReward(-3.0f, 3, 0), 0.0f);

    return true;
}
