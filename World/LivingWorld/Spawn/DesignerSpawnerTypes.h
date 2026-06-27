// Mythic Living World — Designer Conditional Spawner: pure types + condition evaluator
//
// Designer-authored, server-authoritative spawner that pops a placed NPC when a set of designer conditions are met
// (time-of-day window, player-tag gate, optional player-proximity, faction lifecycle state, faction-relation state),
// with a persisted MaxSpawnsEver counter and perma-death via the PersistentNPCRegistry.
//
// This header carries ONLY pure, engine-light data + the header-inline MythicDesignerSpawner condition evaluator, so
// the automation-test translation unit links the evaluator WITHOUT pulling the actor .cpp (and thus without a render /
// world dependency). The actor + the persistence registry live in their own files.
//
// DETERMINISM: IsHourInWindow / EvaluateConditions are pure, branch-deterministic, allocate nothing, read no engine
// state — same inputs always yield the same answer (unit-tested).
//
// FAIL-SAFE: when a required gating faction or a required faction-relation cannot be resolved (faction not in the DB),
// EvaluateConditions returns FALSE — the spawner never pops on missing data.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "World/LivingWorld/Factions/FactionDatabase.h" // EMythicFactionStatus, EMythicFactionRelation
#include "DesignerSpawnerTypes.generated.h"

// ─────────────────────────────────────────────────────────────
// Predicates
// ─────────────────────────────────────────────────────────────

/**
 * Required lifecycle status of the gated faction for the spawn to be allowed.
 * `Alive` maps to EMythicFactionStatus::Active (there is no separate "Alive" status; bAlive is folded upstream — the
 * gather pass passes the faction's EMythicFactionStatus only).
 */
UENUM(BlueprintType)
enum class EMythicDesignerFactionStatePredicate : uint8 {
    Any = 0     UMETA(DisplayName = "Any (ignore faction state)"),
    Alive       UMETA(DisplayName = "Alive (status == Active)"),
    Annihilated UMETA(DisplayName = "Annihilated"),
    Resistance  UMETA(DisplayName = "Resistance"),
    Dormant     UMETA(DisplayName = "Dormant")
};

/**
 * Required relation between RelationFactionA and RelationFactionB for the spawn to be allowed.
 * `AtWar`   == EMythicFactionRelation::Hostile.
 * `AtPeace` == any relation that is NOT Hostile.
 */
UENUM(BlueprintType)
enum class EMythicDesignerRelationPredicate : uint8 {
    Ignore = 0 UMETA(DisplayName = "Ignore (no relation gate)"),
    AtWar      UMETA(DisplayName = "At War (Hostile)"),
    AtPeace    UMETA(DisplayName = "At Peace (not Hostile)")
};

// ─────────────────────────────────────────────────────────────
// Time window
// ─────────────────────────────────────────────────────────────

/**
 * A time-of-day window in game hours. Inclusive start, exclusive end: [StartHour, EndHour).
 * Overnight wrap is supported (e.g. Start=22, End=6 means 22:00..24:00 and 00:00..06:00).
 * When bEnabled is false the window is a no-op (always satisfied).
 */
USTRUCT(BlueprintType)
struct FMythicTimeWindow {
    GENERATED_BODY()

    /** When false, the time window imposes no gate (always satisfied). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner")
    bool bEnabled = false;

    /** Inclusive window start, game hours [0,24]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner", meta = (ClampMin = "0.0", ClampMax = "24.0", EditCondition = "bEnabled"))
    float StartHour = 0.0f;

    /** Exclusive window end, game hours [0,24]. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner", meta = (ClampMin = "0.0", ClampMax = "24.0", EditCondition = "bEnabled"))
    float EndHour = 24.0f;
};

// ─────────────────────────────────────────────────────────────
// Designer condition set (authored on the actor)
// ─────────────────────────────────────────────────────────────

/**
 * The full set of conditions a designer authors on an AMythicDesignerSpawner. ALL active gates must pass for a spawn.
 * Each gate is independently disable-able (empty tag / Any / Ignore / !bEnabled), so an empty condition set => always
 * spawnable (subject to the count/cooldown/concurrency caps on the actor).
 */
USTRUCT(BlueprintType)
struct FMythicDesignerConditionSet {
    GENERATED_BODY()

    /** Time-of-day gate. Disabled by default. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FMythicTimeWindow TimeWindow;

    /** A player must own ALL of these gameplay tags. Empty => no player-tag gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTagContainer RequiredPlayerTags;

    /** When true, at least one player must be within PlayerRangeCm of the spawner (in addition to any tag gate). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    bool bRequireAnyPlayerInRange = false;

    /** Proximity radius (cm) for bRequireAnyPlayerInRange and as the search radius for the player-tag gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions", meta = (ClampMin = "0.0"))
    float PlayerRangeCm = 10000.0f;

    /** The faction whose lifecycle state gates the spawn. Empty => no faction-state gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTag GatingFactionTag;

    /** Required lifecycle state of GatingFactionTag. `Any` ignores the gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    EMythicDesignerFactionStatePredicate FactionState = EMythicDesignerFactionStatePredicate::Any;

    /** First faction of the relation gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTag RelationFactionA;

    /** Second faction of the relation gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    FGameplayTag RelationFactionB;

    /** Required relation between A and B. `Ignore` disables the gate. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Designer Spawner|Conditions")
    EMythicDesignerRelationPredicate Relation = EMythicDesignerRelationPredicate::Ignore;
};

// ─────────────────────────────────────────────────────────────
// Evaluator inputs (PLAIN C++ — gathered each eval from live engine/sim state)
// ─────────────────────────────────────────────────────────────

/**
 * The resolved, engine-gathered inputs the PURE evaluator consumes. NOT a USTRUCT — it is a transient game-thread
 * read bundle, never serialized or exposed to Blueprint. The actor's GatherInputs() fills this from the environment
 * clock, players' ASCs, and the faction database; EvaluateConditions then decides with no engine access.
 */
struct FMythicDesignerConditionInputs {
    /** Current game hour in [0,24). */
    float GameHour = 0.0f;

    /** True if at least one player satisfied the combined player gate (tags + optional range). The actor folds the
     *  bRequireAnyPlayerInRange / RequiredPlayerTags semantics during gather so the evaluator only sees the result. */
    bool bAnyPlayerSatisfiesTags = false;

    /** Resolved lifecycle status of the gating faction (meaningful only if bGatingFactionResolved). */
    EMythicFactionStatus GatingFactionStatus = EMythicFactionStatus::Dormant;

    /** True if the gating faction tag resolved to a real faction in the DB. */
    bool bGatingFactionResolved = false;

    /** Resolved relation between RelationFactionA and RelationFactionB (meaningful only if bRelationResolved). */
    EMythicFactionRelation RelationAB = EMythicFactionRelation::Neutral;

    /** True if BOTH relation factions resolved to real factions in the DB. */
    bool bRelationResolved = false;
};

// ─────────────────────────────────────────────────────────────
// Persisted per-spawner state (keyed by DesignerId in the registry)
// ─────────────────────────────────────────────────────────────

/**
 * The durable, save/loaded state for one designer spawner, keyed by its stable FName DesignerId. The registry is the
 * AUTHORITATIVE store (survives placed-actor path-name churn); the actor mirrors it on BeginPlay.
 */
USTRUCT()
struct FMythicDesignerSpawnerState {
    GENERATED_BODY()

    /** Total NPCs this spawner has ever spawned (monotonic; gates MaxSpawnsEver). */
    UPROPERTY()
    int32 SpawnsEver = 0;

    /** True once the spawner has fired its terminal (perma-death) spawn; it never spawns again. */
    UPROPERTY()
    bool bPermaDead = false;

    /** World time of the most recent death of one of this spawner's NPCs (drives RespawnCooldownSeconds). */
    UPROPERTY()
    double LastDeathTime = 0.0;
};

// ─────────────────────────────────────────────────────────────
// Pure condition evaluator (header-inline so the test TU links without the actor .cpp)
// ─────────────────────────────────────────────────────────────

namespace MythicDesignerSpawner {

/**
 * Is GameHour inside the (inclusive-start, exclusive-end) window W, accounting for overnight wrap?
 *  - W disabled            => always true.
 *  - Start <= End (normal) => GameHour in [Start, End).
 *  - Start  > End (wrap)   => GameHour in [Start, 24) OR [0, End).
 * Pure; no engine state.
 */
inline bool IsHourInWindow(const FMythicTimeWindow& W, float GameHour) {
    if (!W.bEnabled) {
        return true;
    }
    if (W.StartHour <= W.EndHour) {
        return GameHour >= W.StartHour && GameHour < W.EndHour;
    }
    // Overnight wrap (e.g. 22 -> 6).
    return GameHour >= W.StartHour || GameHour < W.EndHour;
}

/** Map a designer faction-state predicate onto a concrete EMythicFactionStatus (Alive == Active). */
inline EMythicFactionStatus DesignerStateToStatus(EMythicDesignerFactionStatePredicate P) {
    switch (P) {
    case EMythicDesignerFactionStatePredicate::Alive:       return EMythicFactionStatus::Active;
    case EMythicDesignerFactionStatePredicate::Annihilated: return EMythicFactionStatus::Annihilated;
    case EMythicDesignerFactionStatePredicate::Resistance:  return EMythicFactionStatus::Resistance;
    case EMythicDesignerFactionStatePredicate::Dormant:     return EMythicFactionStatus::Dormant;
    default:                                                 return EMythicFactionStatus::Active;
    }
}

/**
 * Evaluate the full condition set against gathered inputs. ALL active gates must pass.
 *
 * Gate semantics:
 *  - Time:     IsHourInWindow(C.TimeWindow, In.GameHour).
 *  - Player:   if C.RequiredPlayerTags non-empty OR C.bRequireAnyPlayerInRange => requires In.bAnyPlayerSatisfiesTags.
 *  - Faction:  if C.FactionState != Any AND C.GatingFactionTag valid =>
 *                 FAIL-SAFE: must be resolved (bGatingFactionResolved) AND status == DesignerStateToStatus(FactionState).
 *  - Relation: if C.Relation != Ignore AND both relation tags valid =>
 *                 FAIL-SAFE: must be resolved (bRelationResolved) AND
 *                   AtWar  => RelationAB == Hostile
 *                   AtPeace=> RelationAB != Hostile
 *
 * An unresolved-but-required faction/relation returns FALSE (never spawn on missing data). Empty set => true.
 * Pure; no engine state.
 */
inline bool EvaluateConditions(const FMythicDesignerConditionSet& C, const FMythicDesignerConditionInputs& In) {
    // ── Time gate ──
    if (!IsHourInWindow(C.TimeWindow, In.GameHour)) {
        return false;
    }

    // ── Player gate (tags and/or proximity, folded into bAnyPlayerSatisfiesTags by the gather pass) ──
    const bool bPlayerGateActive = !C.RequiredPlayerTags.IsEmpty() || C.bRequireAnyPlayerInRange;
    if (bPlayerGateActive && !In.bAnyPlayerSatisfiesTags) {
        return false;
    }

    // ── Faction-state gate ──
    if (C.FactionState != EMythicDesignerFactionStatePredicate::Any && C.GatingFactionTag.IsValid()) {
        if (!In.bGatingFactionResolved) {
            return false; // fail-safe: required faction not in DB
        }
        if (In.GatingFactionStatus != DesignerStateToStatus(C.FactionState)) {
            return false;
        }
    }

    // ── Relation gate ──
    if (C.Relation != EMythicDesignerRelationPredicate::Ignore &&
        C.RelationFactionA.IsValid() && C.RelationFactionB.IsValid()) {
        if (!In.bRelationResolved) {
            return false; // fail-safe: a relation faction not in DB
        }
        const bool bHostile = (In.RelationAB == EMythicFactionRelation::Hostile);
        if (C.Relation == EMythicDesignerRelationPredicate::AtWar && !bHostile) {
            return false;
        }
        if (C.Relation == EMythicDesignerRelationPredicate::AtPeace && bHostile) {
            return false;
        }
    }

    return true;
}

} // namespace MythicDesignerSpawner
