// Mythic Living World — Pressure Accumulation & Venting Processor
// Event-driven MASS processor: consumes witness results, accumulates pressure, triggers venting.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "PressureProcessor.generated.h"

class UMythicActionEventSubsystem;

/**
 * MASS processor that accumulates emotional pressure on hydrated entities
 * and triggers venting behavior when thresholds are crossed.
 *
 * Flow (event-driven — only runs when there are witness results):
 * 1. Consumes FMythicWitnessResult queue from the WitnessPerceptionProcessor
 * 2. For each result (budget-capped by MaxPressureEvalsPerFrame):
 *    a. Lazy decay — exponential decay from LastEventTime to now, O(1)
 *    b. Accumulate pressure into channels based on severity + category
 *    c. Check if any channel exceeds VentThreshold
 *    d. Route through personality VentWeights → determine vent behavior
 * 3. Outputs: sets behavioral tags on entities for downstream systems
 *
 * Only operates on hydrated (Tier 1+) entities with psychodynamic + personality fragments.
 * Zero cost when no witness results are pending.
 *
 * Budget: MaxPressureEvalsPerFrame per frame.
 * Cost: O(W × C) where W = witness results, C = pressure channels (6).
 */
UCLASS()
class MYTHIC_API UMythicPressureProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicPressureProcessor();

    /**
     * Despair is a RECOVERABLE state, not a permanent brand: an NPC enters despair when its total pressure reaches
     * DespairThreshold, and lifts out once pressure falls back below a hysteresis band (RecoveryFraction of the
     * threshold) so it does not flicker at the boundary. Previously bDespaired was set once and NEVER reset, so an NPC
     * that briefly spiked stayed "despaired" forever. Static + pure so the state machine is unit-testable.
     */
    static bool ComputeDespairState(float TotalPressure, float DespairThreshold, bool bWasDespaired);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for hydrated entities (have psychodynamic + personality fragments) */
    FMassEntityQuery HydratedEntityQuery;

    /** Cached subsystem pointer */
    TWeakObjectPtr<UMythicActionEventSubsystem> CachedActionSubsystem;
};
