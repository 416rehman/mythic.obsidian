// Mythic Living World — Crime Types
// Crime detection is emergent from the witness + moral evaluation pipeline.
// These types support the crime-to-belief-propagation reporting flow.

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Morality/MoralSignature.h"

// ─────────────────────────────────────────────────────────────
// Crime Severity — Mapped from moral evaluation thresholds
// ─────────────────────────────────────────────────────────────

/**
 * Crime is NOT defined by a hardcoded enum of crime types.
 * Instead, any action that scores above a faction's condemnation threshold
 * on their moral surface IS a crime for that faction.
 *
 * The same action can be legal in one territory and criminal in another,
 * depending on the governing faction's ideology.
 */

// ─────────────────────────────────────────────────────────────
// Crime Record — Produced by witness evaluation, consumed by
//                belief propagation for reporting
// ─────────────────────────────────────────────────────────────

/**
 * A lightweight record of a witnessed crime.
 * Created when WitnessPerceptionProcessor classifies an action at
 * Condemn or Hostile severity. Fed into the belief propagation network
 * for spreading awareness — NOT an instant alert system.
 *
 * Thread safety: created on game thread (WitnessProcessor), consumed on
 * game thread (BeliefPropagationProcessor). No cross-thread access.
 */
struct FMythicCrimeRecord {
    /** Entity that committed the crime (MASS handle index for Tier 0-1, or actor-backed) */
    int32 PerpEntityIndex = INDEX_NONE;

    /** Entity that was the victim, if any */
    int32 VictimEntityIndex = INDEX_NONE;

    /** Faction of the perpetrator */
    FMythicFactionId PerpFaction;

    /** Faction whose moral surface was violated (the "law" that was broken) */
    FMythicFactionId ViolatedFaction;

    /** Severity of the crime as evaluated by the violated faction's ideology */
    EMythicMoralSeverity Severity = EMythicMoralSeverity::Condemn;

    /** The moral vector of the action that triggered this crime */
    FMythicMoralAction ActionMoralVector;

    /** Where the crime occurred */
    FMythicCellCoord Cell;

    /** When the crime occurred */
    double WorldTime = 0.0;

    /** Number of witnesses who saw the crime directly. uint16: a large crowd can exceed 255 direct witnesses for a
     *  single event (e.g. a public massacre), which would wrap a uint8 and corrupt the witness tally / crime weight. */
    uint16 DirectWitnessCount = 0;

    /** How many hops this crime report has propagated through belief network */
    uint8 PropagationHops = 0;

    /**
     * Confidence of this report [0.0, 1.0].
     * Starts at 1.0 for direct witnesses.
     * Decays per propagation hop (from BeliefPropagationDecay setting).
     * Below a threshold → report is discarded (unreliable rumor).
     */
    float Confidence = 1.0f;

    /** Has this record been consumed by the belief propagation system? */
    bool bPropagated = false;
};

// ─────────────────────────────────────────────────────────────
// Crime Report Queue — Batch queue for crime → belief propagation
// ─────────────────────────────────────────────────────────────

/**
 * Lightweight queue for crime reports pending belief propagation.
 * Budget-capped on both the creation side (max crimes per frame from
 * WitnessProcessor) and the consumption side (max propagations per tick
 * from BeliefPropagationProcessor).
 */
struct FMythicCrimeReportQueue {
    /** Pending crime reports awaiting belief propagation */
    TArray<FMythicCrimeRecord> PendingReports;

    /** Maximum reports queued before oldest are discarded (ring behavior) */
    static constexpr int32 MaxQueuedReports = 64;

    void Enqueue(const FMythicCrimeRecord& Record) {
        if (PendingReports.Num() >= MaxQueuedReports) {
            PendingReports.RemoveAt(0, 1, EAllowShrinking::No); // Discard oldest
        }
        PendingReports.Add(Record);
    }

    int32 Num() const { return PendingReports.Num(); }
    bool IsEmpty() const { return PendingReports.Num() == 0; }

    void FlushPropagated() {
        PendingReports.RemoveAll([](const FMythicCrimeRecord& R) { return R.bPropagated; });
    }
};
