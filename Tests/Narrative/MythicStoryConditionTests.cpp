
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Narrative/MythicStoryCondition.h"
#include "GAS/MythicTags_GAS.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicStoryConditionTest,
    "Mythic.Narrative.StoryCondition",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicStoryConditionTest::RunTest(const FString &Parameters) {
    const FGameplayTag TagA = GAS_EVENT_KILL;
    const FGameplayTag TagB = GAS_EVENT_TALKED_TO_NPC;
    const FGameplayTag TagC = GAS_EVENT_ITEM_ACQUIRED;
    const FGameplayTag TagD = GAS_EVENT_REACHED_LOCATION;

    auto MakeContainer = [](std::initializer_list<FGameplayTag> Tags) {
        FGameplayTagContainer C;
        for (const FGameplayTag &T : Tags) {
            C.AddTag(T);
        }
        return C;
    };

    const FGameplayTagContainer Empty;
    const FGameplayTagContainer OwnA = MakeContainer({TagA});
    const FGameplayTagContainer OwnAB = MakeContainer({TagA, TagB});
    const FGameplayTagContainer OwnABC = MakeContainer({TagA, TagB, TagC});

    TestTrue(TEXT("HasAll: {A,B} has all of {A,B}"), OwnAB.HasAll(MakeContainer({TagA, TagB})));
    TestFalse(TEXT("HasAll: {A} lacks all of {A,B}"), OwnA.HasAll(MakeContainer({TagA, TagB})));
    TestTrue(TEXT("HasAll: any set has all of {} (empty arg)"), OwnA.HasAll(Empty));
    TestTrue(TEXT("HasAny: {A,B} has any of {B,C}"), OwnAB.HasAny(MakeContainer({TagB, TagC})));
    TestFalse(TEXT("HasAny: {A} has none of {B,C}"), OwnA.HasAny(MakeContainer({TagB, TagC})));
    TestFalse(TEXT("HasAny: {A} has any of {} (empty arg) is false"), OwnA.HasAny(Empty));

    {
        FMythicStoryCondition C;
        TestTrue(TEXT("empty condition IsEmpty()"), C.IsEmpty());
        TestTrue(TEXT("empty condition passes on empty owned"), FMythicStoryCondition::Evaluate(C, Empty));
        TestTrue(TEXT("empty condition passes on non-empty owned"), FMythicStoryCondition::Evaluate(C, OwnABC));
    }

    {
        FMythicStoryCondition C;
        C.RequireAll = MakeContainer({TagA, TagB});
        TestFalse(TEXT("RequireAll not IsEmpty"), C.IsEmpty());
        TestTrue(TEXT("RequireAll {A,B} met by {A,B}"), FMythicStoryCondition::Evaluate(C, OwnAB));
        TestTrue(TEXT("RequireAll {A,B} met by superset {A,B,C}"), FMythicStoryCondition::Evaluate(C, OwnABC));
        TestFalse(TEXT("RequireAll {A,B} fails on {A} (missing B)"), FMythicStoryCondition::Evaluate(C, OwnA));
        TestFalse(TEXT("RequireAll {A,B} fails on {} "), FMythicStoryCondition::Evaluate(C, Empty));
    }

    {
        FMythicStoryCondition C;
        C.RequireAny = MakeContainer({TagC, TagD});
        TestTrue(TEXT("RequireAny {C,D} met by {A,B,C} (has C)"), FMythicStoryCondition::Evaluate(C, OwnABC));
        TestFalse(TEXT("RequireAny {C,D} fails on {A,B} (has neither)"), FMythicStoryCondition::Evaluate(C, OwnAB));
        TestFalse(TEXT("RequireAny {C,D} fails on {}"), FMythicStoryCondition::Evaluate(C, Empty));
    }

    {
        FMythicStoryCondition C;
        C.BlockAny = MakeContainer({TagC, TagD});
        TestTrue(TEXT("BlockAny {C,D} passes on {A,B} (neither present)"), FMythicStoryCondition::Evaluate(C, OwnAB));
        TestTrue(TEXT("BlockAny {C,D} passes on {}"), FMythicStoryCondition::Evaluate(C, Empty));
        TestFalse(TEXT("BlockAny {C,D} fails on {A,B,C} (C present)"), FMythicStoryCondition::Evaluate(C, OwnABC));
    }

    {
        FMythicStoryCondition C;
        C.RequireAll = MakeContainer({TagA});
        C.RequireAny = MakeContainer({TagB, TagC});
        C.BlockAny = MakeContainer({TagD});
        TestTrue(TEXT("combined: {A,B} passes (A + B, no D)"), FMythicStoryCondition::Evaluate(C, OwnAB));
        TestTrue(TEXT("combined: {A,B,C} passes"), FMythicStoryCondition::Evaluate(C, OwnABC));
        TestFalse(TEXT("combined: {A} fails RequireAny (no B/C)"), FMythicStoryCondition::Evaluate(C, OwnA));
        FGameplayTagContainer OwnABD = OwnAB;
        OwnABD.AddTag(TagD);
        TestFalse(TEXT("combined: {A,B,D} fails BlockAny (D present)"), FMythicStoryCondition::Evaluate(C, OwnABD));
    }

    return true;
}
