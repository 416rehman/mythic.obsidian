// Data-driven objective/quest definition — the first slice of a quest system.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "GAS/MythicTags_GAS.h"
#include "ObjectiveDefinition.generated.h"

class UItemDefinition;

/**
 * A single, designer-authored objective: "do {TriggerEventTag} [x{RequiredCount}, of {RequiredPayloadTag}], then
 * receive {Rewards}". Tracked per-player by UObjectiveTracker. The trigger is a GAS gameplay-event tag from the
 * GAS.Event.* family emitted server-side: combat (Kill / Death / Dmg.* / Heal.*) AND item acquisition
 * (GAS.Event.Item.Acquired — fires on every genuine pickup/grant). With RequiredPayloadTag + bCountByEventMagnitude
 * this expresses non-combat "collect N <type>" objectives (e.g. gather 20 wood) on the same atomic unit. Multi-step
 * quest chains are modeled by PrerequisiteObjectives (gate assignment) + NextObjectives (auto-assign the next step on
 * completion); branching (choose one of N next steps) is still a follow-up.
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

    // --- Turn-in / deliver objectives ("bring N <item> to NPC X") ---
    // When set together with DeliverToNpcTag, this is a DELIVERY objective: it is NOT advanced by GAS events (the tracker
    // skips it in HandleGameplayEvent, regardless of TriggerEventTag). Instead, talking to the NPC whose QuestNpcTag
    // matches DeliverToNpcTag consumes up to the remaining count of this exact item from the player's inventory and
    // advances by the amount consumed. A concrete item (not a type tag) lets the turn-in reuse the inventory's
    // GetItemCount + ServerRemoveItemByDefinition (no by-type slot walk), and makes the consumed amount unambiguous.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Delivery")
    TObjectPtr<UItemDefinition> DeliverItem = nullptr;

    // The receiving NPC's identity tag, matched (hierarchical) against AMythicNPCCharacter::QuestNpcTag. Set together
    // with DeliverItem to make this a delivery objective. Empty = not a delivery objective.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Delivery")
    FGameplayTag DeliverToNpcTag;

    // True iff this is a turn-in/deliver objective (advanced ONLY by handing DeliverItem to DeliverToNpcTag, never by a
    // GAS event). Pure → keeps the "delivery vs event-driven" rule in one place and unit-testable.
    bool IsDeliveryObjective() const { return DeliverItem != nullptr && DeliverToNpcTag.IsValid(); }

    // Rewards granted (server-side) on completion. Reuses the canonical one-of-each reward holder so the derived
    // contexts (XP level / item level) are built correctly — a bare TArray<URewardBase*> would zero those.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FRewardsToGive Rewards;

    // Objectives that must ALL be COMPLETED before this one can be assigned — the multi-step quest chain. Empty
    // (default) = no prerequisite (assignable immediately, the original behaviour). A quest-giver offering a later step
    // won't enroll the player until the earlier steps are done. Direct prerequisites only (no recursion) — the designer
    // authors the chain; a dependency cycle simply leaves all looped steps unassignable (harmless data error).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    TArray<TObjectPtr<UObjectiveDefinition>> PrerequisiteObjectives;

    // Objectives auto-assigned to the player when THIS one completes — the next step(s) of the quest chain, so it
    // advances without re-talking the giver. Each is still gated by its OWN PrerequisiteObjectives at assign time (a
    // converging step waits until all its prerequisites are complete), and an already-tracked/completed step is never
    // re-assigned (so a chain cycle can't loop). Empty (default) = a terminal step.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    TArray<TObjectPtr<UObjectiveDefinition>> NextObjectives;

    // Player-facing objective line (e.g. "Slay 5 wolves").
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FText DisplayText;

    // Optional player-facing line shown on completion.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FText CompletedText;

    // The quest this objective belongs to — the tracker groups objectives under one quest header. Empty = standalone.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    FText QuestName;

    // Optional (secondary) objective — the tracker shows it dimmed/italic; not required to finish the quest.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective")
    bool bOptional = false;

    /** The player-facing callout line for the objective's current state: the completion line once complete (if one was
     *  authored), otherwise the progress line. CompletedText is optional — falls back to DisplayText when empty, so an
     *  objective with no completion line keeps the prior behaviour. Pure (reads only this asset) → unit-testable. */
    FText GetCalloutText(bool bCompleted) const {
        return (bCompleted && !CompletedText.IsEmpty()) ? CompletedText : DisplayText;
    }
};
