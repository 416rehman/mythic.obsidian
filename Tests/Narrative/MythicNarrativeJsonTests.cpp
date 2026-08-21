
#include "Misc/AutomationTest.h"
#include "Narrative/MythicNarrativeJson.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNarrativeJsonTest,
    "Mythic.Narrative.Json",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicNarrativeJsonTest::RunTest(const FString &Parameters) {
    const FString TaskJson = TEXT(R"json(
    {
      "id": "ashfang.confront",
      "display": "Confront the Ashfang leader",
      "trigger": "GAS.Event.Kill",
      "payload": "Faction.Ashfang.Leader",
      "count": 3,
      "optional": true,
      "precondition": {
        "requireAll": ["Story.Ashfang.Reached", "Story.Ashfang.Armed"],
        "requireAny": ["Story.Path.Warrior", "Story.Path.Rogue"],
        "blockAny": ["Story.Ashfang.Fled"]
      },
      "grantStoryTags": ["Story.Ashfang.Confronted"],
      "branches": [
        {
          "outcome": "Story.Outcome.Spared",
          "grantFlags": ["Story.Ashfang.Spared"],
          "next": ["ashfang.spared.followup"],
          "cancel": ["ashfang.killed.followup"]
        },
        {
          "outcome": "Story.Outcome.Killed",
          "grantFlags": ["Story.Ashfang.Dead"],
          "next": ["ashfang.killed.followup"],
          "cancel": ["ashfang.spared.followup"]
        }
      ],
      "next": ["ashfang.aftermath"]
    })json");

    FMythicTaskSpec Task;
    TestTrue(TEXT("ParseTaskSpec succeeds on well-formed task"), FMythicNarrativeJson::ParseTaskSpec(TaskJson, Task));
    TestEqual(TEXT("id"), Task.Id, FString(TEXT("ashfang.confront")));
    TestEqual(TEXT("display"), Task.Display, FString(TEXT("Confront the Ashfang leader")));
    TestEqual(TEXT("trigger"), Task.TriggerTag, FString(TEXT("GAS.Event.Kill")));
    TestEqual(TEXT("payload"), Task.PayloadTag, FString(TEXT("Faction.Ashfang.Leader")));
    TestEqual(TEXT("count"), Task.Count, 3);
    TestTrue(TEXT("optional"), Task.bOptional);

    TestEqual(TEXT("precondition.requireAll count"), Task.Precondition.RequireAll.Num(), 2);
    if (Task.Precondition.RequireAll.Num() == 2) {
        TestEqual(TEXT("precondition.requireAll[0]"), Task.Precondition.RequireAll[0], FString(TEXT("Story.Ashfang.Reached")));
        TestEqual(TEXT("precondition.requireAll[1]"), Task.Precondition.RequireAll[1], FString(TEXT("Story.Ashfang.Armed")));
    }
    TestEqual(TEXT("precondition.requireAny count"), Task.Precondition.RequireAny.Num(), 2);
    if (Task.Precondition.RequireAny.Num() == 2) {
        TestEqual(TEXT("precondition.requireAny[0]"), Task.Precondition.RequireAny[0], FString(TEXT("Story.Path.Warrior")));
        TestEqual(TEXT("precondition.requireAny[1]"), Task.Precondition.RequireAny[1], FString(TEXT("Story.Path.Rogue")));
    }
    TestEqual(TEXT("precondition.blockAny count"), Task.Precondition.BlockAny.Num(), 1);
    if (Task.Precondition.BlockAny.Num() == 1) {
        TestEqual(TEXT("precondition.blockAny[0]"), Task.Precondition.BlockAny[0], FString(TEXT("Story.Ashfang.Fled")));
    }

    TestEqual(TEXT("grantStoryTags count"), Task.GrantStoryTags.Num(), 1);
    if (Task.GrantStoryTags.Num() == 1) {
        TestEqual(TEXT("grantStoryTags[0]"), Task.GrantStoryTags[0], FString(TEXT("Story.Ashfang.Confronted")));
    }

    TestEqual(TEXT("branch count"), Task.Branches.Num(), 2);
    if (Task.Branches.Num() == 2) {
        TestEqual(TEXT("branch0.outcome"), Task.Branches[0].Outcome, FString(TEXT("Story.Outcome.Spared")));
        TestEqual(TEXT("branch0.grantFlags count"), Task.Branches[0].GrantFlags.Num(), 1);
        TestEqual(TEXT("branch0.grantFlags[0]"), Task.Branches[0].GrantFlags[0], FString(TEXT("Story.Ashfang.Spared")));
        TestEqual(TEXT("branch0.next[0]"), Task.Branches[0].Next[0], FString(TEXT("ashfang.spared.followup")));
        TestEqual(TEXT("branch0.cancel[0]"), Task.Branches[0].Cancel[0], FString(TEXT("ashfang.killed.followup")));

        TestEqual(TEXT("branch1.outcome"), Task.Branches[1].Outcome, FString(TEXT("Story.Outcome.Killed")));
        TestEqual(TEXT("branch1.grantFlags[0]"), Task.Branches[1].GrantFlags[0], FString(TEXT("Story.Ashfang.Dead")));
        TestEqual(TEXT("branch1.next[0]"), Task.Branches[1].Next[0], FString(TEXT("ashfang.killed.followup")));
        TestEqual(TEXT("branch1.cancel[0]"), Task.Branches[1].Cancel[0], FString(TEXT("ashfang.spared.followup")));
    }

    TestEqual(TEXT("next count"), Task.Next.Num(), 1);
    if (Task.Next.Num() == 1) {
        TestEqual(TEXT("next[0]"), Task.Next[0], FString(TEXT("ashfang.aftermath")));
    }

    {
        FMythicTaskSpec Dummy;
        TestFalse(TEXT("truncated json → false"),
                  FMythicNarrativeJson::ParseTaskSpec(TEXT("{\"id\":\"x\",\"count\":"), Dummy));
        TestFalse(TEXT("json array (wrong root type) → false"),
                  FMythicNarrativeJson::ParseTaskSpec(TEXT("[1,2,3]"), Dummy));
        TestFalse(TEXT("empty string → false"), FMythicNarrativeJson::ParseTaskSpec(TEXT(""), Dummy));
        TestFalse(TEXT("object without id → false"),
                  FMythicNarrativeJson::ParseTaskSpec(TEXT("{\"display\":\"no id\"}"), Dummy));
        TestFalse(TEXT("bare number (wrong root type) → false"),
                  FMythicNarrativeJson::ParseTaskSpec(TEXT("42"), Dummy));
    }

    {
        FMythicTaskSpec Spec;
        Spec.Id = TEXT("rt.task");
        Spec.Display = TEXT("Round trip me");
        Spec.TriggerTag = TEXT("GAS.Event.Item.Acquired");
        Spec.PayloadTag = TEXT("Itemization.Type.Resource.Wood");
        Spec.Count = 20;
        Spec.bOptional = true;
        Spec.Precondition.RequireAll = {TEXT("A"), TEXT("B")};
        Spec.Precondition.RequireAny = {TEXT("C")};
        Spec.Precondition.BlockAny = {TEXT("D"), TEXT("E")};
        Spec.GrantStoryTags = {TEXT("Story.Did.It")};

        FMythicBranchSpec B0;
        B0.Outcome = TEXT("Story.Outcome.Spared");
        B0.GrantFlags = {TEXT("F")};
        B0.Next = {TEXT("n1"), TEXT("n2")};
        B0.Cancel = {TEXT("c1")};
        FMythicBranchSpec B1;
        B1.Outcome = TEXT("Story.Outcome.Killed");
        B1.Next = {TEXT("n3")};
        Spec.Branches = {B0, B1};
        Spec.Next = {TEXT("after")};

        const FString Json = FMythicNarrativeJson::SerializeTaskSpec(Spec);
        FMythicTaskSpec RoundTripped;
        TestTrue(TEXT("round-trip parse succeeds"), FMythicNarrativeJson::ParseTaskSpec(Json, RoundTripped));
        TestTrue(TEXT("round-trip spec equals original (field-by-field ==)"), RoundTripped == Spec);

        FMythicTaskSpec Minimal;
        Minimal.Id = TEXT("min");
        FMythicTaskSpec MinRT;
        TestTrue(TEXT("minimal round-trip parse succeeds"),
                 FMythicNarrativeJson::ParseTaskSpec(FMythicNarrativeJson::SerializeTaskSpec(Minimal), MinRT));
        TestTrue(TEXT("minimal round-trip equals original"), MinRT == Minimal);
    }

    {
        const FString StoryJson = TEXT(R"json(
        {
          "id": "arc.test",
          "arcTag": "Story.Arc.Test",
          "quests": [
            {
              "id": "q1",
              "tasks": [
                { "id": "t1", "branches": [ { "outcome": "Story.Outcome.Spared", "next": ["t2"], "cancel": ["t3"] } ] }
              ]
            },
            {
              "id": "q2",
              "tasks": [
                { "id": "t2" },
                { "id": "t3" }
              ]
            }
          ]
        })json");

        FMythicStorylineSpec Story;
        TestTrue(TEXT("ParseStorylineSpec succeeds"), FMythicNarrativeJson::ParseStorylineSpec(StoryJson, Story));
        TestEqual(TEXT("arc id"), Story.Id, FString(TEXT("arc.test")));
        TestEqual(TEXT("arcTag"), Story.ArcTag, FString(TEXT("Story.Arc.Test")));
        TestEqual(TEXT("quest count"), Story.Quests.Num(), 2);
        if (Story.Quests.Num() == 2) {
            TestEqual(TEXT("q1 id"), Story.Quests[0].Id, FString(TEXT("q1")));
            TestEqual(TEXT("q1 task count"), Story.Quests[0].Tasks.Num(), 1);
            TestEqual(TEXT("q2 id"), Story.Quests[1].Id, FString(TEXT("q2")));
            TestEqual(TEXT("q2 task count"), Story.Quests[1].Tasks.Num(), 2);

            const FMythicTaskSpec &T1 = Story.Quests[0].Tasks[0];
            TestEqual(TEXT("t1 id"), T1.Id, FString(TEXT("t1")));
            TestEqual(TEXT("t1 branch count"), T1.Branches.Num(), 1);
            if (T1.Branches.Num() == 1) {
                TestEqual(TEXT("t1 branch next → t2"), T1.Branches[0].Next.Num() == 1 ? T1.Branches[0].Next[0] : FString(),
                          FString(TEXT("t2")));
                TestEqual(TEXT("t1 branch cancel → t3"),
                          T1.Branches[0].Cancel.Num() == 1 ? T1.Branches[0].Cancel[0] : FString(), FString(TEXT("t3")));
            }
        }
    }

    return true;
}
