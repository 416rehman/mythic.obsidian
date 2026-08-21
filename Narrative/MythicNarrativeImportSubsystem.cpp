
#include "Narrative/MythicNarrativeImportSubsystem.h"

#include "Narrative/MythicNarrativeJson.h"
#include "Narrative/Dialogue/MythicDialogueJson.h"
#include "Narrative/MythicStoryCondition.h"
#include "Narrative/Dialogue/MythicDialogueGraphTypes.h"
#include "Narrative/MythicQuestDefinition.h"
#include "Narrative/MythicStorylineDefinition.h"
#include "Narrative/MythicQuestOutcome.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Rewards/RewardBase.h"
#include "Rewards/XPReward.h"
#include "Rewards/ItemReward.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Mythic/Mythic.h"

#include "GameplayTagContainer.h"
#include "HAL/IConsoleManager.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"


void UMythicNarrativeImportSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
    Super::Initialize(Collection);

    ReloadCommand = IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("Mythic.ReloadNarrative"),
        TEXT("Re-scan <ProjectContentDir>/Story for *.json storyline files and rebuild narrative definitions."),
        FConsoleCommandDelegate::CreateWeakLambda(this, [this]() { ReloadNarrative(); }),
        ECVF_Default);

    ReloadNarrative();
}

void UMythicNarrativeImportSubsystem::Deinitialize() {
    if (ReloadCommand) {
        IConsoleManager::Get().UnregisterConsoleObject(ReloadCommand);
        ReloadCommand = nullptr;
    }
    ClearBuiltDefinitions();
    Super::Deinitialize();
}

FString UMythicNarrativeImportSubsystem::GetStoryContentDir() {
    return FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Story"));
}


void UMythicNarrativeImportSubsystem::ReloadNarrative() {
    ClearBuiltDefinitions();

    const FString Dir = GetStoryContentDir();
    TArray<FString> Files;
    IFileManager::Get().FindFilesRecursive(Files, *Dir, TEXT("*.json"), true, false);

    if (Files.Num() == 0) {
        UE_LOG(Myth, Log, TEXT("[NarrativeImport] No *.json story files found under %s"), *Dir);
        return;
    }

    TArray<FMythicStorylineSpec> Specs;
    Specs.Reserve(Files.Num());
    for (const FString &File : Files) {
        FString Content;
        if (!FFileHelper::LoadFileToString(Content, *File)) {
            UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Failed to read %s"), *File);
            continue;
        }
        if (!ImportDocument(Content, File, Specs)) {
            UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Malformed / unparseable narrative json (skipped): %s"), *File);
        }
    }

    BuildFromSpecs(Specs);

    UE_LOG(Myth, Log, TEXT("[NarrativeImport] Built %d storyline(s), %d quest(s), %d task(s), %d dialogue graph(s) from %s"),
           StorylinesById.Num(), QuestsById.Num(), TasksById.Num(), GraphsById.Num(), *Dir);
}

bool UMythicNarrativeImportSubsystem::ImportDocument(const FString &JsonText, const FString &SourceForLog,
                                                     TArray<FMythicStorylineSpec> &OutStorylines) {
    if (FMythicDialogueJson::IsDialogueDocument(JsonText)) {
        FMythicDialogueGraphSpec GraphSpec;
        if (FMythicDialogueJson::ParseGraphSpec(JsonText, GraphSpec)) {
            return BuildDialogueGraph(GraphSpec) != nullptr;
        }
        UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Dialogue-shaped document failed to parse: %s"), *SourceForLog);
        return false;
    }

    FMythicStorylineSpec Spec;
    if (FMythicNarrativeJson::ParseStorylineSpec(JsonText, Spec)) {
        OutStorylines.Add(MoveTemp(Spec));
        return true;
    }
    return false;
}

void UMythicNarrativeImportSubsystem::ClearBuiltDefinitions() {
    TasksById.Empty();
    QuestsById.Empty();
    StorylinesById.Empty();
    GraphsById.Empty();
    GraphIdByNpcTag.Empty();
    OrderedGraphIds.Empty();
}


FGameplayTag UMythicNarrativeImportSubsystem::ToTag(const FString &TagString) {
    if (TagString.IsEmpty()) {
        return FGameplayTag();
    }
    const FGameplayTag Tag = FGameplayTag::RequestGameplayTag(FName(*TagString), false);
    if (!Tag.IsValid()) {
        UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Unknown gameplay tag '%s' -> empty tag."), *TagString);
    }
    return Tag;
}

FGameplayTagContainer UMythicNarrativeImportSubsystem::ToTagContainer(const TArray<FString> &TagStrings) {
    FGameplayTagContainer Container;
    for (const FString &S : TagStrings) {
        const FGameplayTag Tag = ToTag(S);
        if (Tag.IsValid()) {
            Container.AddTag(Tag);
        }
    }
    return Container;
}

FMythicStoryCondition UMythicNarrativeImportSubsystem::ToCondition(const FMythicStoryConditionSpec &Spec) {
    FMythicStoryCondition Condition;
    Condition.RequireAll = ToTagContainer(Spec.RequireAll);
    Condition.RequireAny = ToTagContainer(Spec.RequireAny);
    Condition.BlockAny = ToTagContainer(Spec.BlockAny);
    return Condition;
}

FRewardsToGive UMythicNarrativeImportSubsystem::BuildRewards(const FMythicRewardsSpec &Spec, UObject *Owner) {
    FRewardsToGive Rewards;
    UObject *RewardOuter = Owner ? Owner : static_cast<UObject *>(this);

    if (Spec.XpPercentage > 0.0f) {
        UXPReward *Xp = NewObject<UXPReward>(RewardOuter);
        Xp->Percentage = Spec.XpPercentage;
        if (!Spec.XpProficiency.IsEmpty()) {
            if (UProficiencyDefinition *Prof = LoadObject<UProficiencyDefinition>(nullptr, *Spec.XpProficiency)) {
                Xp->ProficiencyDef = Prof;
            }
            else {
                UE_LOG(Myth, Warning, TEXT("[NarrativeImport] XP proficiency asset '%s' not found; leaving null."),
                       *Spec.XpProficiency);
            }
        }
        Rewards.XPReward = Xp;
    }

    if (!Spec.ItemId.IsEmpty() && Spec.ItemQuantity > 0) {
        UItemReward *Item = NewObject<UItemReward>(RewardOuter);
        Item->Quantity = Spec.ItemQuantity;
        if (UItemDefinition *Def = LoadObject<UItemDefinition>(nullptr, *Spec.ItemId)) {
            Item->Item = Def;
        }
        else {
            UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Item asset '%s' not found; item reward has null item."),
                   *Spec.ItemId);
        }
        Rewards.ItemReward = Item;
    }

    return Rewards;
}


void UMythicNarrativeImportSubsystem::BuildFromSpecs(const TArray<FMythicStorylineSpec> &Storylines) {
    for (const FMythicStorylineSpec &S : Storylines) {
        UMythicStorylineDefinition *Arc = NewObject<UMythicStorylineDefinition>(this);
        Arc->ArcTag = ToTag(S.ArcTag);
        Arc->ArcGate = ToCondition(S.ArcGate);
        Arc->Rewards = BuildRewards(S.Rewards, Arc);
        Arc->GrantStoryTagsOnComplete = ToTagContainer(S.GrantStoryTags);
        Arc->JournalTitle = FText::FromString(S.Display);
        if (StorylinesById.Contains(S.Id)) {
            UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Duplicate storyline id '%s' — overwriting."), *S.Id);
        }
        StorylinesById.Add(S.Id, Arc);

        for (const FMythicQuestSpec &Q : S.Quests) {
            UMythicQuestDefinition *Quest = NewObject<UMythicQuestDefinition>(this);
            Quest->UnlockConditions = ToCondition(Q.UnlockCondition);
            Quest->ExclusiveLockTags = ToTagContainer(Q.ExclusiveLockTags);
            Quest->bIsOptional = Q.bOptional;
            Quest->Rewards = BuildRewards(Q.Rewards, Quest);
            Quest->GrantStoryTagsOnComplete = ToTagContainer(Q.GrantStoryTags);
            Quest->JournalTitle = FText::FromString(Q.Display);
            for (const FMythicOutcomeSpec &Oc : Q.Outcomes) {
                FMythicQuestOutcome Outcome;
                Outcome.OutcomeTag = ToTag(Oc.Outcome);
                Outcome.When = ToCondition(Oc.When);
                Outcome.Rewards = BuildRewards(Oc.Rewards, Quest);
                Outcome.GrantStoryTags = ToTagContainer(Oc.GrantStoryTags);
                Quest->Outcomes.Add(MoveTemp(Outcome));
            }
            if (QuestsById.Contains(Q.Id)) {
                UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Duplicate quest id '%s' — overwriting."), *Q.Id);
            }
            QuestsById.Add(Q.Id, Quest);

            for (const FMythicTaskSpec &T : Q.Tasks) {
                UObjectiveDefinition *Task = NewObject<UObjectiveDefinition>(this);
                if (const FGameplayTag Trig = ToTag(T.TriggerTag); Trig.IsValid()) {
                    Task->TriggerEventTag = Trig;
                }
                Task->RequiredPayloadTag = ToTag(T.PayloadTag);
                Task->RequiredCount = FMath::Max(1, T.Count);
                Task->bOptional = T.bOptional;
                Task->Precondition = ToCondition(T.Precondition);
                Task->GrantStoryTagsOnComplete = ToTagContainer(T.GrantStoryTags);
                Task->DisplayText = FText::FromString(T.Display);
                for (const FMythicBranchSpec &B : T.Branches) {
                    FMythicObjectiveBranch Branch;
                    Branch.OutcomeTag = ToTag(B.Outcome);
                    Branch.GrantStoryTags = ToTagContainer(B.GrantFlags);
                    Task->OutcomeBranches.Add(MoveTemp(Branch));
                }
                if (TasksById.Contains(T.Id)) {
                    UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Duplicate task id '%s' — overwriting."), *T.Id);
                }
                TasksById.Add(T.Id, Task);
            }
        }
    }

    auto ResolveTasks = [this](const TArray<FString> &Ids) {
        TArray<TObjectPtr<UObjectiveDefinition>> Result;
        Result.Reserve(Ids.Num());
        for (const FString &Id : Ids) {
            if (const TObjectPtr<UObjectiveDefinition> *Found = TasksById.Find(Id)) {
                Result.Add(*Found);
            }
            else {
                UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Unknown task id '%s' referenced; skipping."), *Id);
            }
        }
        return Result;
    };

    for (const FMythicStorylineSpec &S : Storylines) {
        if (UMythicStorylineDefinition *Arc = StorylinesById.FindRef(S.Id)) {
            for (const FMythicQuestSpec &Q : S.Quests) {
                if (UMythicQuestDefinition *Quest = QuestsById.FindRef(Q.Id)) {
                    Arc->Quests.Add(Quest);
                }
                else {
                    UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Unknown quest id '%s' referenced by storyline '%s'."),
                           *Q.Id, *S.Id);
                }
            }
        }

        for (const FMythicQuestSpec &Q : S.Quests) {
            if (UMythicQuestDefinition *Quest = QuestsById.FindRef(Q.Id)) {
                for (const FMythicTaskSpec &T : Q.Tasks) {
                    if (UObjectiveDefinition *Task = TasksById.FindRef(T.Id)) {
                        Quest->Tasks.Add(Task);
                    }
                }
            }

            for (const FMythicTaskSpec &T : Q.Tasks) {
                UObjectiveDefinition *Task = TasksById.FindRef(T.Id);
                if (!Task) {
                    continue;
                }
                Task->NextObjectives = ResolveTasks(T.Next);
                const int32 NumBranches = FMath::Min(Task->OutcomeBranches.Num(), T.Branches.Num());
                for (int32 i = 0; i < NumBranches; ++i) {
                    Task->OutcomeBranches[i].NextObjectives = ResolveTasks(T.Branches[i].Next);
                    Task->OutcomeBranches[i].CancelSiblings = ResolveTasks(T.Branches[i].Cancel);
                }
            }
        }
    }
}


UObjectiveDefinition *UMythicNarrativeImportSubsystem::GetTaskById(const FString &Id) const {
    const TObjectPtr<UObjectiveDefinition> *Found = TasksById.Find(Id);
    return Found ? Found->Get() : nullptr;
}

UMythicQuestDefinition *UMythicNarrativeImportSubsystem::GetQuestById(const FString &Id) const {
    const TObjectPtr<UMythicQuestDefinition> *Found = QuestsById.Find(Id);
    return Found ? Found->Get() : nullptr;
}

UMythicStorylineDefinition *UMythicNarrativeImportSubsystem::GetStorylineById(const FString &Id) const {
    const TObjectPtr<UMythicStorylineDefinition> *Found = StorylinesById.Find(Id);
    return Found ? Found->Get() : nullptr;
}

TArray<UMythicStorylineDefinition *> UMythicNarrativeImportSubsystem::GetAllStorylines() const {
    TArray<UMythicStorylineDefinition *> Result;
    Result.Reserve(StorylinesById.Num());
    for (const TPair<FString, TObjectPtr<UMythicStorylineDefinition>> &Pair : StorylinesById) {
        if (Pair.Value) {
            Result.Add(Pair.Value);
        }
    }
    return Result;
}


UMythicDialogueGraph *UMythicNarrativeImportSubsystem::BuildDialogueGraph(const FMythicDialogueGraphSpec &Spec) {
    if (Spec.Id.IsEmpty()) {
        return nullptr;
    }
    UMythicDialogueGraph *Graph = NewObject<UMythicDialogueGraph>(this);
    Graph->GraphId = Spec.Id;
    Graph->NpcTag = ToTag(Spec.NpcTag);
    Graph->Role = ToTag(Spec.Role);
    Graph->Faction = ToTag(Spec.Faction);
    Graph->EntryNodeId = Spec.EntryNodeId;

    Graph->Nodes.Reserve(Spec.Nodes.Num());
    for (const FMythicDialogueNodeSpec &N : Spec.Nodes) {
        FMythicDialogueNode Node;
        Node.Id = N.Id;
        Node.Speaker = ToTag(N.Speaker);
        Node.Line = FText::FromString(N.Line);
        Node.EntryCondition = ToCondition(N.EntryCondition);
        Node.Choices.Reserve(N.Choices.Num());
        for (const FMythicDialogueChoiceSpec &C : N.Choices) {
            FMythicDialogueChoice Choice;
            Choice.Text = FText::FromString(C.Text);
            Choice.Condition = ToCondition(C.Condition);
            Choice.GrantTags = ToTagContainer(C.GrantTags);
            Choice.Rewards = BuildRewards(C.Rewards, Graph);
            Choice.QuestOfferId = C.QuestOfferId;
            Choice.StorylineOfferId = C.StorylineOfferId;
            Choice.GotoNodeId = C.GotoNodeId;
            Choice.bEndsDialogue = C.bEndsDialogue;
            Node.Choices.Add(MoveTemp(Choice));
        }
        Graph->Nodes.Add(MoveTemp(Node));
    }

    if (GraphsById.Contains(Spec.Id)) {
        UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Duplicate dialogue graph id '%s' — overwriting."), *Spec.Id);
    }
    else {
        OrderedGraphIds.Add(Spec.Id);
    }
    GraphsById.Add(Spec.Id, Graph);
    if (Graph->NpcTag.IsValid()) {
        if (const FString *Prev = GraphIdByNpcTag.Find(Graph->NpcTag); Prev && *Prev != Spec.Id) {
            UE_LOG(Myth, Warning, TEXT("[NarrativeImport] Dialogue npcTag %s claimed by both '%s' and '%s' — '%s' wins."),
                   *Graph->NpcTag.ToString(), **Prev, *Spec.Id, *Spec.Id);
        }
        GraphIdByNpcTag.Add(Graph->NpcTag, Spec.Id);
    }
    return Graph;
}

UMythicDialogueGraph *UMythicNarrativeImportSubsystem::GetDialogueGraphById(const FString &Id) const {
    const TObjectPtr<UMythicDialogueGraph> *Found = GraphsById.Find(Id);
    return Found ? Found->Get() : nullptr;
}

UMythicDialogueGraph *UMythicNarrativeImportSubsystem::ResolveGraphForNpc(FGameplayTag NpcTag, FGameplayTag Role,
                                                                          FGameplayTag Faction) const {
    if (NpcTag.IsValid()) {
        if (const FString *Id = GraphIdByNpcTag.Find(NpcTag)) {
            if (UMythicDialogueGraph *Graph = GetDialogueGraphById(*Id)) {
                return Graph;
            }
        }
    }
    auto FindGeneric = [this](const TFunctionRef<bool(const UMythicDialogueGraph &)> Match) -> UMythicDialogueGraph * {
        for (const FString &Id : OrderedGraphIds) {
            UMythicDialogueGraph *Graph = GetDialogueGraphById(Id);
            if (Graph && !Graph->NpcTag.IsValid() && Match(*Graph)) {
                return Graph;
            }
        }
        return nullptr;
    };
    if (Role.IsValid() && Faction.IsValid()) {
        if (UMythicDialogueGraph *Graph = FindGeneric([&](const UMythicDialogueGraph &G) {
                return G.Role == Role && G.Faction == Faction;
            })) {
            return Graph;
        }
    }
    if (Role.IsValid()) {
        if (UMythicDialogueGraph *Graph = FindGeneric([&](const UMythicDialogueGraph &G) {
                return G.Role == Role && !G.Faction.IsValid();
            })) {
            return Graph;
        }
    }
    if (Faction.IsValid()) {
        if (UMythicDialogueGraph *Graph = FindGeneric([&](const UMythicDialogueGraph &G) {
                return G.Faction == Faction && !G.Role.IsValid();
            })) {
            return Graph;
        }
    }
    return nullptr;
}
