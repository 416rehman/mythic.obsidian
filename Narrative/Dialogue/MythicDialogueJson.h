
#pragma once

#include "CoreMinimal.h"
#include "Narrative/MythicNarrativeJson.h"

struct FMythicDialogueChoiceSpec {
    FString Text;
    FMythicStoryConditionSpec Condition;
    TArray<FString> GrantTags;
    FMythicRewardsSpec Rewards;
    FString QuestOfferId;
    FString StorylineOfferId;
    FString GotoNodeId;
    bool bEndsDialogue = false;

    bool operator==(const FMythicDialogueChoiceSpec &O) const {
        return Text == O.Text && Condition == O.Condition && GrantTags == O.GrantTags && Rewards == O.Rewards &&
               QuestOfferId == O.QuestOfferId && StorylineOfferId == O.StorylineOfferId && GotoNodeId == O.GotoNodeId &&
               bEndsDialogue == O.bEndsDialogue;
    }
    bool operator!=(const FMythicDialogueChoiceSpec &O) const { return !(*this == O); }
};

struct FMythicDialogueNodeSpec {
    FString Id;
    FString Speaker;
    FString Line;
    FMythicStoryConditionSpec EntryCondition;
    TArray<FMythicDialogueChoiceSpec> Choices;

    bool operator==(const FMythicDialogueNodeSpec &O) const {
        return Id == O.Id && Speaker == O.Speaker && Line == O.Line && EntryCondition == O.EntryCondition &&
               Choices == O.Choices;
    }
    bool operator!=(const FMythicDialogueNodeSpec &O) const { return !(*this == O); }
};

struct FMythicDialogueGraphSpec {
    FString Id;
    FString NpcTag;
    FString Role;
    FString Faction;
    FString EntryNodeId;
    TArray<FMythicDialogueNodeSpec> Nodes;

    bool operator==(const FMythicDialogueGraphSpec &O) const {
        return Id == O.Id && NpcTag == O.NpcTag && Role == O.Role && Faction == O.Faction &&
               EntryNodeId == O.EntryNodeId && Nodes == O.Nodes;
    }
    bool operator!=(const FMythicDialogueGraphSpec &O) const { return !(*this == O); }
};

class MYTHIC_API FMythicDialogueJson {
public:
    static bool IsDialogueDocument(const FString &Json);

    static bool ParseGraphSpec(const FString &Json, FMythicDialogueGraphSpec &Out);

    static FString SerializeGraphSpec(const FMythicDialogueGraphSpec &Spec);
};
