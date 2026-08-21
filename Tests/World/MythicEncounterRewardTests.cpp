
#include "Misc/AutomationTest.h"
#include "World/LivingWorld/Encounters/MythicEncounterObjectiveDefaults.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Rewards/LootReward.h"
#include "GAS/MythicTags_GAS.h"
#include "UObject/Package.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicEncounterRewardTest,
    "Mythic.World.EncounterReward",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicEncounterRewardTest::RunTest(const FString &Parameters) {
    UObject *Outer = GetTransientPackage();

    TestNull(TEXT("null outer yields null (never crashes)"),
             MythicEncounterObjectiveDefaults::BuildDefaultEncounterClearObjective(nullptr, 3));

    UObjectiveDefinition *Def = MythicEncounterObjectiveDefaults::BuildDefaultEncounterClearObjective(Outer, 4);
    TestNotNull(TEXT("a valid outer yields a definition"), Def);
    if (Def) {
        TestTrue(TEXT("triggers on the proven server-emitted kill event"), Def->TriggerEventTag == GAS_EVENT_KILL);
        TestEqual(TEXT("RequiredCount reflects the requested kill count"), Def->RequiredCount, 4);
        TestNotNull(TEXT("the code-default objective carries a default LootReward"), ToRawPtr(Def->Rewards.LootReward));
        TestNull(TEXT("no code-fabricated XP reward (needs an authored ProficiencyDefinition)"), ToRawPtr(Def->Rewards.XPReward));
        TestNull(TEXT("no code-fabricated Item reward (needs an authored ItemDefinition)"), ToRawPtr(Def->Rewards.ItemReward));
    }

    UObjectiveDefinition *ZeroDef = MythicEncounterObjectiveDefaults::BuildDefaultEncounterClearObjective(Outer, 0);
    TestNotNull(TEXT("zero kills still builds"), ZeroDef);
    if (ZeroDef) {
        TestEqual(TEXT("RequiredCount clamps up to 1"), ZeroDef->RequiredCount, 1);
    }
    UObjectiveDefinition *NegDef = MythicEncounterObjectiveDefaults::BuildDefaultEncounterClearObjective(Outer, -5);
    TestNotNull(TEXT("negative kills still builds"), NegDef);
    if (NegDef) {
        TestEqual(TEXT("negative RequiredCount clamps up to 1"), NegDef->RequiredCount, 1);
    }

    return true;
}
