
#include "Misc/AutomationTest.h"
#include "GameplayTagContainer.h"
#include "Narrative/Dialogue/MythicDialogueJson.h"
#include "Narrative/MythicQuestDefinition.h"
#include "Narrative/MythicStorylineDefinition.h"
#include "Narrative/Dialogue/MythicDialogueCore.h"
#include "Narrative/Dialogue/MythicDialogueGraphTypes.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueStorylineOfferTest,
    "Mythic.Narrative.DialogueStorylineOffer",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueStorylineOfferTest::RunTest(const FString &Parameters) {
    {
        const FString GraphJson = TEXT(R"json(
        {
          "kind": "dialogue",
          "id": "arc.offer.dialogue",
          "entryNodeId": "hook",
          "nodes": [
            {
              "id": "hook",
              "speaker": "",
              "line": "A whole arc begins here.",
              "choices": [
                {
                  "text": "Begin the Reckoning.",
                  "questOfferId": "",
                  "storylineOfferId": "ashfang.arc",
                  "endsDialogue": true
                }
              ]
            }
          ]
        })json");

        FMythicDialogueGraphSpec Graph;
        TestTrue(TEXT("parse graph with storylineOfferId"), FMythicDialogueJson::ParseGraphSpec(GraphJson, Graph));
        TestEqual(TEXT("nodes count"), Graph.Nodes.Num(), 1);
        if (Graph.Nodes.Num() == 1 && Graph.Nodes[0].Choices.Num() == 1) {
            const FMythicDialogueChoiceSpec &C = Graph.Nodes[0].Choices[0];
            TestEqual(TEXT("parsed choice.storylineOfferId"), C.StorylineOfferId, FString(TEXT("ashfang.arc")));
            TestTrue(TEXT("parsed choice.questOfferId stays empty (independent field)"), C.QuestOfferId.IsEmpty());
        }

        FMythicDialogueGraphSpec NoOffer;
        TestTrue(TEXT("parse doc without storylineOfferId"),
                 FMythicDialogueJson::ParseGraphSpec(
                     TEXT("{ \"id\": \"g\", \"nodes\": [ { \"id\": \"n\", \"choices\": [ { \"text\": \"hi\" } ] } ] }"),
                     NoOffer));
        if (NoOffer.Nodes.Num() == 1 && NoOffer.Nodes[0].Choices.Num() == 1) {
            TestTrue(TEXT("absent storylineOfferId defaults empty"), NoOffer.Nodes[0].Choices[0].StorylineOfferId.IsEmpty());
        }

        FMythicDialogueGraphSpec Src;
        Src.Id = TEXT("roundtrip.arc");
        FMythicDialogueNodeSpec Node;
        Node.Id = TEXT("start");
        FMythicDialogueChoiceSpec Offer;
        Offer.Text = TEXT("Take up the arc.");
        Offer.StorylineOfferId = TEXT("ashfang.arc");
        Offer.bEndsDialogue = true;
        Node.Choices.Add(Offer);
        Src.Nodes.Add(Node);

        const FString Serialized = FMythicDialogueJson::SerializeGraphSpec(Src);
        FMythicDialogueGraphSpec Reparsed;
        TestTrue(TEXT("serialized arc-offer graph reparses"), FMythicDialogueJson::ParseGraphSpec(Serialized, Reparsed));
        TestTrue(TEXT("round-trip Parse(Serialize(x)) == x preserves storylineOfferId"), Src == Reparsed);
        if (Reparsed.Nodes.Num() == 1 && Reparsed.Nodes[0].Choices.Num() == 1) {
            TestEqual(TEXT("round-tripped storylineOfferId value"),
                      Reparsed.Nodes[0].Choices[0].StorylineOfferId, FString(TEXT("ashfang.arc")));
        }

        FMythicDialogueChoiceSpec A, B;
        A.StorylineOfferId = TEXT("ashfang.arc");
        B.StorylineOfferId = TEXT("other.arc");
        TestTrue(TEXT("specs differing only by storylineOfferId are not equal"), A != B);
        B.StorylineOfferId = TEXT("ashfang.arc");
        TestTrue(TEXT("specs with equal storylineOfferId are equal"), A == B);
    }

    {
        FMythicDialogueChoice Choice;
        Choice.QuestOfferId = TEXT("some.quest");
        Choice.StorylineOfferId = TEXT("ashfang.arc");
        Choice.GotoNodeId = TEXT("next");

        const FMythicDialogueConsequencePlan Plan = FMythicDialogueCore::PlanChoiceConsequences(Choice);
        TestEqual(TEXT("plan.StorylineOfferId snapshots the choice"), Plan.StorylineOfferId, FString(TEXT("ashfang.arc")));
        TestEqual(TEXT("plan.QuestOfferId still snapshots independently"), Plan.QuestOfferId, FString(TEXT("some.quest")));

        FMythicDialogueChoice Plain;
        TestTrue(TEXT("plan.StorylineOfferId empty when unset"),
                 FMythicDialogueCore::PlanChoiceConsequences(Plain).StorylineOfferId.IsEmpty());
    }

    {
        UMythicStorylineDefinition *ArcA = NewObject<UMythicStorylineDefinition>(GetTransientPackage());
        UMythicStorylineDefinition *ArcB = NewObject<UMythicStorylineDefinition>(GetTransientPackage());
        UMythicQuestDefinition *Q1 = NewObject<UMythicQuestDefinition>(GetTransientPackage());
        UMythicQuestDefinition *Q2 = NewObject<UMythicQuestDefinition>(GetTransientPackage());
        ArcA->Quests = {Q1, Q2};

        TArray<TObjectPtr<UMythicStorylineDefinition>> ActiveStorylines;
        TArray<TObjectPtr<UMythicQuestDefinition>> TrackedQuests;
        TSet<TObjectPtr<UMythicStorylineDefinition>> CompletedStorylines;
        TSet<FString> Owned;

        TMap<UMythicQuestDefinition *, FString> UnlockTag;
        UnlockTag.Add(Q1, FString());
        UnlockTag.Add(Q2, TEXT("Q1.Complete"));

        auto StartQuestInternal = [&](UMythicQuestDefinition *Q) -> bool {
            if (!Q || TrackedQuests.Contains(Q)) {
                return false;
            }
            const FString *Need = UnlockTag.Find(Q);
            if (Need && !Need->IsEmpty() && !Owned.Contains(*Need)) {
                return false;
            }
            TrackedQuests.Add(Q);
            return true;
        };
        auto StartStoryline = [&](UMythicStorylineDefinition *Arc) {
            ActiveStorylines.AddUnique(Arc);
            for (const TObjectPtr<UMythicQuestDefinition> &Q : Arc->Quests) {
                if (Q && !TrackedQuests.Contains(Q)) {
                    if (StartQuestInternal(Q.Get())) {
                        break;
                    }
                }
            }
        };

        StartStoryline(ArcA);
        TestEqual(TEXT("first start: one active storyline"), ActiveStorylines.Num(), 1);
        TestEqual(TEXT("first start: only the arc's first quest tracked"), TrackedQuests.Num(), 1);
        TestTrue(TEXT("first start: Q1 tracked"), TrackedQuests.Contains(Q1));
        TestFalse(TEXT("first start: Q2 not yet tracked (unlock-gated behind Q1)"), TrackedQuests.Contains(Q2));

        StartStoryline(ArcA);
        TestEqual(TEXT("repeat start: active storyline count stays 1 (AddUnique)"), ActiveStorylines.Num(), 1);
        TestEqual(TEXT("repeat start: no new quest started (Q1 tracked → skip; Q2 unlock-gated behind Q1)"),
                  TrackedQuests.Num(), 1);

        Owned.Add(TEXT("Q1.Complete"));
        TestTrue(TEXT("Q2 advances once Q1 completes (unlock now satisfied)"), StartQuestInternal(Q2));
        CompletedStorylines.Add(ArcA);

        StartStoryline(ArcA);
        TestEqual(TEXT("completed-arc restart: active count still 1"), ActiveStorylines.Num(), 1);
        TestEqual(TEXT("completed-arc restart: no quest re-added (all tracked)"), TrackedQuests.Num(), 2);
        TestTrue(TEXT("completed-arc stays recorded completed (arc reward not re-granted)"),
                 CompletedStorylines.Contains(ArcA));

        StartStoryline(ArcB);
        TestEqual(TEXT("distinct arc starts (guard is per-arc)"), ActiveStorylines.Num(), 2);
    }

    return true;
}
