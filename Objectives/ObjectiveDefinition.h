
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "GAS/MythicTags_GAS.h"
#include "Narrative/MythicStoryCondition.h"
#include "ObjectiveDefinition.generated.h"

class UItemDefinition;
class UObjectiveDefinition;

UENUM(BlueprintType)
enum class EMythicObjectiveOutcome : uint8 {
    Completed,
    Spared,
    Killed,
    Failed
};

USTRUCT(BlueprintType)
struct FMythicObjectiveBranch {
    GENERATED_BODY()

    // The outcome this branch handles. Matched (hierarchical) against the completing event tag / payload tags. The
    // canonical kinds are named by EMythicObjectiveOutcome; authored as the event/payload tag that signifies them.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Branch")
    FGameplayTag OutcomeTag;

    // Objectives auto-assigned when this branch is taken (each still gated by its own PrerequisiteObjectives + the same
    // dedup/null-skip/cycle-safety as the default NextObjectives path).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Branch")
    TArray<TObjectPtr<UObjectiveDefinition>> NextObjectives;

    // Story tags stamped into the player's narrative ledger when this branch is taken (the choice's consequence).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Branch")
    FGameplayTagContainer GrantStoryTags;

    // Sibling objectives to ABANDON when this branch is taken — the mutually-exclusive OTHER path(s). Any currently
    // tracked, non-completed one is abandoned so the alternative route is genuinely closed (spare-vs-kill sticks).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Branch")
    TArray<TObjectPtr<UObjectiveDefinition>> CancelSiblings;
};

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

    // --- Narrative gating + agency (KEYSTONE) ---
    // Tag-driven PRECONDITION gating assignment: the offer is refused (EObjectiveOfferResult::PreconditionNotMet) unless
    // this condition passes against the player's owned story tags (∪ world flags). Empty (default) = ungated (assignment
    // depends only on prerequisites, the prior behaviour). Layered ON TOP of PrerequisiteObjectives.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Narrative")
    FMythicStoryCondition Precondition;

    // Story tags stamped into the player's narrative ledger when THIS objective completes (regardless of branch) — the
    // fact of having done it. Branch-specific consequences live on FMythicObjectiveBranch::GrantStoryTags instead.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Narrative")
    FGameplayTagContainer GrantStoryTagsOnComplete;

    // OUTCOME-ROUTED branches — the agency mechanic. On completion the achieved outcome (derived from HOW it finished)
    // selects the matching branch; its successors assign, its story tags stamp, and its CancelSiblings are abandoned. If
    // no branch matches (or none authored), completion falls back to the default NextObjectives (Completed) path — so
    // this is fully back-compatible. See UObjectiveTracker::SelectBranchForOutcome / DeriveAchievedOutcome.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Narrative")
    TArray<FMythicObjectiveBranch> OutcomeBranches;

    // --- Map / compass marker (W4) ---
    // When true, a location objective carries a world anchor (WorldMarkerLocation) that a HUD compass + war-map can
    // render as an Objective/Waypoint marker. Default false = no marker (unchanged behaviour). NOTE: wiring the
    // ObjectiveTracker to EMIT the marker and the actual compass/map WIDGET rendering are UI and remain a follow-up.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Map")
    bool bShowOnMap = false;

    // World-space anchor for this objective's map/compass marker. Only meaningful when bShowOnMap is true. Zero = unset.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Objective|Map", meta = (EditCondition = "bShowOnMap"))
    FVector WorldMarkerLocation = FVector::ZeroVector;

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

    // REPEATABLE (bounty/daily): a completed instance can be re-accepted from its offer source once RepeatCooldownSeconds
    // has elapsed (endless-content loop — bounty boards, dailies). Default false = one-shot (the original behavior; a
    // completed objective stays AlreadyCompleted forever). Chain objectives (NextObjectives) should stay one-shot.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Repeat")
    bool bRepeatable = false;

    // Cooldown (seconds) before a completed bRepeatable objective can be re-accepted. 0 = re-accept immediately on the
    // next offer. Ignored unless bRepeatable. (Runtime cooldown vs. server world-time; not persisted across a reload —
    // a repeatable bounty simply becomes available again after this many seconds of session uptime post-completion.)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Objective|Repeat", meta = (ClampMin = "0.0", EditCondition = "bRepeatable"))
    float RepeatCooldownSeconds = 0.0f;

    FText GetCalloutText(bool bCompleted) const {
        return (bCompleted && !CompletedText.IsEmpty()) ? CompletedText : DisplayText;
    }
};
