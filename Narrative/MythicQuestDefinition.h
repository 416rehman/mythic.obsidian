
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "Narrative/MythicStoryCondition.h"
#include "Narrative/MythicQuestOutcome.h"
#include "MythicQuestDefinition.generated.h"

class UObjectiveDefinition;

UCLASS(BlueprintType)
class MYTHIC_API UMythicQuestDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Ordered TASK list. Reuses UObjectiveDefinition (the existing, tracked task primitive) verbatim — the journal
    // assigns these to the ObjectiveTracker on quest start and aggregates their per-task states into the quest state.
    // A task is REQUIRED for completion unless its own UObjectiveDefinition::bOptional is set (single source of truth).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    TArray<TObjectPtr<UObjectiveDefinition>> Tasks;

    // Tag-driven gate deciding whether this quest may START (evaluated against the player's owned story tags ∪ world
    // flags). Empty (default) = ungated. Layered like UObjectiveDefinition::Precondition but at the quest tier.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Narrative")
    FMythicStoryCondition UnlockConditions;

    // Owning ANY of these tags LOCKS/FAILS the quest — a mutually-exclusive route was taken (e.g. you sided with the
    // faction this quest opposes). Checked every recompute; trips the quest to Failed. Empty (default) = never locks.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Narrative")
    FGameplayTagContainer ExclusiveLockTags;

    // Side-quest flag: this quest is OPTIONAL to its storyline (arc completion does not wait on it). Default false = a
    // main-line quest that gates arc completion.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    bool bIsOptional = false;

    // Quest-level rewards granted (server-side) on the → Completed transition, in ADDITION to the matched Outcome's
    // rewards. Reuses the canonical FRewardsToGive (correct XP / item-level contexts).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest")
    FRewardsToGive Rewards;

    // Story tags stamped into the ledger when this quest COMPLETES (regardless of which outcome) — the fact of having
    // finished it. Outcome-specific consequences live on FMythicQuestOutcome::GrantStoryTags.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Narrative")
    FGameplayTagContainer GrantStoryTagsOnComplete;

    // Authored ENDINGS in priority order. On completion the journal picks the first outcome whose `When` passes
    // (FMythicQuestOutcome::ResolveQuestOutcome) and grants its rewards/tags. Empty = no outcome layer (just the
    // quest-level Rewards + GrantStoryTagsOnComplete apply).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|Narrative")
    TArray<FMythicQuestOutcome> Outcomes;

    // Player-facing journal header + body.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|UI")
    FText JournalTitle;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Quest|UI")
    FText JournalText;
};
