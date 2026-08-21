
#include "Misc/AutomationTest.h"
#include "Narrative/MythicQuestJournalComponent.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicQuestStateTest,
    "Mythic.Narrative.QuestState",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicQuestStateTest::RunTest(const FString &Parameters) {
    using ETask = EMythicTaskState;
    using EQuest = EMythicQuestState;

    auto Derive = [](const TArray<ETask> &States, const TArray<bool> &Required, bool bLock) {
        return UMythicQuestJournalComponent::DeriveQuestState(States, Required, bLock);
    };

    const TArray<bool> AllRequired2 = {true, true};
    const TArray<bool> AllRequired3 = {true, true, true};

    TestEqual(TEXT("all NotStarted (required) → NotStarted"),
              Derive({ETask::NotStarted, ETask::NotStarted}, AllRequired2, false), EQuest::NotStarted);

    TestEqual(TEXT("one Active, one NotStarted → Active"),
              Derive({ETask::Active, ETask::NotStarted}, AllRequired2, false), EQuest::Active);
    TestEqual(TEXT("one Complete, one NotStarted → Active (not all required done)"),
              Derive({ETask::Complete, ETask::NotStarted}, AllRequired2, false), EQuest::Active);

    TestEqual(TEXT("all required Complete → Completed"),
              Derive({ETask::Complete, ETask::Complete}, AllRequired2, false), EQuest::Completed);
    TestEqual(TEXT("single required Complete → Completed"),
              Derive({ETask::Complete}, {true}, false), EQuest::Completed);

    TestEqual(TEXT("required Complete + optional NotStarted → Completed"),
              Derive({ETask::Complete, ETask::NotStarted}, {true, false}, false), EQuest::Completed);
    TestEqual(TEXT("required Complete + optional Active → Completed"),
              Derive({ETask::Complete, ETask::Active}, {true, false}, false), EQuest::Completed);
    TestEqual(TEXT("required Complete + optional Failed → Completed (optional never fails the quest)"),
              Derive({ETask::Complete, ETask::Failed}, {true, false}, false), EQuest::Completed);

    TestEqual(TEXT("a required task Failed → Failed"),
              Derive({ETask::Failed, ETask::Complete}, AllRequired2, false), EQuest::Failed);
    TestEqual(TEXT("required Failed dominates the other completes → Failed"),
              Derive({ETask::Complete, ETask::Failed, ETask::Complete}, AllRequired3, false), EQuest::Failed);

    TestEqual(TEXT("exclusive lock + partial progress → Failed"),
              Derive({ETask::Active, ETask::NotStarted}, AllRequired2, true), EQuest::Failed);
    TestEqual(TEXT("exclusive lock + nothing started → Failed"),
              Derive({ETask::NotStarted, ETask::NotStarted}, AllRequired2, true), EQuest::Failed);
    TestEqual(TEXT("exclusive lock overrides all-complete → Failed"),
              Derive({ETask::Complete, ETask::Complete}, AllRequired2, true), EQuest::Failed);

    TestEqual(TEXT("all-optional NotStarted → NotStarted"),
              Derive({ETask::NotStarted, ETask::NotStarted}, {false, false}, false), EQuest::NotStarted);
    TestEqual(TEXT("all-optional, one Active → Active"),
              Derive({ETask::Active, ETask::NotStarted}, {false, false}, false), EQuest::Active);

    TestEqual(TEXT("no tasks → NotStarted"), Derive({}, {}, false), EQuest::NotStarted);
    TestEqual(TEXT("no tasks + lock → Failed"), Derive({}, {}, true), EQuest::Failed);

    TestEqual(TEXT("short mask defaults required → Active (index 1 required+Active)"),
              Derive({ETask::Complete, ETask::Active}, {true}, false), EQuest::Active);

    return true;
}
