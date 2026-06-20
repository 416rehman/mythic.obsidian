// Mythic Living World — Action Event Types
// Game-thread structs for submitting gameplay actions into the event pipeline.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "ActionEventTypes.generated.h"

// ─────────────────────────────────────────────────────────────
// Action Event — Game-thread input to the event pipeline
// ─────────────────────────────────────────────────────────────

/**
 * Lightweight struct submitted by gameplay systems (combat, crime, abilities)
 * to record an action in the Living World event pipeline.
 *
 * Actors are resolved to cell coordinates and factions at submission time,
 * then discarded — the pipeline operates on pure data from this point forward.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicActionEvent {
    GENERATED_BODY()

    /** The actor performing the action (resolved to cell + faction at submission) */
    UPROPERTY()
    TWeakObjectPtr<AActor> Perpetrator;

    /** The actor being acted upon, if any (resolved to cell + faction at submission) */
    UPROPERTY()
    TWeakObjectPtr<AActor> Victim;

    /** Moral vector of this action — contribution per moral axis */
    FMythicMoralAction MoralVector;

    /** Gameplay tag describing the action (e.g. "Action.Combat.MeleeKill") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Event", meta=(Categories="World.Action"))
    FGameplayTag ActionTag;

    /** Category for magic/ability classification (maps to moral axes) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Event")
    EMythicActionCategory ActionCategory = EMythicActionCategory::Melee;

    /**
     * Category bitmask for fast filtering (EMythicEventCategory).
     * Multiple categories can be OR'd together (e.g. Combat | Death).
     */
    uint16 CategoryFlags = 0;

    /** How significant this event is [0.0, 1.0]. Affects propagation priority and significance scoring. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Action Event", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Significance = 0.5f;

    /** Override cell coordinate — if set (X >= 0), used instead of deriving from Perpetrator position */
    FMythicCellCoord OverrideCell = FMythicCellCoord(-1, -1);

    /** Faction overrides — when valid, used instead of ResolveActorFaction (currently a stub). Lets the submit
     *  site supply factions it already knows (e.g. read from the actors' cognitive brains), so crime records carry
     *  real perpetrator/victim attribution. */
    FMythicFactionId PerpFactionOverride;
    FMythicFactionId VictimFactionOverride;
};

// ─────────────────────────────────────────────────────────────
// Witness Result — Output of witness perception, input to pressure
// ─────────────────────────────────────────────────────────────

/**
 * Result of a single witness evaluating an action event.
 * Produced by WitnessPerceptionProcessor, consumed by PressureProcessor.
 * Pure value type — no actor references, no UObject overhead.
 */
struct FMythicWitnessResult {
    /** MASS entity handle of the witness */
    FMassEntityHandle WitnessEntity;

    /** Moral severity as evaluated by the witness's faction ideology */
    EMythicMoralSeverity Severity = EMythicMoralSeverity::Ignore;

    /** The original action's moral vector (needed for pressure channel mapping) */
    FMythicMoralAction ActionMoralVector;

    /** Category flags from the original event (for pressure channel selection) */
    uint16 EventCategoryFlags = 0;

    /** Event significance (for weighted pressure accumulation) */
    float EventSignificance = 0.0f;

    /** World time of the event */
    double EventWorldTime = 0.0;

    /** Cell where the event occurred */
    FMythicCellCoord EventCell;

    /** Faction of the perpetrator */
    FMythicFactionId PerpFaction;
};

// ─────────────────────────────────────────────────────────────
// Pending Event — Queued for witness processing
// ─────────────────────────────────────────────────────────────

/**
 * A resolved action event ready for witness scanning.
 * All actor references have been resolved to IDs/cells/factions.
 */
struct FMythicPendingActionEvent {
    /** The world event that was written to the causal fabric */
    FMythicWorldEvent WorldEvent;

    /** Number of witnesses processed so far (for budget-capped multi-frame processing) */
    int32 WitnessesProcessed = 0;

    /** Whether this event has been fully processed by the witness system */
    bool bFullyProcessed = false;
};
