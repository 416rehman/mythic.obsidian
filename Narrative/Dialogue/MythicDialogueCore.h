
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Narrative/MythicStoryCondition.h"
#include "MythicDialogueGraphTypes.h"

struct FMythicDialogueConsequencePlan {
    FGameplayTagContainer GrantTags;
    bool bHasRewards = false;
    FString QuestOfferId;
    FString StorylineOfferId;
    FString GotoNodeId;
    bool bEnds = false;
};

struct FMythicDialogueCore {
    static const FMythicDialogueNode *ResolveEntryNode(const UMythicDialogueGraph &Graph,
                                                       const FGameplayTagContainer &Owned) {
        if (const FMythicDialogueNode *Preferred = Graph.FindNode(Graph.EntryNodeId)) {
            if (FMythicStoryCondition::Evaluate(Preferred->EntryCondition, Owned)) {
                return Preferred;
            }
        }
        for (const FMythicDialogueNode &Node : Graph.Nodes) {
            if (FMythicStoryCondition::Evaluate(Node.EntryCondition, Owned)) {
                return &Node;
            }
        }
        return nullptr;
    }

    static bool IsChoiceValid(const FMythicDialogueNode &Node, int32 ChoiceIndex, const FGameplayTagContainer &Owned) {
        if (!Node.Choices.IsValidIndex(ChoiceIndex)) {
            return false;
        }
        return FMythicStoryCondition::Evaluate(Node.Choices[ChoiceIndex].Condition, Owned);
    }

    static TArray<int32> FilterValidChoices(const FMythicDialogueNode &Node, const FGameplayTagContainer &Owned) {
        TArray<int32> Valid;
        Valid.Reserve(Node.Choices.Num());
        for (int32 i = 0; i < Node.Choices.Num(); ++i) {
            if (FMythicStoryCondition::Evaluate(Node.Choices[i].Condition, Owned)) {
                Valid.Add(i);
            }
        }
        return Valid;
    }

    static bool HasAnyReward(const FRewardsToGive &Rewards) {
        return Rewards.XPReward || Rewards.ItemReward || Rewards.LootReward || Rewards.AbilityReward ||
               Rewards.AttributeReward || Rewards.RenownReward;
    }

    static FString MakeChoiceConsumedKey(const FString &GraphId, const FString &NodeId, int32 ChoiceIndex) {
        return GraphId + TEXT("|") + NodeId + TEXT("|") + FString::FromInt(ChoiceIndex);
    }

    static FMythicDialogueConsequencePlan PlanChoiceConsequences(const FMythicDialogueChoice &Choice) {
        FMythicDialogueConsequencePlan Plan;
        Plan.GrantTags = Choice.GrantTags;
        Plan.bHasRewards = HasAnyReward(Choice.Rewards);
        Plan.QuestOfferId = Choice.QuestOfferId;
        Plan.StorylineOfferId = Choice.StorylineOfferId;
        Plan.GotoNodeId = Choice.GotoNodeId;
        Plan.bEnds = Choice.bEndsDialogue;
        return Plan;
    }
};
