
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "GAS/MythicTags_GAS.h"
#include "Rewards/XPReward.h"
#include "Narrative/Dialogue/MythicDialogueCore.h"
#include "Narrative/Dialogue/MythicDialogueGraphTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueCoreTest,
    "Mythic.Narrative.DialogueCore",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueCoreTest::RunTest(const FString &Parameters) {
    const FGameplayTag TagReached = GAS_EVENT_REACHED_LOCATION;
    const FGameplayTag TagSpared = GAS_EVENT_TALKED_TO_NPC;
    const FGameplayTag TagDead = GAS_EVENT_KILL;
    const FGameplayTag TagRenown = GAS_EVENT_ITEM_ACQUIRED;

    auto MakeContainer = [](std::initializer_list<FGameplayTag> Tags) {
        FGameplayTagContainer C;
        for (const FGameplayTag &T : Tags) {
            C.AddTag(T);
        }
        return C;
    };
    const FGameplayTagContainer OwnedNone;
    const FGameplayTagContainer OwnedReached = MakeContainer({TagReached});
    const FGameplayTagContainer OwnedSpared = MakeContainer({TagReached, TagSpared});
    const FGameplayTagContainer OwnedRenown = MakeContainer({TagReached, TagRenown});

    UMythicDialogueGraph *Graph = NewObject<UMythicDialogueGraph>(GetTransientPackage());
    Graph->GraphId = TEXT("core.test");
    Graph->EntryNodeId = TEXT("confrontation");

    {
        FMythicDialogueNode Confront;
        Confront.Id = TEXT("confrontation");
        Confront.Line = FText::FromString(TEXT("Strike, or speak."));
        Confront.EntryCondition.RequireAll = MakeContainer({TagReached});
        Confront.EntryCondition.BlockAny = MakeContainer({TagSpared, TagDead});

        FMythicDialogueChoice Ask;
        Ask.Text = FText::FromString(TEXT("Why?"));
        Ask.GotoNodeId = TEXT("war-reasons");
        Confront.Choices.Add(Ask);

        FMythicDialogueChoice Spare;
        Spare.Text = FText::FromString(TEXT("Spare him."));
        Spare.Condition.BlockAny = MakeContainer({TagSpared, TagDead});
        Spare.GrantTags = MakeContainer({TagSpared});
        Spare.QuestOfferId = TEXT("ashfang.mercy");
        Spare.GotoNodeId = TEXT("spared-followup");
        Confront.Choices.Add(Spare);

        FMythicDialogueChoice Execute;
        Execute.Text = FText::FromString(TEXT("Execute him."));
        Execute.Condition.RequireAny = MakeContainer({TagRenown});
        Execute.Condition.BlockAny = MakeContainer({TagSpared, TagDead});
        Execute.GrantTags = MakeContainer({TagDead});
        Execute.bEndsDialogue = true;
        Confront.Choices.Add(Execute);
        Graph->Nodes.Add(Confront);

        FMythicDialogueNode Reasons;
        Reasons.Id = TEXT("war-reasons");
        Graph->Nodes.Add(Reasons);

        FMythicDialogueNode SparedNode;
        SparedNode.Id = TEXT("spared-followup");
        SparedNode.EntryCondition.RequireAll = MakeContainer({TagSpared});
        Graph->Nodes.Add(SparedNode);
    }

    {
        const FMythicDialogueNode *Entry = FMythicDialogueCore::ResolveEntryNode(*Graph, OwnedReached);
        TestNotNull(TEXT("entry resolves with Reached"), Entry);
        if (Entry) {
            TestEqual(TEXT("explicit entry wins when eligible"), Entry->Id, FString(TEXT("confrontation")));
        }

        Entry = FMythicDialogueCore::ResolveEntryNode(*Graph, OwnedSpared);
        TestNotNull(TEXT("entry falls back when explicit gated off"), Entry);
        if (Entry) {
            TestEqual(TEXT("fallback = first passing node in authored order"), Entry->Id, FString(TEXT("war-reasons")));
        }

        UMythicDialogueGraph *Gated = NewObject<UMythicDialogueGraph>(GetTransientPackage());
        Gated->EntryNodeId = TEXT("only");
        FMythicDialogueNode Only;
        Only.Id = TEXT("only");
        Only.EntryCondition.RequireAll = MakeContainer({TagSpared});
        Gated->Nodes.Add(Only);
        TestNull(TEXT("no eligible node -> null"), FMythicDialogueCore::ResolveEntryNode(*Gated, OwnedNone));
        TestNotNull(TEXT("same graph opens once gated tag owned"), FMythicDialogueCore::ResolveEntryNode(*Gated, OwnedSpared));

        UMythicDialogueGraph *NoEntry = NewObject<UMythicDialogueGraph>(GetTransientPackage());
        FMythicDialogueNode Free;
        Free.Id = TEXT("free");
        NoEntry->Nodes.Add(Free);
        const FMythicDialogueNode *Fallback = FMythicDialogueCore::ResolveEntryNode(*NoEntry, OwnedNone);
        TestNotNull(TEXT("empty EntryNodeId falls back to scan"), Fallback);
    }

    const FMythicDialogueNode &Confront = Graph->Nodes[0];

    {
        TArray<int32> Valid = FMythicDialogueCore::FilterValidChoices(Confront, OwnedReached);
        TestEqual(TEXT("fresh player sees 2 choices"), Valid.Num(), 2);
        TestTrue(TEXT("fresh: ask [0] offered"), Valid.Contains(0));
        TestTrue(TEXT("fresh: spare [1] offered"), Valid.Contains(1));
        TestFalse(TEXT("fresh: renown-gated execute [2] hidden (requireAny)"), Valid.Contains(2));

        Valid = FMythicDialogueCore::FilterValidChoices(Confront, OwnedRenown);
        TestEqual(TEXT("renowned player sees 3 choices"), Valid.Num(), 3);
        TestTrue(TEXT("indices are real + ordered"), Valid == TArray<int32>({0, 1, 2}));

        Valid = FMythicDialogueCore::FilterValidChoices(Confront, MakeContainer({TagReached, TagSpared, TagRenown}));
        TestEqual(TEXT("taken outcome leaves only the ungated ask"), Valid.Num(), 1);
        TestTrue(TEXT("taken: only ask [0] remains"), Valid.Contains(0));
    }

    {
        TestTrue(TEXT("valid index + passing condition -> ok"), FMythicDialogueCore::IsChoiceValid(Confront, 1, OwnedReached));
        TestFalse(TEXT("forged index into renown gate rejected"), FMythicDialogueCore::IsChoiceValid(Confront, 2, OwnedReached));
        TestTrue(TEXT("same index passes once renown owned"), FMythicDialogueCore::IsChoiceValid(Confront, 2, OwnedRenown));
        TestFalse(TEXT("stale one-shot rejected after its tag earned"), FMythicDialogueCore::IsChoiceValid(Confront, 1, OwnedSpared));
        TestFalse(TEXT("negative index rejected"), FMythicDialogueCore::IsChoiceValid(Confront, -1, OwnedRenown));
        TestFalse(TEXT("out-of-range index rejected"), FMythicDialogueCore::IsChoiceValid(Confront, 99, OwnedRenown));
    }

    {
        const FMythicDialogueConsequencePlan SparePlan = FMythicDialogueCore::PlanChoiceConsequences(Confront.Choices[1]);
        TestTrue(TEXT("plan.GrantTags carries the spare tag"), SparePlan.GrantTags.HasTagExact(TagSpared));
        TestEqual(TEXT("plan.GrantTags count"), SparePlan.GrantTags.Num(), 1);
        TestFalse(TEXT("plan.bHasRewards false with empty rewards"), SparePlan.bHasRewards);
        TestEqual(TEXT("plan.QuestOfferId"), SparePlan.QuestOfferId, FString(TEXT("ashfang.mercy")));
        TestEqual(TEXT("plan.GotoNodeId"), SparePlan.GotoNodeId, FString(TEXT("spared-followup")));
        TestFalse(TEXT("plan.bEnds false (goto continues)"), SparePlan.bEnds);

        const FMythicDialogueConsequencePlan ExecutePlan = FMythicDialogueCore::PlanChoiceConsequences(Confront.Choices[2]);
        TestTrue(TEXT("execute plan ends the dialogue"), ExecutePlan.bEnds);
        TestTrue(TEXT("execute plan has empty goto"), ExecutePlan.GotoNodeId.IsEmpty());

        FMythicDialogueChoice Rewarded;
        Rewarded.Rewards.XPReward = NewObject<UXPReward>(GetTransientPackage());
        TestTrue(TEXT("plan.bHasRewards true with an XP reward"),
                 FMythicDialogueCore::PlanChoiceConsequences(Rewarded).bHasRewards);
    }

    return true;
}
