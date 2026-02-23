// Mythic Living World — Significance Rescore & Promotion Processor
// Timer-driven MASS processor: rescores dirty entities and promotes/demotes between tiers.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "SignificanceProcessor.generated.h"

/**
 * MASS processor that maintains significance scores and handles tier promotion/demotion.
 *
 * Two passes per tick (timer-driven, default 500ms):
 * 1. **Rescore pass** — entities with bDirty=true get their score recalculated:
 *    Score = ProximityWeight × proximity + EventCountWeight × eventCount + EmotionalIntensityWeight × pressure
 *    Budget: MaxRescoresPerFrame per tick.
 *
 * 2. **Promotion/demotion pass** — scored entities checked against tier thresholds:
 *    - Score > PromotionThreshold → promote (add hydration fragments)
 *    - Score < DemotionThreshold → demote (remove hydration fragments)
 *    - Hysteresis prevents oscillation at threshold boundaries.
 *    Budget: MaxPromotionsPerFrame per tick.
 *
 * Cost: O(D + P) where D = dirty entities rescored, P = promotions/demotions.
 * Amortized across timer interval — not per-frame.
 */
UCLASS()
class MYTHIC_API UMythicSignificanceProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicSignificanceProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for all entities with significance (all tiers) */
    FMassEntityQuery AllSignificanceQuery;

    /** Timer accumulator — rescoring runs at configured interval */
    float TimeSinceLastTick = 0.0f;

    /** Compute proximity score: inverse distance to nearest player as cell distance [0.0, 1.0] */
    float ComputeProximityScore(const FMythicCellCoord &EntityCell, float SpawnRadius) const;
};
