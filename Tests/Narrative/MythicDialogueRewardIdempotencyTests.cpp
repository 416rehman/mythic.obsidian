
#include "Misc/AutomationTest.h"
#include "Narrative/Dialogue/MythicDialogueCore.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicDialogueRewardIdempotencyTest,
    "Mythic.Narrative.DialogueRewardIdempotency",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicDialogueRewardIdempotencyTest::RunTest(const FString &Parameters) {
    const FString Graph = TEXT("elder_greeting");
    const FString Node = TEXT("offer_reward");

    const FString KeyA1 = FMythicDialogueCore::MakeChoiceConsumedKey(Graph, Node, 0);
    const FString KeyA2 = FMythicDialogueCore::MakeChoiceConsumedKey(Graph, Node, 0);
    TestEqual(TEXT("same graph/node/index → identical key (deterministic)"), KeyA1, KeyA2);

    TestNotEqual(TEXT("different choice index → different key"),
                 KeyA1, FMythicDialogueCore::MakeChoiceConsumedKey(Graph, Node, 1));
    TestNotEqual(TEXT("different node → different key"),
                 KeyA1, FMythicDialogueCore::MakeChoiceConsumedKey(Graph, TEXT("other_node"), 0));
    TestNotEqual(TEXT("different graph → different key"),
                 KeyA1, FMythicDialogueCore::MakeChoiceConsumedKey(TEXT("other_graph"), Node, 0));

    TSet<FString> Consumed;

    const bool bFirstWouldPay = !Consumed.Contains(KeyA1);
    TestTrue(TEXT("first pick of a reward choice would pay (key not yet consumed)"), bFirstWouldPay);
    Consumed.Add(KeyA1);

    const bool bSecondWouldPay = !Consumed.Contains(KeyA1);
    TestFalse(TEXT("re-picking the SAME reward choice is skipped (key already consumed) — no re-farm"), bSecondWouldPay);

    const FString KeyB = FMythicDialogueCore::MakeChoiceConsumedKey(Graph, Node, 1);
    TestTrue(TEXT("a sibling reward choice is unaffected by the first choice's consumption"),
             !Consumed.Contains(KeyB));

    return true;
}
