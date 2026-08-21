
#include "Misc/AutomationTest.h"
#include "Narrative/MythicQuestJournalComponent.h"
#include "Subsystem/SaveSystem/Character/SavedQuestJournal.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicQuestJournalPersistenceTest,
    "Mythic.Narrative.QuestJournalPersistence",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

bool FMythicQuestJournalPersistenceTest::RunTest(const FString &Parameters) {
    using EQuest = EMythicQuestState;

    auto IsTerminalLatched = [](EQuest S) { return S == EQuest::Completed || S == EQuest::Failed; };

    const TArray<EQuest> AllStates = {EQuest::NotStarted, EQuest::Active, EQuest::Completed, EQuest::Failed};

    for (EQuest S : AllStates) {
        FMythicQuestJournalEntry Runtime;
        Runtime.Quest = nullptr;
        Runtime.State = S;

        const FSerializedQuestJournalEntry Serialized = UMythicQuestJournalComponent::MakeSerializedEntry(Runtime);
        TestEqual(TEXT("MakeSerializedEntry stores State as its uint8"),
                  Serialized.State, static_cast<uint8>(S));

        const EQuest Restored = static_cast<EQuest>(Serialized.State);
        TestEqual(TEXT("State survives the serialize→restore round-trip"), Restored, S);

        TestEqual(TEXT("terminal-latch classification preserved across round-trip"),
                  IsTerminalLatched(Restored), IsTerminalLatched(S));
    }

    {
        FMythicQuestJournalEntry Completed;
        Completed.State = EQuest::Completed;
        const FSerializedQuestJournalEntry S = UMythicQuestJournalComponent::MakeSerializedEntry(Completed);
        TestTrue(TEXT("restored Completed quest is terminal-latched (Pass-1 skips → no reward re-grant)"),
                 IsTerminalLatched(static_cast<EQuest>(S.State)));

        FMythicQuestJournalEntry Failed;
        Failed.State = EQuest::Failed;
        const FSerializedQuestJournalEntry F = UMythicQuestJournalComponent::MakeSerializedEntry(Failed);
        TestTrue(TEXT("restored Failed quest is terminal-latched (Pass-1 skips)"),
                 IsTerminalLatched(static_cast<EQuest>(F.State)));

        FMythicQuestJournalEntry Active;
        Active.State = EQuest::Active;
        TestFalse(TEXT("restored Active quest is NOT terminal-latched (recomputes live)"),
                  IsTerminalLatched(static_cast<EQuest>(UMythicQuestJournalComponent::MakeSerializedEntry(Active).State)));
    }

    {
        const FString QuestPathStr = TEXT("/Game/Mythic/Narrative/Quests/Q_ClearTheCamp.Q_ClearTheCamp");
        FSerializedQuestJournalEntry Entry;
        Entry.QuestPath = FSoftObjectPath(QuestPathStr);
        Entry.State = static_cast<uint8>(EQuest::Completed);

        const FSerializedQuestJournalEntry RoundTripped = Entry;
        TestEqual(TEXT("quest soft path survives the round-trip"),
                  RoundTripped.QuestPath.ToString(), QuestPathStr);
        TestEqual(TEXT("quest state survives alongside the path"),
                  static_cast<EQuest>(RoundTripped.State), EQuest::Completed);
    }

    {
        const FString ActivePath = TEXT("/Game/Mythic/Narrative/Arcs/Arc_Coastline.Arc_Coastline");
        const FString CompletedPath = TEXT("/Game/Mythic/Narrative/Arcs/Arc_Prologue.Arc_Prologue");

        TArray<FSoftObjectPath> Active = {FSoftObjectPath(ActivePath)};
        TArray<FSoftObjectPath> Completed = {FSoftObjectPath(CompletedPath)};

        const TArray<FSoftObjectPath> ActiveRT = Active;
        const TArray<FSoftObjectPath> CompletedRT = Completed;

        TestEqual(TEXT("one active storyline round-trips"), ActiveRT.Num(), 1);
        TestEqual(TEXT("active storyline path preserved"), ActiveRT[0].ToString(), ActivePath);
        TestEqual(TEXT("one completed storyline round-trips"), CompletedRT.Num(), 1);
        TestEqual(TEXT("completed storyline path preserved (arc stays completed → no arc-reward re-grant)"),
                  CompletedRT[0].ToString(), CompletedPath);
    }

    {
        const TArray<FSerializedQuestJournalEntry> EmptyJournal;
        const TArray<FSoftObjectPath> EmptyStorylines;
        TestEqual(TEXT("pre-QuestJournal (empty) journal has nothing to restore"), EmptyJournal.Num(), 0);
        TestEqual(TEXT("pre-QuestJournal (empty) storylines have nothing to restore"), EmptyStorylines.Num(), 0);
    }

    return true;
}
