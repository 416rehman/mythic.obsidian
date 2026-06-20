// Data-driven objective/quest definition — the first slice of a quest system.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "GAS/MythicTags_GAS.h"
#include "ObjectiveDefinition.generated.h"

/**
 * A single, designer-authored objective: "do {TriggerEventTag} x {RequiredCount}, then receive {Rewards}".
 * Tracked per-player by UObjectiveTracker. The trigger is a GAS gameplay-event tag — constrained to the
 * GAS.Event.* family because that is the only event family actually emitted server-side today (Kill / Death /
 * Dmg.* / Heal.*). Multi-step chains, prerequisites, branching, and non-combat verbs are intentionally NOT
 * modeled here (separate follow-ups); this is the atomic objective unit.
 */
UCLASS(BlueprintType)
class MYTHIC_API UObjectiveDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // The gameplay-event tag whose occurrences advance this objective (matched against FGameplayEventData::EventTag).
    // Defaults to GAS.Event.Kill — the proven-emitted kill event (UMythicLifeComponent::TriggerGameplayEvent_Kill).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective", meta = (Categories = "GAS.Event"))
    FGameplayTag TriggerEventTag = GAS_EVENT_KILL;

    // Occurrences of TriggerEventTag needed to complete this objective.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective", meta = (ClampMin = "1"))
    int32 RequiredCount = 1;

    // Rewards granted (server-side) on completion. Reuses the canonical one-of-each reward holder so the derived
    // contexts (XP level / item level) are built correctly — a bare TArray<URewardBase*> would zero those.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FRewardsToGive Rewards;

    // Player-facing objective line (e.g. "Slay 5 wolves").
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FText DisplayText;

    // Optional player-facing line shown on completion.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FText CompletedText;
};
