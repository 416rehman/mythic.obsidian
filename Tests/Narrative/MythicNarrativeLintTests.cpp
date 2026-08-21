
#include "Misc/AutomationTest.h"
#include "Narrative/MythicNarrativeLint.h"
#include "Narrative/MythicNarrativeJson.h"
#include "Narrative/Dialogue/MythicDialogueJson.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FMythicNarrativeLintTest,
    "Mythic.Narrative.Lint",
    EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

namespace {
FMythicStoryConditionSpec Cond(const TArray<FString> &All, const TArray<FString> &Any, const TArray<FString> &Block) {
    FMythicStoryConditionSpec C;
    C.RequireAll = All;
    C.RequireAny = Any;
    C.BlockAny = Block;
    return C;
}

FMythicDialogueChoiceSpec Choice(const FString &Text, const FString &Goto, bool bEnds) {
    FMythicDialogueChoiceSpec C;
    C.Text = Text;
    C.GotoNodeId = Goto;
    C.bEndsDialogue = bEnds;
    return C;
}

FMythicDialogueNodeSpec Node(const FString &Id) {
    FMythicDialogueNodeSpec N;
    N.Id = Id;
    return N;
}

FMythicTaskSpec Task(const FString &Id) {
    FMythicTaskSpec T;
    T.Id = Id;
    return T;
}

FMythicStorylineSpec Storyline(const FString &Id, const FMythicQuestSpec &Q) {
    FMythicStorylineSpec S;
    S.Id = Id;
    S.Quests.Add(Q);
    return S;
}

bool LoadRealDeserterSlice(TArray<FMythicDialogueGraphSpec> &OutGraphs, TArray<FMythicStorylineSpec> &OutStorylines,
                           FAutomationTestBase &T) {
    const FString Dir = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Story"));
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *Dir, TEXT("sample_deserter_*.json"), true,
 false);
    if (Files.Num() == 0) {
        return false;
    }
    TArray<FMythicDialogueGraphSpec> Graphs;
    TArray<FMythicStorylineSpec> Storylines;
    for (const FString &File : Files) {
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *File)) {
            return false;
        }
        if (FMythicDialogueJson::IsDialogueDocument(Content)) {
            FMythicDialogueGraphSpec G;
            if (!FMythicDialogueJson::ParseGraphSpec(Content, G)) {
                T.AddError(FString::Printf(TEXT("Authored dialogue file failed to parse: %s"), *File));
                return false;
            }
            Graphs.Add(MoveTemp(G));
        }
        else {
            FMythicStorylineSpec S;
            if (!FMythicNarrativeJson::ParseStorylineSpec(Content, S)) {
                T.AddError(FString::Printf(TEXT("Authored storyline file failed to parse: %s"), *File));
                return false;
            }
            Storylines.Add(MoveTemp(S));
        }
    }
    if (Graphs.Num() != 2 || Storylines.Num() != 2) {
        return false;
    }
    OutGraphs = MoveTemp(Graphs);
    OutStorylines = MoveTemp(Storylines);
    return true;
}

void BuildFallbackDeserterSlice(TArray<FMythicDialogueGraphSpec> &Graphs, TArray<FMythicStorylineSpec> &Storylines) {
    {
        FMythicTaskSpec Track = Task(TEXT("deserter.track"));
        Track.GrantStoryTags = {TEXT("Story.Deserter.Found")};
        FMythicTaskSpec Evidence = Task(TEXT("deserter.evidence"));
        Evidence.bOptional = true;
        Evidence.GrantStoryTags = {TEXT("Story.Deserter.Evidence")};

        FMythicQuestSpec Confront;
        Confront.Id = TEXT("deserter.confront");
        Confront.Tasks = {Track, Evidence};
        FMythicOutcomeSpec Exposed;
        Exposed.Outcome = TEXT("Story.Outcome.Exposed");
        Exposed.When = Cond({TEXT("Story.Deserter.Evidence")}, {}, {});
        FMythicOutcomeSpec Merciful;
        Merciful.Outcome = TEXT("Story.Outcome.Merciful");
        Merciful.When = Cond({TEXT("Story.Deserter.Spared")}, {}, {});
        FMythicOutcomeSpec Lawful;
        Lawful.Outcome = TEXT("Story.Outcome.Lawful");
        Lawful.When = Cond({TEXT("Story.Deserter.Condemned")}, {}, {});
        Confront.Outcomes = {Exposed, Merciful, Lawful};

        FMythicStorylineSpec Hunt = Storyline(TEXT("deserter.hunt"), Confront);
        Hunt.GrantStoryTags = {TEXT("Story.Deserter.HuntClosed")};
        Storylines.Add(Hunt);
    }
    {
        FMythicTaskSpec AmendsDeliver = Task(TEXT("amends.deliver"));
        AmendsDeliver.Precondition = Cond({TEXT("Story.Deserter.Spared")}, {}, {});
        FMythicQuestSpec Amends;
        Amends.Id = TEXT("deserter.amends");
        Amends.UnlockCondition = Cond({TEXT("Story.Deserter.Spared")}, {}, {});
        Amends.ExclusiveLockTags = {TEXT("Story.Deserter.Condemned")};
        Amends.Tasks = {AmendsDeliver};

        FMythicTaskSpec Testify = Task(TEXT("tribunal.testify"));
        Testify.Precondition = Cond({TEXT("Story.Deserter.Condemned")}, {}, {});
        FMythicQuestSpec Tribunal;
        Tribunal.Id = TEXT("deserter.tribunal");
        Tribunal.UnlockCondition = Cond({TEXT("Story.Deserter.Condemned")}, {}, {});
        Tribunal.ExclusiveLockTags = {TEXT("Story.Deserter.Spared")};
        Tribunal.Tasks = {Testify};
        FMythicOutcomeSpec Execution;
        Execution.Outcome = TEXT("Story.Outcome.Execution");
        Execution.When = Cond({TEXT("Story.Deserter.Executed")}, {}, {});
        FMythicOutcomeSpec Sentence;
        Sentence.Outcome = TEXT("Story.Outcome.Sentence");
        Tribunal.Outcomes = {Execution, Sentence};

        FMythicStorylineSpec Reckoning;
        Reckoning.Id = TEXT("deserter.reckoning");
        Reckoning.ArcGate = Cond({}, {TEXT("Story.Deserter.Spared"), TEXT("Story.Deserter.Condemned")}, {});
        Reckoning.Quests = {Amends, Tribunal};
        Reckoning.GrantStoryTags = {TEXT("Story.Deserter.ReckoningClosed")};
        Storylines.Add(Reckoning);
    }
    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("deserter.sergeant.dialogue");
        G.EntryNodeId = TEXT("muster");

        FMythicDialogueNodeSpec Muster = Node(TEXT("muster"));
        Muster.EntryCondition = Cond({}, {}, {TEXT("Story.Deserter.Spared"), TEXT("Story.Deserter.Condemned")});
        Muster.Choices.Add(Choice(TEXT("Who fled?"), TEXT("briefing"), false));
        FMythicDialogueChoiceSpec Accept = Choice(TEXT("Accept"), TEXT("accepted"), false);
        Accept.Condition = Cond({}, {}, {TEXT("Story.Deserter.Contracted")});
        Accept.GrantTags = {TEXT("Story.Deserter.Contracted")};
        Accept.QuestOfferId = TEXT("deserter.confront");
        Muster.Choices.Add(Accept);
        Muster.Choices.Add(Choice(TEXT("Leave"), TEXT(""), true));

        FMythicDialogueNodeSpec Briefing = Node(TEXT("briefing"));
        Briefing.Choices.Add(Choice(TEXT("Back"), TEXT("muster"), false));

        FMythicDialogueNodeSpec Accepted = Node(TEXT("accepted"));
        Accepted.Choices.Add(Choice(TEXT("On my way"), TEXT(""), true));

        FMythicDialogueNodeSpec RepSpared = Node(TEXT("report-spared"));
        RepSpared.EntryCondition = Cond({TEXT("Story.Deserter.Spared")}, {}, {});
        RepSpared.Choices.Add(Choice(TEXT("Let it lie"), TEXT(""), true));
        FMythicDialogueChoiceSpec CauseA = Choice(TEXT("Take up the cause"), TEXT(""), true);
        CauseA.Condition = Cond({}, {}, {TEXT("Story.Deserter.ReckoningStarted")});
        CauseA.GrantTags = {TEXT("Story.Deserter.ReckoningStarted")};
        CauseA.StorylineOfferId = TEXT("deserter.reckoning");
        RepSpared.Choices.Add(CauseA);

        FMythicDialogueNodeSpec RepCondemned = Node(TEXT("report-condemned"));
        RepCondemned.EntryCondition = Cond({TEXT("Story.Deserter.Condemned")}, {}, {});
        RepCondemned.Choices.Add(Choice(TEXT("It's done"), TEXT(""), true));
        FMythicDialogueChoiceSpec CauseB = Choice(TEXT("Take up the cause"), TEXT(""), true);
        CauseB.Condition = Cond({}, {}, {TEXT("Story.Deserter.ReckoningStarted")});
        CauseB.GrantTags = {TEXT("Story.Deserter.ReckoningStarted")};
        CauseB.StorylineOfferId = TEXT("deserter.reckoning");
        RepCondemned.Choices.Add(CauseB);

        G.Nodes = {Muster, Briefing, Accepted, RepSpared, RepCondemned};
        Graphs.Add(G);
    }
    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("deserter.fugitive.dialogue");
        G.EntryNodeId = TEXT("plea");

        FMythicDialogueNodeSpec Plea = Node(TEXT("plea"));
        Plea.EntryCondition = Cond({TEXT("Story.Deserter.Found")}, {},
                                   {TEXT("Story.Deserter.Spared"), TEXT("Story.Deserter.Condemned")});
        FMythicDialogueChoiceSpec Spare = Choice(TEXT("Spare"), TEXT("spared-aftermath"), false);
        Spare.Condition = Cond({}, {}, {TEXT("Story.Deserter.Spared"), TEXT("Story.Deserter.Condemned")});
        Spare.GrantTags = {TEXT("Story.Deserter.Spared")};
        Plea.Choices.Add(Spare);
        FMythicDialogueChoiceSpec Condemn = Choice(TEXT("Condemn"), TEXT("condemned-aftermath"), false);
        Condemn.Condition = Cond({}, {}, {TEXT("Story.Deserter.Spared"), TEXT("Story.Deserter.Condemned")});
        Condemn.GrantTags = {TEXT("Story.Deserter.Condemned")};
        Plea.Choices.Add(Condemn);
        FMythicDialogueChoiceSpec Execute = Choice(TEXT("Execute"), TEXT("condemned-aftermath"), false);
        Execute.Condition = Cond({}, {TEXT("Renown.Frontier.Feared")},
                                 {TEXT("Story.Deserter.Spared"), TEXT("Story.Deserter.Condemned")});
        Execute.GrantTags = {TEXT("Story.Deserter.Condemned"), TEXT("Story.Deserter.Executed")};
        Plea.Choices.Add(Execute);

        FMythicDialogueNodeSpec SparedAfter = Node(TEXT("spared-aftermath"));
        SparedAfter.EntryCondition = Cond({TEXT("Story.Deserter.Spared")}, {}, {});
        SparedAfter.Choices.Add(Choice(TEXT("Go"), TEXT(""), true));

        FMythicDialogueNodeSpec CondemnedAfter = Node(TEXT("condemned-aftermath"));
        CondemnedAfter.EntryCondition = Cond({TEXT("Story.Deserter.Condemned")}, {}, {});
        CondemnedAfter.Choices.Add(Choice(TEXT("March"), TEXT(""), true));

        G.Nodes = {Plea, SparedAfter, CondemnedAfter};
        Graphs.Add(G);
    }
}
}

bool FMythicNarrativeLintTest::RunTest(const FString &Parameters) {
    using FLint = FMythicNarrativeLint;

    {
        const FString DialogueJson = TEXT(R"json(
        {
          "kind": "dialogue",
          "id": "rt.dialogue",
          "entryNodeId": "start",
          "nodes": [
            { "id": "start", "line": "Choose.",
              "choices": [
                { "text": "Offer a quest", "grantTags": ["Story.Deserter.Contracted"],
                  "condition": { "requireAll": [], "requireAny": [], "blockAny": ["Story.Deserter.Contracted"] },
                  "questOfferId": "deserter.confront", "gotoNodeId": "next" },
                { "text": "Offer an arc", "storylineOfferId": "deserter.reckoning", "endsDialogue": true }
              ] },
            { "id": "next", "line": "Done.", "choices": [ { "text": "Bye", "endsDialogue": true } ] }
          ]
        })json");
        FMythicDialogueGraphSpec G;
        TestTrue(TEXT("real parser: IsDialogueDocument routes the dialogue doc"),
                 FMythicDialogueJson::IsDialogueDocument(DialogueJson));
        TestTrue(TEXT("real parser: dialogue slice shape parses (schema-valid)"),
                 FMythicDialogueJson::ParseGraphSpec(DialogueJson, G));
        FMythicDialogueGraphSpec G2;
        TestTrue(TEXT("dialogue round-trip reparses"),
                 FMythicDialogueJson::ParseGraphSpec(FMythicDialogueJson::SerializeGraphSpec(G), G2));
        TestTrue(TEXT("dialogue Parse(Serialize(x)) == x"), G == G2);

        const FString StorylineJson = TEXT(R"json(
        {
          "id": "rt.storyline",
          "display": "Round Trip",
          "arcTag": "Story.Arc.Deserter",
          "quests": [
            { "id": "rt.quest", "display": "Q",
              "tasks": [
                { "id": "rt.req", "display": "Required", "optional": false, "grantStoryTags": ["Story.Deserter.Found"] },
                { "id": "rt.opt", "display": "Optional", "optional": true, "grantStoryTags": ["Story.Deserter.Evidence"] }
              ],
              "outcomes": [
                { "outcome": "Story.Outcome.Merciful", "when": { "requireAll": ["Story.Deserter.Spared"], "requireAny": [], "blockAny": [] } }
              ] }
          ]
        })json");
        FMythicStorylineSpec S;
        TestFalse(TEXT("real parser: storyline doc is NOT a dialogue document"),
                  FMythicDialogueJson::IsDialogueDocument(StorylineJson));
        TestTrue(TEXT("real parser: storyline slice shape parses (schema-valid)"),
                 FMythicNarrativeJson::ParseStorylineSpec(StorylineJson, S));
        if (S.Quests.Num() == 1 && S.Quests[0].Tasks.Num() == 2) {
            TestFalse(TEXT("required task parses optional=false"), S.Quests[0].Tasks[0].bOptional);
            TestTrue(TEXT("optional task parses optional=true"), S.Quests[0].Tasks[1].bOptional);
        }
        FMythicStorylineSpec S2;
        TestTrue(TEXT("storyline round-trip reparses"),
                 FMythicNarrativeJson::ParseStorylineSpec(FMythicNarrativeJson::SerializeStorylineSpec(S), S2));
        TestTrue(TEXT("storyline Parse(Serialize(x)) == x"), S == S2);
    }

    {
        TArray<FMythicDialogueGraphSpec> Graphs;
        TArray<FMythicStorylineSpec> Storylines;
        const bool bReal = LoadRealDeserterSlice(Graphs, Storylines, *this);
        if (!bReal) {
            AddInfo(TEXT("Live Content/Story deserter files not found/parsed headless — using the embedded fallback "
                         "clean slice. (A future editor/commandlet pass can point Validate at the live content dir.)"));
            Graphs.Reset();
            Storylines.Reset();
            BuildFallbackDeserterSlice(Graphs, Storylines);
        }
        else {
            AddInfo(TEXT("Linted the LIVE authored Content/Story/sample_deserter_*.json files."));
            FMythicDialogueGraphSpec RG;
            TestTrue(TEXT("live dialogue round-trips"),
                     FMythicDialogueJson::ParseGraphSpec(FMythicDialogueJson::SerializeGraphSpec(Graphs[0]), RG) &&
                         RG == Graphs[0]);
            FMythicStorylineSpec RS;
            TestTrue(TEXT("live storyline round-trips"),
                     FMythicNarrativeJson::ParseStorylineSpec(FMythicNarrativeJson::SerializeStorylineSpec(Storylines[0]),
                                                              RS) &&
                         RS == Storylines[0]);
        }

        const FMythicNarrativeLintResult R = FLint::Validate(Graphs, {}, Storylines);

        TestTrue(TEXT("CLEAN slice: lint is clean (zero errors)"), R.IsClean());
        TestEqual(TEXT("CLEAN slice: exactly zero errors"), R.Errors.Num(), 0);
        TestTrue(TEXT("CLEAN slice: W7 report-spared unreachable-via-goto"), R.HasWarning(TEXT("report-spared")));
        TestTrue(TEXT("CLEAN slice: W7 report-condemned unreachable-via-goto"), R.HasWarning(TEXT("report-condemned")));
        TestTrue(TEXT("CLEAN slice: W8 dead terminal tag HuntClosed"),
                 R.HasWarning(TEXT("Story.Deserter.HuntClosed")));
        TestTrue(TEXT("CLEAN slice: W8 dead terminal tag ReckoningClosed"),
                 R.HasWarning(TEXT("Story.Deserter.ReckoningClosed")));
        TestEqual(TEXT("CLEAN slice: exactly the 4 expected warnings (no unexpected ones)"), R.Warnings.Num(), 4);
    }


    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("dup.node.graph");
        G.EntryNodeId = TEXT("n");
        FMythicDialogueNodeSpec A = Node(TEXT("n"));
        A.Choices.Add(Choice(TEXT("end"), TEXT(""), true));
        FMythicDialogueNodeSpec B = Node(TEXT("n"));
        B.Choices.Add(Choice(TEXT("end"), TEXT(""), true));
        G.Nodes = {A, B};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestFalse(TEXT("E1 dup-node: not clean"), R.IsClean());
        TestTrue(TEXT("E1 dup-node: message names the duplicate node id"),
                 R.HasError(TEXT("duplicate node id 'n'")));
    }

    {
        FMythicQuestSpec DupQ;
        DupQ.Id = TEXT("dup.quest");
        FMythicStorylineSpec S1 = Storyline(TEXT("dup.arc"), DupQ);
        FMythicStorylineSpec S2 = Storyline(TEXT("dup.arc"), DupQ);
        const FMythicNarrativeLintResult R = FLint::Validate({}, {}, {S1, S2});
        TestTrue(TEXT("E1 dup-storyline: message names the duplicate storyline id"),
                 R.HasError(TEXT("duplicate storyline id 'dup.arc'")));
        TestTrue(TEXT("E1 dup-quest: message names the duplicate quest id"),
                 R.HasError(TEXT("duplicate quest id 'dup.quest'")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("dangling.goto");
        G.EntryNodeId = TEXT("start");
        FMythicDialogueNodeSpec Start = Node(TEXT("start"));
        Start.Choices.Add(Choice(TEXT("go nowhere"), TEXT("ghost-node"), false));
        G.Nodes = {Start};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("E2 goto: message names the dangling goto target"),
                 R.HasError(TEXT("gotoNodeId 'ghost-node'")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("dangling.entry");
        G.EntryNodeId = TEXT("ghost-entry");
        FMythicDialogueNodeSpec Real = Node(TEXT("real"));
        Real.Choices.Add(Choice(TEXT("end"), TEXT(""), true));
        G.Nodes = {Real};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("E3 entry: message names the dangling entryNodeId"),
                 R.HasError(TEXT("entryNodeId 'ghost-entry'")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("bad.offers");
        G.EntryNodeId = TEXT("n");
        FMythicDialogueNodeSpec N = Node(TEXT("n"));
        FMythicDialogueChoiceSpec OfferQuest = Choice(TEXT("offer quest"), TEXT(""), true);
        OfferQuest.QuestOfferId = TEXT("no.such.quest");
        FMythicDialogueChoiceSpec OfferArc = Choice(TEXT("offer arc"), TEXT(""), true);
        OfferArc.StorylineOfferId = TEXT("no.such.arc");
        N.Choices = {OfferQuest, OfferArc};
        G.Nodes = {N};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("E4 quest-offer: message names the undefined quest"),
                 R.HasError(TEXT("questOfferId 'no.such.quest'")));
        TestTrue(TEXT("E4 storyline-offer: message names the undefined storyline"),
                 R.HasError(TEXT("storylineOfferId 'no.such.arc'")));
    }

    {
        FMythicTaskSpec T = Task(TEXT("real.task"));
        T.Next = {TEXT("ghost.task")};
        FMythicQuestSpec Q;
        Q.Id = TEXT("q.badref");
        Q.Tasks = {T};
        const FMythicStorylineSpec S = Storyline(TEXT("s.badref"), Q);
        const FMythicNarrativeLintResult R = FLint::Validate({}, {}, {S});
        TestTrue(TEXT("E5 task-ref: message names the undefined task id"),
                 R.HasError(TEXT("undefined task id 'ghost.task'")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("dead.branch");
        G.EntryNodeId = TEXT("n");
        FMythicDialogueNodeSpec N = Node(TEXT("n"));
        FMythicDialogueChoiceSpec Gated = Choice(TEXT("gated"), TEXT(""), true);
        Gated.Condition = Cond({TEXT("Story.Never.Granted")}, {}, {});
        N.Choices = {Gated};
        G.Nodes = {N};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("E6 dead-branch (requireAll): message names the ungranted tag"),
                 R.HasError(TEXT("'Story.Never.Granted'")));
        TestTrue(TEXT("E6 dead-branch: message explains the gate can never open"),
                 R.HasError(TEXT("can never open")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("dead.any");
        G.EntryNodeId = TEXT("n");
        FMythicDialogueNodeSpec N = Node(TEXT("n"));
        FMythicDialogueChoiceSpec Gated = Choice(TEXT("gated"), TEXT(""), true);
        Gated.Condition = Cond({}, {TEXT("Story.Ghost.A"), TEXT("Story.Ghost.B")}, {});
        N.Choices = {Gated};
        G.Nodes = {N};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("E6 dead-branch (requireAny): flags the fully-ungranted clause"),
                 R.HasError(TEXT("requireAny")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("renown.gated");
        G.EntryNodeId = TEXT("n");
        FMythicDialogueNodeSpec N = Node(TEXT("n"));
        FMythicDialogueChoiceSpec Gated = Choice(TEXT("renown option"), TEXT(""), true);
        Gated.Condition = Cond({}, {TEXT("Renown.Frontier.Feared")}, {});
        N.Choices = {Gated};
        G.Nodes = {N};

        const FMythicNarrativeLintResult Def = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("allowlist: renown-gated choice is clean under the default allowlist"), Def.IsClean());

        const FMythicNarrativeLintResult Empty = FLint::Validate({G}, {}, {}, {});
        TestFalse(TEXT("allowlist: same choice trips the dead-branch error when the allowlist is emptied"),
                  Empty.IsClean());
        TestTrue(TEXT("allowlist: the tripped error names the renown tag"),
                 Empty.HasError(TEXT("Renown.Frontier.Feared")));
    }

    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("ends.and.goto");
        G.EntryNodeId = TEXT("a");
        FMythicDialogueNodeSpec A = Node(TEXT("a"));
        A.Choices.Add(Choice(TEXT("both"), TEXT("b"), true));
        FMythicDialogueNodeSpec B = Node(TEXT("b"));
        B.Choices.Add(Choice(TEXT("end"), TEXT(""), true));
        G.Nodes = {A, B};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("W9: ends+goto ambiguity is a warning, not an error"), R.IsClean());
        TestTrue(TEXT("W9: message flags endsDialogue winning over the goto"),
                 R.HasWarning(TEXT("endsDialogue wins")));
    }
    {
        FMythicDialogueGraphSpec G;
        G.Id = TEXT("all.loops");
        G.EntryNodeId = TEXT("a");
        FMythicDialogueNodeSpec A = Node(TEXT("a"));
        A.Choices.Add(Choice(TEXT("to b"), TEXT("b"), false));
        FMythicDialogueNodeSpec B = Node(TEXT("b"));
        B.Choices.Add(Choice(TEXT("to a"), TEXT("a"), false));
        G.Nodes = {A, B};
        const FMythicNarrativeLintResult R = FLint::Validate({G}, {}, {});
        TestTrue(TEXT("W10: no-reachable-end is a warning, not an error"), R.IsClean());
        TestTrue(TEXT("W10: message flags the no-end graph"), R.HasWarning(TEXT("no reachable end")));
    }

    return true;
}
