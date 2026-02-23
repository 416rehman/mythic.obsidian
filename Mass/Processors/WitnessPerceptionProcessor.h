// Mythic Living World — Witness Perception Processor
// Event-driven MASS processor: scans nearby entities when action events occur.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "WitnessPerceptionProcessor.generated.h"

class UMythicActionEventSubsystem;

/**
 * MASS processor that evaluates witnesses to gameplay action events.
 *
 * Flow (event-driven — NOT per-tick polling):
 * 1. Checks ActionEventSubsystem for pending events (queued by SubmitAction)
 * 2. For each pending event (budget-capped by MaxWitnessEvalsPerFrame):
 *    a. Cell-based spatial lookup — O(1) entities in event cell + adjacent cells
 *    b. Hearing range filter — cell distance ≤ configurable radius
 *    c. Moral evaluation — dot product of action moral vector vs witness faction ideology
 *    d. Output: FMythicWitnessResult per non-Ignore witness → queued for PressureProcessor
 * 3. Tier 0 entities witnessing Condemn+ events are marked for hydration
 *
 * Budget: MaxWitnessEvalsPerFrame witnesses per frame, overflow defers to next frame.
 * Cost: O(E × W_cell) where E = events, W_cell = entities in event cell neighborhood.
 */
UCLASS()
class MYTHIC_API UMythicWitnessPerceptionProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicWitnessPerceptionProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for all entities with identity (potential witnesses) */
    FMassEntityQuery AllEntitiesQuery;

    /** Cached subsystem pointer — resolved on first Execute */
    TWeakObjectPtr<UMythicActionEventSubsystem> CachedActionSubsystem;
};
