// Mythic Living World — Significance Rescore & Promotion Processor
// Timer-driven MASS processor: rescores dirty entities and promotes/demotes between tiers.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/LivingWorldTypes.h" // FMythicCellCoord (used by value via TConstArrayView in ComputeProximityScore)
#include "SignificanceProcessor.generated.h"

class APlayerController;

/**
 * Cached per-player view for the embodiment view-gate (embodiment-service-LOCK-v1 §5b).
 * Snapshotted once per Execute on the game thread (PlayerCameraManager reads), then dot-tested against candidate
 * spawn positions with zero further allocation/traces. Mirrors the EXACT math in
 * UMythicActorSpawnProcessor::IsActorInCloseView (the despawn-side gate) so spawn + despawn agree on "in close view".
 */
struct FMythicPlayerView {
    /** World-space camera location. */
    FVector CamLocation = FVector::ZeroVector;

    /** Normalized camera forward (CameraRotation.Vector()). */
    FVector CamForward = FVector::ForwardVector;

    /** cos(half horizontal FOV + ViewConeMargin) — precomputed so the test is a single dot-product compare. */
    float CosHalfCone = -1.0f;
};

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

    /** view-cone test: is WorldPos within ViewGateMinSpawnDistance of, AND inside the (margin-widened) view cone of,
     *  ANY cached player view? Pure + static so the spawn/despawn view-gate math is unit-testable without a live world.
     *  Mirrors UMythicActorSpawnProcessor::IsActorInCloseView so the spawn-gate (this) and the despawn-gate (there)
     *  agree. MinSpawnDistance is in cm (compared squared internally). Empty PlayerViews → false (nothing to gate). */
    static bool IsInCloseView(const FVector &WorldPos, TConstArrayView<FMythicPlayerView> PlayerViews, float MinSpawnDistance);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for all entities with significance (all tiers) */
    FMassEntityQuery AllSignificanceQuery;

    /** Timer accumulator — rescoring runs at configured interval */
    float TimeSinceLastTick = 0.0f;

    // ─── Stream-in grace bookkeeping (embodiment-service-LOCK-v1 §5c) ───
    // Per-player: world time when the player was first seen OR last region-jumped (a single-tick Manhattan cell jump
    // >= RegionEnterCellJump). While Now - FirstSeenTime < StreamInGraceSeconds the view-gate is BYPASSED for that
    // player's window, so a town bulk-embodies behind the fade-in instead of popping in one NPC at a time. Game-thread
    // only (this processor is bRequiresGameThreadExecution=true) — no lock. Weak keys: pruned of stale/destroyed
    // controllers each Execute so the maps never grow unbounded. NOT UPROPERTYs (raw maps keyed on weak PC ptrs).
    TMap<TWeakObjectPtr<const APlayerController>, double> PlayerFirstSeenTime;

    /** Per-player: the player's cell on the PREVIOUS significance tick, to detect a region-enter teleport (a Manhattan
     *  jump >= RegionEnterCellJump re-arms the grace window above). Pruned alongside PlayerFirstSeenTime. */
    TMap<TWeakObjectPtr<const APlayerController>, FMythicCellCoord> PlayerLastCell;
};
