
#include "Misc/AutomationTest.h"
#include "Engine/GameInstance.h"
#include "Narrative/Dialogue/MythicDialogueJson.h"
#include "Narrative/MythicNarrativeJson.h"
#include "Narrative/MythicNarrativeImportSubsystem.h"
#include "Narrative/Dialogue/MythicDialogueGraphTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueJsonTest,
    "Mythic.Narrative.DialogueJson",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueJsonTest::RunTest(const FString &Parameters) {
    const FString GraphJson = TEXT(R"json(
    {
      "kind": "dialogue",
      "id": "ashfang.leader.dialogue",
      "npcTag": "Faction.Ashfang.Leader",
      "role": "NPC.Role.Warchief",
      "faction": "Faction.Ashfang",
      "entryNodeId": "confrontation",
      "nodes": [
        {
          "id": "confrontation",
          "speaker": "Faction.Ashfang.Leader",
          "line": "Strike, or speak.",
          "entryCondition": {
            "requireAll": ["Story.Ashfang.Reached"],
            "requireAny": [],
            "blockAny": ["Story.Ashfang.Spared", "Story.Ashfang.Dead"]
          },
          "choices": [
            {
              "text": "Spare him.",
              "condition": {
                "requireAll": [],
                "requireAny": [],
                "blockAny": ["Story.Ashfang.Spared", "Story.Ashfang.Dead"]
              },
              "grantTags": ["Story.Ashfang.Spared"],
              "rewards": {
                "xpPercentage": 0.5,
                "xpProficiency": "/Game/Prof/Prof_Diplomacy.Prof_Diplomacy",
                "itemId": "/Game/Items/Item_PeaceToken.Item_PeaceToken",
                "itemQuantity": 1
              },
              "questOfferId": "ashfang.mercy",
              "gotoNodeId": "spared-followup",
              "endsDialogue": false
            },
            {
              "text": "Execute him.",
              "condition": {
                "requireAll": [],
                "requireAny": ["Renown.Global.Honored", "Renown.Global.Exalted"],
                "blockAny": ["Story.Ashfang.Dead", "Story.Ashfang.Spared"]
              },
              "grantTags": ["Story.Ashfang.Dead"],
              "rewards": {
                "xpPercentage": 0,
                "xpProficiency": "",
                "itemId": "",
                "itemQuantity": 0
              },
              "questOfferId": "ashfang.vengeance",
              "gotoNodeId": "",
              "endsDialogue": true
            }
          ]
        },
        {
          "id": "spared-followup",
          "speaker": "Faction.Ashfang.Leader",
          "line": "The Ashfang owe you a debt.",
          "entryCondition": {
            "requireAll": ["Story.Ashfang.Spared"],
            "requireAny": [],
            "blockAny": []
          },
          "choices": []
        }
      ]
    })json");

    FMythicDialogueGraphSpec Graph;
    TestTrue(TEXT("ParseGraphSpec succeeds on well-formed graph"), FMythicDialogueJson::ParseGraphSpec(GraphJson, Graph));
    TestEqual(TEXT("graph.id"), Graph.Id, FString(TEXT("ashfang.leader.dialogue")));
    TestEqual(TEXT("graph.npcTag"), Graph.NpcTag, FString(TEXT("Faction.Ashfang.Leader")));
    TestEqual(TEXT("graph.role"), Graph.Role, FString(TEXT("NPC.Role.Warchief")));
    TestEqual(TEXT("graph.faction"), Graph.Faction, FString(TEXT("Faction.Ashfang")));
    TestEqual(TEXT("graph.entryNodeId"), Graph.EntryNodeId, FString(TEXT("confrontation")));
    TestEqual(TEXT("graph.nodes count"), Graph.Nodes.Num(), 2);

    if (Graph.Nodes.Num() == 2) {
        const FMythicDialogueNodeSpec &N0 = Graph.Nodes[0];
        TestEqual(TEXT("node0.id"), N0.Id, FString(TEXT("confrontation")));
        TestEqual(TEXT("node0.speaker"), N0.Speaker, FString(TEXT("Faction.Ashfang.Leader")));
        TestEqual(TEXT("node0.line"), N0.Line, FString(TEXT("Strike, or speak.")));
        TestEqual(TEXT("node0.entry.requireAll count"), N0.EntryCondition.RequireAll.Num(), 1);
        if (N0.EntryCondition.RequireAll.Num() == 1) {
            TestEqual(TEXT("node0.entry.requireAll[0]"), N0.EntryCondition.RequireAll[0], FString(TEXT("Story.Ashfang.Reached")));
        }
        TestEqual(TEXT("node0.entry.blockAny count"), N0.EntryCondition.BlockAny.Num(), 2);
        TestEqual(TEXT("node0.choices count"), N0.Choices.Num(), 2);

        if (N0.Choices.Num() == 2) {
            const FMythicDialogueChoiceSpec &C0 = N0.Choices[0];
            TestEqual(TEXT("choice0.text"), C0.Text, FString(TEXT("Spare him.")));
            TestEqual(TEXT("choice0.condition.blockAny count"), C0.Condition.BlockAny.Num(), 2);
            TestEqual(TEXT("choice0.grantTags count"), C0.GrantTags.Num(), 1);
            if (C0.GrantTags.Num() == 1) {
                TestEqual(TEXT("choice0.grantTags[0]"), C0.GrantTags[0], FString(TEXT("Story.Ashfang.Spared")));
            }
            TestEqual(TEXT("choice0.rewards.xpPercentage"), C0.Rewards.XpPercentage, 0.5f);
            TestEqual(TEXT("choice0.rewards.xpProficiency"), C0.Rewards.XpProficiency,
                      FString(TEXT("/Game/Prof/Prof_Diplomacy.Prof_Diplomacy")));
            TestEqual(TEXT("choice0.rewards.itemId"), C0.Rewards.ItemId,
                      FString(TEXT("/Game/Items/Item_PeaceToken.Item_PeaceToken")));
            TestEqual(TEXT("choice0.rewards.itemQuantity"), C0.Rewards.ItemQuantity, 1);
            TestEqual(TEXT("choice0.questOfferId"), C0.QuestOfferId, FString(TEXT("ashfang.mercy")));
            TestEqual(TEXT("choice0.gotoNodeId"), C0.GotoNodeId, FString(TEXT("spared-followup")));
            TestFalse(TEXT("choice0.endsDialogue"), C0.bEndsDialogue);

            const FMythicDialogueChoiceSpec &C1 = N0.Choices[1];
            TestEqual(TEXT("choice1.condition.requireAny count"), C1.Condition.RequireAny.Num(), 2);
            if (C1.Condition.RequireAny.Num() == 2) {
                TestEqual(TEXT("choice1.condition.requireAny[0]"), C1.Condition.RequireAny[0],
                          FString(TEXT("Renown.Global.Honored")));
            }
            TestEqual(TEXT("choice1.questOfferId"), C1.QuestOfferId, FString(TEXT("ashfang.vengeance")));
            TestTrue(TEXT("choice1.endsDialogue"), C1.bEndsDialogue);
        }

        const FMythicDialogueNodeSpec &N1 = Graph.Nodes[1];
        TestEqual(TEXT("node1.id"), N1.Id, FString(TEXT("spared-followup")));
        TestEqual(TEXT("node1.choices empty"), N1.Choices.Num(), 0);
    }

    {
        FMythicDialogueGraphSpec Out;
        TestFalse(TEXT("empty string fails"), FMythicDialogueJson::ParseGraphSpec(TEXT(""), Out));
        TestFalse(TEXT("truncated json fails"), FMythicDialogueJson::ParseGraphSpec(TEXT("{ \"id\": \"x"), Out));
        TestFalse(TEXT("array root fails"), FMythicDialogueJson::ParseGraphSpec(TEXT("[1,2,3]"), Out));
        TestFalse(TEXT("missing id fails"), FMythicDialogueJson::ParseGraphSpec(TEXT("{ \"nodes\": [] }"), Out));
        TestFalse(TEXT("empty id fails"), FMythicDialogueJson::ParseGraphSpec(TEXT("{ \"id\": \"\", \"nodes\": [] }"), Out));
        TestFalse(TEXT("node missing id fails"),
                  FMythicDialogueJson::ParseGraphSpec(
                      TEXT("{ \"id\": \"g\", \"nodes\": [ { \"line\": \"no id here\" } ] }"), Out));
        TestTrue(TEXT("failed parse resets out-param"), Out.Id.IsEmpty() && Out.Nodes.Num() == 0);
    }

    {
        FMythicDialogueGraphSpec Src;
        Src.Id = TEXT("roundtrip.graph");
        Src.NpcTag = TEXT("Objective.NPC.VillageElder");
        Src.Role = TEXT("NPC.Role.Elder");
        Src.Faction = TEXT("Faction.Lowlands");
        Src.EntryNodeId = TEXT("greet");

        FMythicDialogueNodeSpec Node;
        Node.Id = TEXT("greet");
        Node.Speaker = TEXT("Objective.NPC.VillageElder");
        Node.Line = TEXT("Welcome, wanderer.");
        Node.EntryCondition.RequireAll = {TEXT("Story.Prologue.Done")};
        Node.EntryCondition.RequireAny = {TEXT("Renown.Global.Friendly"), TEXT("Renown.Global.Honored")};
        Node.EntryCondition.BlockAny = {TEXT("Story.Elder.Insulted")};

        FMythicDialogueChoiceSpec Choice;
        Choice.Text = TEXT("I seek work.");
        Choice.Condition.BlockAny = {TEXT("Story.Elder.QuestTaken")};
        Choice.GrantTags = {TEXT("Story.Elder.QuestTaken"), TEXT("Story.Elder.Met")};
        Choice.Rewards.XpPercentage = 1.25f;
        Choice.Rewards.XpProficiency = TEXT("/Game/Prof/Prof_Diplomacy.Prof_Diplomacy");
        Choice.Rewards.ItemId = TEXT("/Game/Items/Item_Bread.Item_Bread");
        Choice.Rewards.ItemQuantity = 3;
        Choice.QuestOfferId = TEXT("elder.firstquest");
        Choice.GotoNodeId = TEXT("farewell");
        Choice.bEndsDialogue = false;
        Node.Choices.Add(Choice);

        FMythicDialogueChoiceSpec Bye;
        Bye.Text = TEXT("Farewell.");
        Bye.bEndsDialogue = true;
        Node.Choices.Add(Bye);
        Src.Nodes.Add(Node);

        FMythicDialogueNodeSpec Farewell;
        Farewell.Id = TEXT("farewell");
        Farewell.Speaker = TEXT("Objective.NPC.VillageElder");
        Farewell.Line = TEXT("May the roads be kind.");
        Src.Nodes.Add(Farewell);

        const FString Serialized = FMythicDialogueJson::SerializeGraphSpec(Src);
        FMythicDialogueGraphSpec Reparsed;
        TestTrue(TEXT("serialized graph reparses"), FMythicDialogueJson::ParseGraphSpec(Serialized, Reparsed));
        TestTrue(TEXT("round-trip Parse(Serialize(x)) == x"), Src == Reparsed);
        TestTrue(TEXT("serialized graph self-discriminates as dialogue"), FMythicDialogueJson::IsDialogueDocument(Serialized));
    }

    TestTrue(TEXT("kind:dialogue discriminates"),
             FMythicDialogueJson::IsDialogueDocument(TEXT("{ \"kind\": \"dialogue\", \"id\": \"g\" }")));
    TestTrue(TEXT("nodes array discriminates (no kind)"),
             FMythicDialogueJson::IsDialogueDocument(TEXT("{ \"id\": \"g\", \"nodes\": [] }")));
    TestFalse(TEXT("storyline-shaped doc does not discriminate"),
              FMythicDialogueJson::IsDialogueDocument(TEXT("{ \"id\": \"arc\", \"quests\": [] }")));
    TestFalse(TEXT("garbage does not discriminate"), FMythicDialogueJson::IsDialogueDocument(TEXT("not json at all")));

    return true;
}


IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueImportDiscriminationTest,
    "Mythic.Narrative.DialogueImportDiscrimination",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueImportDiscriminationTest::RunTest(const FString &Parameters) {
    const FString DialogueJson = TEXT(R"json(
    {
      "kind": "dialogue",
      "id": "disc.dialogue",
      "npcTag": "",
      "role": "",
      "faction": "",
      "entryNodeId": "n1",
      "nodes": [ { "id": "n1", "speaker": "", "line": "hi", "choices": [] } ]
    })json");

    const FString StorylineJson = TEXT(R"json(
    {
      "id": "disc.arc",
      "display": "Discrimination Arc",
      "quests": []
    })json");

    {
        FMythicStorylineSpec Swallowed;
        TestTrue(TEXT("storyline parser would swallow a dialogue doc (gate order matters)"),
                 FMythicNarrativeJson::ParseStorylineSpec(DialogueJson, Swallowed));
    }

    UGameInstance *OuterGI = NewObject<UGameInstance>(GetTransientPackage());
    UMythicNarrativeImportSubsystem *Import = NewObject<UMythicNarrativeImportSubsystem>(OuterGI);
    TestNotNull(TEXT("import subsystem constructs"), Import);
    if (!Import) {
        return false;
    }

    TArray<FMythicStorylineSpec> Collected;
    TestTrue(TEXT("dialogue doc imports"), Import->ImportDocument(DialogueJson, TEXT("<test:dialogue>"), Collected));
    TestEqual(TEXT("dialogue doc collects 0 storylines"), Collected.Num(), 0);
    UMythicDialogueGraph *Graph = Import->GetDialogueGraphById(TEXT("disc.dialogue"));
    TestNotNull(TEXT("dialogue doc built 1 graph"), Graph);
    if (Graph) {
        TestEqual(TEXT("graph node count"), Graph->Nodes.Num(), 1);
        TestEqual(TEXT("graph entry node id"), Graph->EntryNodeId, FString(TEXT("n1")));
    }
    TestNull(TEXT("dialogue doc built 0 storylines"), Import->GetStorylineById(TEXT("disc.dialogue")));

    TestTrue(TEXT("storyline doc imports"), Import->ImportDocument(StorylineJson, TEXT("<test:storyline>"), Collected));
    TestEqual(TEXT("storyline doc collects 1 storyline"), Collected.Num(), 1);
    Import->BuildFromSpecs(Collected);
    TestNotNull(TEXT("storyline doc built 1 storyline"), Import->GetStorylineById(TEXT("disc.arc")));
    TestNull(TEXT("storyline doc built 0 graphs"), Import->GetDialogueGraphById(TEXT("disc.arc")));

    return true;
}
