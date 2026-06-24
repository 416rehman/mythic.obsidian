// Mythic Living World — Significance Rescore & Promotion Processor
// Timer-driven MASS processor: rescores dirty entities and promotes/demotes between tiers.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/LivingWorldTypes.h" // FMythicCellCoord (used by value via TConstArrayView in ComputeProximityScore)
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

    /** Compute proximity score: inverse cell (Manhattan) distance to the NEAREST player, normalized by SpawnRadius, in
     *  [0.0, 1.0]. Empty PlayerCells → 0.0; at a player's cell → 1.0; at/beyond SpawnRadius → 0.0. Pure + static so the
     *  core LOD-proximity math is unit-testable without a live world. Used by the rescore pass. */
    static float ComputeProximityScore(const FMythicCellCoord &EntityCell, TConstArrayView<FMythicCellCoord> PlayerCells, float SpawnRadius);

    /** Rescore gate (LOD policy): always re-evaluate the CAPPED cognitive hot-set (Tier1+) every significance tick so
     *  proximity stays fresh and an entity DEMOTES when players leave (a stationary embodied NPC otherwise kept a stale
     *  high score forever → cognitive-slot leak). The vast AMBIENT (Tier0) set stays event-driven (rescored only when
     *  bDirty). Pure + static so the policy is unit-testable. */
    static bool ShouldRescore(bool bDirty, EMythicSignificanceTier Tier);

    /** Tier-PROMOTION gate: score must clear the threshold by the full hysteresis margin (>= Threshold + Hysteresis).
     *  Pass Hysteresis = 0 for an un-margined gate (the Tier2 spawn threshold). Paired with QualifiesForDemotion, this
     *  creates the dead-band (Threshold_demote - H, Threshold_promote + H) in which an entity holds its tier — the
     *  anti-oscillation contract. Pure + static so the LOD promote/demote band is unit-testable. */
    static bool QualifiesForPromotion(float Score, float Threshold, float Hysteresis);

    /** Tier-DEMOTION gate: score must fall the full hysteresis margin BELOW the threshold (<= Threshold - Hysteresis).
     *  See QualifiesForPromotion for the dead-band it forms. Pure + static. */
    static bool QualifiesForDemotion(float Score, float Threshold, float Hysteresis);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for all entities with significance (all tiers) */
    FMassEntityQuery AllSignificanceQuery;

    /** Timer accumulator — rescoring runs at configured interval */
    float TimeSinceLastTick = 0.0f;
};
