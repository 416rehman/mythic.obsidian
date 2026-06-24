// Data-driven objective/quest definition — the first slice of a quest system.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "GAS/MythicTags_GAS.h"
#include "ObjectiveDefinition.generated.h"

/**
 * A single, designer-authored objective: "do {TriggerEventTag} [x{RequiredCount}, of {RequiredPayloadTag}], then
 * receive {Rewards}". Tracked per-player by UObjectiveTracker. The trigger is a GAS gameplay-event tag from the
 * GAS.Event.* family emitted server-side: combat (Kill / Death / Dmg.* / Heal.*) AND item acquisition
 * (GAS.Event.Item.Acquired — fires on every genuine pickup/grant). With RequiredPayloadTag + bCountByEventMagnitude
 * this expresses non-combat "collect N <type>" objectives (e.g. gather 20 wood) on the same atomic unit. Multi-step
 * chains, prerequisites, and branching are still intentionally NOT modeled here (separate follow-ups).
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

    // When set, the triggering event's payload (FGameplayEventData::TargetTags) must contain this tag (hierarchical
    // HasTag) for the event to advance this objective. Empty = no filter (counts every TriggerEventTag occurrence,
    // the original behaviour). Lets one trigger family serve specific objectives — e.g. TriggerEventTag =
    // GAS.Event.Item.Acquired + RequiredPayloadTag = Itemization.Type.Resource.Wood = "collect N wood".
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FGameplayTag RequiredPayloadTag;

    // false (default): each matching event advances by 1 (kills, the original behaviour — kill events carry the damage
    // as magnitude, so magnitude counting would be wrong for them). true: advance by the event's EventMagnitude rounded
    // (>=1), for quantity-bearing events like item acquisition (EventMagnitude = stacks acquired).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    bool bCountByEventMagnitude = false;

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

    /** The player-facing callout line for the objective's current state: the completion line once complete (if one was
     *  authored), otherwise the progress line. CompletedText is optional — falls back to DisplayText when empty, so an
     *  objective with no completion line keeps the prior behaviour. Pure (reads only this asset) → unit-testable. */
    FText GetCalloutText(bool bCompleted) const {
        return (bCompleted && !CompletedText.IsEmpty()) ? CompletedText : DisplayText;
    }
};
