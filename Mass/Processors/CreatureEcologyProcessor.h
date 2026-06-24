// Mythic Living World — Creature Ecology Processor
// MASS processor for creature pack behavior, territorial defense, and herd-flee contagion.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "CreatureEcologyProcessor.generated.h"

/**
 * MASS processor that drives creature behavior through ecology rules.
 *
 * Three systems run each tick:
 * 1. **Pack Pressure Sharing** — Creatures in the same pack within radius share pressure state.
 *    When one wolf detects a threat, nearby pack members amplify their own Threat pressure.
 * 2. **Territorial Defense** — Creatures near their den cell get an aggression boost
 *    (BaseAggression + TerritorialAggressionBoost). Controls attack behavior near dens.
 * 3. **Herd-Flee Contagion** — When a herd creature's Threat pressure exceeds flee threshold,
 *    nearby herd members also raise their Threat (stampede behavior).
 *
 * Runs on server/standalone at configured interval, budget-capped.
 */
UCLASS()
class MYTHIC_API UMythicCreatureEcologyProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicCreatureEcologyProcessor();

    /** Effective territorial aggression for a creature THIS tick: BaseAggression, plus TerritorialBoost (clamped to 1)
     *  when near the den, else the bare BaseAggression. Recomputed from the AUTHORED base every tick — calling it
     *  repeatedly with the same inputs is idempotent (it does NOT accumulate), which is the whole point: the prior code
     *  did `BaseAggression += Boost` near the den with no decay, ratcheting aggression to 1.0 permanently. Pure + static
     *  for unit testing. */
    static float ComputeTerritorialAggression(float BaseAggression, bool bNearDen, float TerritorialBoost);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for creature entities with ecology data */
    FMassEntityQuery CreatureQuery;

    /** Query for hydrated creatures (have psychodynamic fragment for pressure sharing) */
    FMassEntityQuery HydratedCreatureQuery;

    /** Timer accumulator for throttling */
    float TimeSinceLastTick = 0.0f;
};
