
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Narrative/MythicQuestOutcome.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicQuestOutcomeTest,
    "Mythic.Narrative.QuestOutcome",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicQuestOutcomeTest::RunTest(const FString &Parameters) {
    const FGameplayTag TagA = GAS_EVENT_KILL;
    const FGameplayTag TagB = GAS_EVENT_TALKED_TO_NPC;
    const FGameplayTag TagC = GAS_EVENT_ITEM_ACQUIRED;

    auto MakeContainer = [](std::initializer_list<FGameplayTag> Tags) {
        FGameplayTagContainer C;
        for (const FGameplayTag &T : Tags) {
            C.AddTag(T);
        }
        return C;
    };

    auto MakeOutcome = [&](std::initializer_list<FGameplayTag> RequireAllTags) {
        FMythicQuestOutcome O;
        O.When.RequireAll = MakeContainer(RequireAllTags);
        return O;
    };

    const FGameplayTagContainer OwnA = MakeContainer({TagA});
    const FGameplayTagContainer OwnB = MakeContainer({TagB});
    const FGameplayTagContainer OwnC = MakeContainer({TagC});
    const FGameplayTagContainer OwnAB = MakeContainer({TagA, TagB});
    const FGameplayTagContainer Empty;

    {
        TArray<FMythicQuestOutcome> None;
        TestEqual(TEXT("empty outcome list → -1"), FMythicQuestOutcome::ResolveQuestOutcome(None, OwnA), INDEX_NONE);
    }

    {
        TArray<FMythicQuestOutcome> Outcomes = {MakeOutcome({TagA}), MakeOutcome({TagB})};
        TestEqual(TEXT("{A} → outcome 0"), FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, OwnA), 0);
        TestEqual(TEXT("{B} → outcome 1 (0 fails, no A)"), FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, OwnB), 1);
        TestEqual(TEXT("{C} → -1 (no gate passes, no default)"),
                  FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, OwnC), INDEX_NONE);
    }

    {
        TArray<FMythicQuestOutcome> Outcomes = {MakeOutcome({TagA}), MakeOutcome({TagB}), MakeOutcome({})};
        TestEqual(TEXT("{A} → 0 (gated wins over the later default)"),
                  FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, OwnA), 0);
        TestEqual(TEXT("{B} → 1"), FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, OwnB), 1);
        TestEqual(TEXT("{C} → 2 (falls through to the empty-When default)"),
                  FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, OwnC), 2);
        TestEqual(TEXT("{} → 2 (empty owned still hits the default)"),
                  FMythicQuestOutcome::ResolveQuestOutcome(Outcomes, Empty), 2);
    }

    {
        TArray<FMythicQuestOutcome> CatchAllFirst = {MakeOutcome({}), MakeOutcome({TagA})};
        TestEqual(TEXT("catch-all first → always 0 ({A})"),
                  FMythicQuestOutcome::ResolveQuestOutcome(CatchAllFirst, OwnA), 0);
        TestEqual(TEXT("catch-all first → always 0 ({C})"),
                  FMythicQuestOutcome::ResolveQuestOutcome(CatchAllFirst, OwnC), 0);

        TArray<FMythicQuestOutcome> AThenB = {MakeOutcome({TagA}), MakeOutcome({TagB})};
        TArray<FMythicQuestOutcome> BThenA = {MakeOutcome({TagB}), MakeOutcome({TagA})};
        TestEqual(TEXT("{A,B} with [A,B] → 0"), FMythicQuestOutcome::ResolveQuestOutcome(AThenB, OwnAB), 0);
        TestEqual(TEXT("{A,B} with [B,A] → 0 (order flips the winner)"),
                  FMythicQuestOutcome::ResolveQuestOutcome(BThenA, OwnAB), 0);
    }

    return true;
}
