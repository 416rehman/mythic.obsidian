
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Narrative/MythicStoryCondition.h"
#include "Rewards/RewardBase.h"
#include "MythicDialogueGraphTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicDialogueChoice {
    GENERATED_BODY()

    // Player-facing choice line.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FText Text;

    // Tag gate on offering this choice. Author blockAny on the choice's own grant tag for one-shot choices
    // (the taken outcome hides itself on reopen — no re-award).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FMythicStoryCondition Condition;

    // Story tags stamped (via UMythicNarrativeStateComponent::ServerSetStoryTag) when this choice is picked.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FGameplayTagContainer GrantTags;

    // Rewards granted when this choice is picked (given through the owning PlayerController).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FRewardsToGive Rewards;

    // Authored quest id (import subsystem GetQuestById) started when this choice is picked. Empty = none.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FString QuestOfferId;

    // Authored storyline/arc id (import subsystem GetStorylineById) started when this choice is picked — the
    // production entry point into the Storyline > Quest > Task tier. Starts the arc via
    // UMythicQuestJournalComponent::ServerStartStoryline (re-gates on ArcGate; idempotent). Empty = none. Mirrors QuestOfferId.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FString StorylineOfferId;

    // Node id the conversation advances to after the consequences apply. Empty = none (dialogue ends unless a
    // designer also authored bEndsDialogue — a choice with neither goto nor end still terminates, by design).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FString GotoNodeId;

    // True = picking this choice closes the conversation (after consequences apply).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    bool bEndsDialogue = false;
};

USTRUCT(BlueprintType)
struct FMythicDialogueNode {
    GENERATED_BODY()

    // Unique-within-graph id — referenced by UMythicDialogueGraph::EntryNodeId and choice GotoNodeId.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FString Id;

    // Speaker identity tag (e.g. the NPC's QuestNpcTag) — lets the UI resolve a name/portrait.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag Speaker;

    // The spoken line.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FText Line;

    // Tag gate on ENTERING this node — respected by both entry resolution and goto advancement, so an authored
    // follow-up node (e.g. "spared" aftermath) only ever shows on the route that earned it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FMythicStoryCondition EntryCondition;

    // The player's options, in authored order (choice INDICES into this array are the server-validated pick keys).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    TArray<FMythicDialogueChoice> Choices;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicDialogueGraph : public UDataAsset {
    GENERATED_BODY()

public:
    // Author id (json "id") — the GetDialogueGraphById key.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FString GraphId;

    // Named-NPC identity key (matches AMythicNPCCharacter::QuestNpcTag). Valid = this graph binds to that NPC only.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag NpcTag;

    // Role resolution key (cognitive brain GetRole). Empty = not role-keyed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag Role;

    // Faction resolution key (faction database FactionTag). Empty = not faction-keyed.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FGameplayTag Faction;

    // Preferred entry node id. May be condition-rejected at runtime → entry falls back to the first node (in Nodes
    // order) whose EntryCondition passes. Empty = pure fallback scan.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    FString EntryNodeId;

    // Every node in the web, in authored order (the entry fallback scan honours this order).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Dialogue")
    TArray<FMythicDialogueNode> Nodes;

    const FMythicDialogueNode *FindNode(const FString &NodeId) const {
        if (NodeId.IsEmpty()) {
            return nullptr;
        }
        return Nodes.FindByPredicate([&NodeId](const FMythicDialogueNode &N) { return N.Id == NodeId; });
    }
};
