
#pragma once

#include "CoreMinimal.h"
#include "Narrative/MythicNarrativeJson.h"
#include "Narrative/Dialogue/MythicDialogueJson.h"

struct FMythicNarrativeLintResult {
    TArray<FString> Errors;
    TArray<FString> Warnings;

    bool IsClean() const { return Errors.Num() == 0; }

    bool HasError(const FString &Substring) const {
        return Errors.ContainsByPredicate([&Substring](const FString &E) { return E.Contains(Substring); });
    }
    bool HasWarning(const FString &Substring) const {
        return Warnings.ContainsByPredicate([&Substring](const FString &W) { return W.Contains(Substring); });
    }
};

class FMythicNarrativeLint {
public:
    static TArray<FString> DefaultExternalTagPrefixes() {
        return {TEXT("Renown."), TEXT("Faction."), TEXT("Standing."), TEXT("GAS."), TEXT("Proficiency.")};
    }

    static FMythicNarrativeLintResult Validate(const TArray<FMythicDialogueGraphSpec> &Graphs,
                                               const TArray<FMythicQuestSpec> &Quests,
                                               const TArray<FMythicStorylineSpec> &Storylines) {
        return Validate(Graphs, Quests, Storylines, DefaultExternalTagPrefixes());
    }

    static FMythicNarrativeLintResult Validate(const TArray<FMythicDialogueGraphSpec> &Graphs,
                                               const TArray<FMythicQuestSpec> &Quests,
                                               const TArray<FMythicStorylineSpec> &Storylines,
                                               const TArray<FString> &ExternalTagPrefixes) {
        FMythicNarrativeLintResult R;

        TSet<FString> QuestIds;
        TSet<FString> StorylineIds;
        TSet<FString> TaskIds;

        for (const FMythicStorylineSpec &S : Storylines) {
            if (!S.Id.IsEmpty() && !AddUnique(StorylineIds, S.Id)) {
                R.Errors.Add(FString::Printf(TEXT("[E1 dup-storyline] duplicate storyline id '%s'."), *S.Id));
            }
        }
        auto RegisterQuest = [&](const FMythicQuestSpec &Q, const FString &OwnerDesc) {
            if (Q.Id.IsEmpty()) {
                return;
            }
            if (!AddUnique(QuestIds, Q.Id)) {
                R.Errors.Add(FString::Printf(TEXT("[E1 dup-quest] duplicate quest id '%s' (%s)."), *Q.Id, *OwnerDesc));
            }
            for (const FMythicTaskSpec &T : Q.Tasks) {
                if (!T.Id.IsEmpty() && !AddUnique(TaskIds, T.Id)) {
                    R.Errors.Add(FString::Printf(TEXT("[E1 dup-task] duplicate task id '%s' in quest '%s' (%s)."),
                                                 *T.Id, *Q.Id, *OwnerDesc));
                }
            }
        };
        for (const FMythicQuestSpec &Q : Quests) {
            RegisterQuest(Q, TEXT("standalone"));
        }
        for (const FMythicStorylineSpec &S : Storylines) {
            for (const FMythicQuestSpec &Q : S.Quests) {
                RegisterQuest(Q, FString::Printf(TEXT("in storyline '%s'"), *S.Id));
            }
        }

        TSet<FString> Granted;
        TArray<TPair<FString, FString>> GrantedList;
        TSet<FString> ConsumedAny;
        TArray<TPair<FString, FString>> RequireAllTags;
        TArray<TPair<TArray<FString>, FString>> RequireAnyClauses;

        auto AddGrants = [&](const TArray<FString> &Tags, const FString &Ctx) {
            for (const FString &T : Tags) {
                if (!T.IsEmpty()) {
                    Granted.Add(T);
                    GrantedList.Add({T, Ctx});
                }
            }
        };
        auto AddCondition = [&](const FMythicStoryConditionSpec &C, const FString &Ctx) {
            for (const FString &T : C.RequireAll) {
                if (!T.IsEmpty()) {
                    ConsumedAny.Add(T);
                    RequireAllTags.Add({T, Ctx});
                }
            }
            if (C.RequireAny.Num() > 0) {
                TArray<FString> Clause;
                for (const FString &T : C.RequireAny) {
                    if (!T.IsEmpty()) {
                        ConsumedAny.Add(T);
                        Clause.Add(T);
                    }
                }
                if (Clause.Num() > 0) {
                    RequireAnyClauses.Add({Clause, Ctx});
                }
            }
            for (const FString &T : C.BlockAny) {
                if (!T.IsEmpty()) {
                    ConsumedAny.Add(T);
                }
            }
        };

        TSet<FString> GraphIds;
        for (const FMythicDialogueGraphSpec &G : Graphs) {
            if (!G.Id.IsEmpty() && !AddUnique(GraphIds, G.Id)) {
                R.Errors.Add(FString::Printf(TEXT("[E1 dup-graph] duplicate dialogue graph id '%s'."), *G.Id));
            }

            TSet<FString> NodeIds;
            for (const FMythicDialogueNodeSpec &N : G.Nodes) {
                if (N.Id.IsEmpty()) {
                    R.Errors.Add(FString::Printf(TEXT("[E1 node-id] graph '%s' has a node with an empty id."), *G.Id));
                    continue;
                }
                if (!AddUnique(NodeIds, N.Id)) {
                    R.Errors.Add(FString::Printf(TEXT("[E1 dup-node] graph '%s' has duplicate node id '%s'."),
                                                 *G.Id, *N.Id));
                }
            }

            if (!G.EntryNodeId.IsEmpty() && !NodeIds.Contains(G.EntryNodeId)) {
                R.Errors.Add(FString::Printf(
                    TEXT("[E3 entry] graph '%s' entryNodeId '%s' resolves to no node in the graph."),
                    *G.Id, *G.EntryNodeId));
            }

            for (const FMythicDialogueNodeSpec &N : G.Nodes) {
                AddCondition(N.EntryCondition, FString::Printf(TEXT("graph '%s' node '%s' entryCondition"), *G.Id, *N.Id));
                for (int32 Ci = 0; Ci < N.Choices.Num(); ++Ci) {
                    const FMythicDialogueChoiceSpec &C = N.Choices[Ci];
                    const FString Where = FString::Printf(TEXT("graph '%s' node '%s' choice #%d"), *G.Id, *N.Id, Ci);

                    AddCondition(C.Condition, Where + TEXT(" condition"));
                    AddGrants(C.GrantTags, Where + TEXT(" grantTags"));

                    if (C.bEndsDialogue && !C.GotoNodeId.IsEmpty()) {
                        R.Warnings.Add(FString::Printf(
                            TEXT("[W9 ends+goto] %s sets endsDialogue AND gotoNodeId '%s' — at runtime endsDialogue "
                                 "wins and the goto is ignored."),
                            *Where, *C.GotoNodeId));
                    }

                    if (!C.GotoNodeId.IsEmpty() && !NodeIds.Contains(C.GotoNodeId) && !C.bEndsDialogue) {
                        R.Errors.Add(FString::Printf(
                            TEXT("[E2 goto] %s gotoNodeId '%s' resolves to no node in the graph."),
                            *Where, *C.GotoNodeId));
                    }

                    if (!C.QuestOfferId.IsEmpty() && !QuestIds.Contains(C.QuestOfferId)) {
                        R.Errors.Add(FString::Printf(
                            TEXT("[E4 quest-offer] %s questOfferId '%s' resolves to no defined quest."),
                            *Where, *C.QuestOfferId));
                    }
                    if (!C.StorylineOfferId.IsEmpty() && !StorylineIds.Contains(C.StorylineOfferId)) {
                        R.Errors.Add(FString::Printf(
                            TEXT("[E4 storyline-offer] %s storylineOfferId '%s' resolves to no defined storyline."),
                            *Where, *C.StorylineOfferId));
                    }
                }
            }

            CheckGraphReachability(G, NodeIds, R);
        }

        auto ProcessQuest = [&](const FMythicQuestSpec &Q, const FString &OwnerDesc) {
            AddCondition(Q.UnlockCondition, FString::Printf(TEXT("quest '%s' unlockCondition"), *Q.Id));
            for (const FString &T : Q.ExclusiveLockTags) {
                if (!T.IsEmpty()) {
                    ConsumedAny.Add(T);
                }
            }
            AddGrants(Q.GrantStoryTags, FString::Printf(TEXT("quest '%s' grantStoryTags"), *Q.Id));
            for (const FMythicOutcomeSpec &Oc : Q.Outcomes) {
                AddCondition(Oc.When, FString::Printf(TEXT("quest '%s' outcome '%s' when"), *Q.Id, *Oc.Outcome));
                AddGrants(Oc.GrantStoryTags,
                          FString::Printf(TEXT("quest '%s' outcome '%s' grantStoryTags"), *Q.Id, *Oc.Outcome));
            }
            for (const FMythicTaskSpec &T : Q.Tasks) {
                AddCondition(T.Precondition,
                             FString::Printf(TEXT("quest '%s' task '%s' precondition"), *Q.Id, *T.Id));
                AddGrants(T.GrantStoryTags,
                          FString::Printf(TEXT("quest '%s' task '%s' grantStoryTags"), *Q.Id, *T.Id));
                for (const FMythicBranchSpec &B : T.Branches) {
                    AddGrants(B.GrantFlags,
                              FString::Printf(TEXT("quest '%s' task '%s' branch '%s' grantFlags"), *Q.Id, *T.Id,
                                              *B.Outcome));
                }
            }
            CheckTaskReferences(Q, TaskIds, OwnerDesc, R);
        };
        for (const FMythicQuestSpec &Q : Quests) {
            ProcessQuest(Q, TEXT("standalone"));
        }
        for (const FMythicStorylineSpec &S : Storylines) {
            AddCondition(S.ArcGate, FString::Printf(TEXT("storyline '%s' arcGate"), *S.Id));
            AddGrants(S.GrantStoryTags, FString::Printf(TEXT("storyline '%s' grantStoryTags"), *S.Id));
            for (const FMythicQuestSpec &Q : S.Quests) {
                ProcessQuest(Q, FString::Printf(TEXT("storyline '%s'"), *S.Id));
            }
        }

        for (const TPair<FString, FString> &Req : RequireAllTags) {
            if (!IsGrantable(Req.Key, Granted, ExternalTagPrefixes)) {
                R.Errors.Add(FString::Printf(
                    TEXT("[E6 dead-branch] %s requires story tag '%s', which is granted by no choice/quest/task/"
                         "outcome/storyline and is not an external tag — this gate can never open."),
                    *Req.Value, *Req.Key));
            }
        }
        for (const TPair<TArray<FString>, FString> &Clause : RequireAnyClauses) {
            bool bAnyGrantable = false;
            for (const FString &T : Clause.Key) {
                if (IsGrantable(T, Granted, ExternalTagPrefixes)) {
                    bAnyGrantable = true;
                    break;
                }
            }
            if (!bAnyGrantable) {
                R.Errors.Add(FString::Printf(
                    TEXT("[E6 dead-branch] %s requireAny [%s] — NONE of these tags is granted anywhere (nor external), "
                         "so this gate can never open."),
                    *Clause.Value, *FString::Join(Clause.Key, TEXT(", "))));
            }
        }

        TSet<FString> WarnedDeadTags;
        for (const TPair<FString, FString> &G : GrantedList) {
            if (IsReferencedByAnyCondition(G.Key, ConsumedAny)) {
                continue;
            }
            if (WarnedDeadTags.Contains(G.Key)) {
                continue;
            }
            WarnedDeadTags.Add(G.Key);
            R.Warnings.Add(FString::Printf(
                TEXT("[W8 dead-tag] story tag '%s' is granted (%s) but read by no condition anywhere — harmless, but "
                     "likely an authoring leftover."),
                *G.Key, *G.Value));
        }

        return R;
    }

private:
    static bool AddUnique(TSet<FString> &Set, const FString &Key) {
        if (Set.Contains(Key)) {
            return false;
        }
        Set.Add(Key);
        return true;
    }

    static bool HasExternalPrefix(const FString &Tag, const TArray<FString> &Prefixes) {
        for (const FString &P : Prefixes) {
            if (!P.IsEmpty() && Tag.StartsWith(P, ESearchCase::CaseSensitive)) {
                return true;
            }
        }
        return false;
    }

    static bool IsGrantable(const FString &Req, const TSet<FString> &Granted, const TArray<FString> &Prefixes) {
        if (HasExternalPrefix(Req, Prefixes)) {
            return true;
        }
        if (Granted.Contains(Req)) {
            return true;
        }
        const FString ReqDot = Req + TEXT(".");
        for (const FString &G : Granted) {
            if (G.StartsWith(ReqDot, ESearchCase::CaseSensitive)) {
                return true;
            }
        }
        return false;
    }

    static bool IsReferencedByAnyCondition(const FString &G, const TSet<FString> &ConsumedAny) {
        if (ConsumedAny.Contains(G)) {
            return true;
        }
        const FString GDot = G + TEXT(".");
        for (const FString &C : ConsumedAny) {
            if (C.StartsWith(GDot, ESearchCase::CaseSensitive) || G.StartsWith(C + TEXT("."), ESearchCase::CaseSensitive)) {
                return true;
            }
        }
        return false;
    }

    static void CheckGraphReachability(const FMythicDialogueGraphSpec &G, const TSet<FString> &NodeIds,
                                       FMythicNarrativeLintResult &R) {
        if (G.Nodes.Num() == 0) {
            return;
        }
        FString StartId = (!G.EntryNodeId.IsEmpty() && NodeIds.Contains(G.EntryNodeId)) ? G.EntryNodeId : G.Nodes[0].Id;

        TMap<FString, const FMythicDialogueNodeSpec *> ById;
        for (const FMythicDialogueNodeSpec &N : G.Nodes) {
            ById.Add(N.Id, &N);
        }

        TSet<FString> Reached;
        TArray<FString> Frontier;
        if (ById.Contains(StartId)) {
            Reached.Add(StartId);
            Frontier.Add(StartId);
        }
        for (int32 Fi = 0; Fi < Frontier.Num(); ++Fi) {
            const FMythicDialogueNodeSpec *Node = ById.FindRef(Frontier[Fi]);
            if (!Node) {
                continue;
            }
            for (const FMythicDialogueChoiceSpec &C : Node->Choices) {
                if (!C.bEndsDialogue && !C.GotoNodeId.IsEmpty() && ById.Contains(C.GotoNodeId) &&
                    !Reached.Contains(C.GotoNodeId)) {
                    Reached.Add(C.GotoNodeId);
                    Frontier.Add(C.GotoNodeId);
                }
            }
        }

        for (const FMythicDialogueNodeSpec &N : G.Nodes) {
            if (!Reached.Contains(N.Id)) {
                R.Warnings.Add(FString::Printf(
                    TEXT("[W7 unreachable] graph '%s' node '%s' is not reachable from entry via any choice goto "
                         "(may be an intentional entry-conditional alt-entry)."),
                    *G.Id, *N.Id));
            }
        }

        bool bAnyEndReachable = false;
        for (const FString &Id : Reached) {
            const FMythicDialogueNodeSpec *Node = ById.FindRef(Id);
            if (Node && NodeEnds(*Node)) {
                bAnyEndReachable = true;
                break;
            }
        }
        if (!bAnyEndReachable && Reached.Num() > 0) {
            R.Warnings.Add(FString::Printf(
                TEXT("[W10 no-end] graph '%s' has no reachable end — every path from entry loops (no endsDialogue and "
                     "no terminating choice is reachable)."),
                *G.Id));
        }
    }

    static bool NodeEnds(const FMythicDialogueNodeSpec &N) {
        if (N.Choices.Num() == 0) {
            return true;
        }
        for (const FMythicDialogueChoiceSpec &C : N.Choices) {
            if (C.bEndsDialogue || C.GotoNodeId.IsEmpty()) {
                return true;
            }
        }
        return false;
    }

    static void CheckTaskReferences(const FMythicQuestSpec &Q, const TSet<FString> &TaskIds, const FString &OwnerDesc,
                                    FMythicNarrativeLintResult &R) {
        auto CheckList = [&](const TArray<FString> &Ids, const FString &RefKind, const FString &TaskId) {
            for (const FString &Id : Ids) {
                if (!Id.IsEmpty() && !TaskIds.Contains(Id)) {
                    R.Errors.Add(FString::Printf(
                        TEXT("[E5 task-ref] quest '%s' (%s) task '%s' %s references undefined task id '%s'."),
                        *Q.Id, *OwnerDesc, *TaskId, *RefKind, *Id));
                }
            }
        };
        for (const FMythicTaskSpec &T : Q.Tasks) {
            CheckList(T.Next, TEXT("next"), T.Id);
            for (const FMythicBranchSpec &B : T.Branches) {
                CheckList(B.Next, FString::Printf(TEXT("branch '%s' next"), *B.Outcome), T.Id);
                CheckList(B.Cancel, FString::Printf(TEXT("branch '%s' cancel"), *B.Outcome), T.Id);
            }
        }
    }
};
