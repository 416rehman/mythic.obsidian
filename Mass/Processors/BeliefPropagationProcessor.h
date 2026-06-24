// Mythic Living World — Belief Propagation Processor
// Timer-driven MASS processor: propagates beliefs through the social graph.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "BeliefPropagationProcessor.generated.h"

/**
 * MASS processor that propagates beliefs between connected entities via social edges.
 *
 * Implements REQ-CRM-004: "Reporting via belief propagation through social network
 * (rate-limited, mutating, budget-capped). No instant alert system."
 *
 * How it works:
 * 1. Timer-driven (2s intervals) — not per-frame
 * 2. Scans hydrated entities (Tier 1+) that have social edges
 * 3. For each high-pressure entity that recently witnessed events, propagates significance-AWARENESS
 *    (the dirty flag + a hop-decayed RelevantEventCount) to its social neighbors via social edges
 * 4. Each propagation reduces the contributed event count by BeliefPropagationDecay (hop decay)
 * 5. Total propagations per tick capped by MaxBeliefPropagationsPerTick
 *
 * Spread is naturally bounded to ~one hop per witness: a propagated-to entity gains awareness but no pressure, so it
 * does not itself re-propagate. The per-belief MAX-HOP cap (MaxBeliefPropagationHops) applies to the cognitive belief
 * path (UMythicPartySubsystem::ShouldShareBelief), not to this awareness spread.
 *
 * This allows crime reports to spread through the social network organically:
 * Witness → friend → guard → investigation. Not instant — takes real game time.
 *
 * Cost: O(H) where H = hydrated entities with edges. Budget-capped to
 * MaxBeliefPropagationsPerTick per tick.
 */
UCLASS()
class MYTHIC_API UMythicBeliefPropagationProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicBeliefPropagationProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for hydrated entities with social edges */
    FMassEntityQuery HydratedSocialQuery;

    /** Timer accumulator */
    float TimeSinceLastTick = 0.0f;
};
