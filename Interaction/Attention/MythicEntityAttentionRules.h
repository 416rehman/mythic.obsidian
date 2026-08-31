#pragma once

#include "CoreMinimal.h"
#include "Interaction/Attention/MythicEntityAttentionTypes.h"

/** Redacted scalar input used by deterministic attention scoring. */
struct MYTHIC_API FMythicEntityAttentionScoreInput {
    float DistanceCentimeters = 0.0f;
    float ViewAlignment = -1.0f;
    float SignalStrength = 1.0f;
    EMythicEntityAttentionPriorityClass PriorityClass = EMythicEntityAttentionPriorityClass::Ambient;
    bool bOnScreen = false;
    bool bHasLineOfSight = false;
    bool bInteractionTarget = false;
    bool bHardTarget = false;
    bool bInspectTarget = false;
};

/** Tiny deterministic counter enforcing the hard per-pass visibility-query ceiling. */
struct MYTHIC_API FMythicEntityAttentionTraceBudget {
    explicit FMythicEntityAttentionTraceBudget(const int32 InMaximum)
        : Maximum(FMath::Max(0, InMaximum)) {}

    /** Consumes one trace if capacity remains; false permits only a bounded cached-positive deferral before failing closed. */
    bool TryConsume() {
        if (Used >= Maximum) {
            return false;
        }
        ++Used;
        return true;
    }

    int32 Maximum = 0;
    int32 Used = 0;
};

/** Small deterministic visibility latch used to debounce fresh line-of-sight samples. */
struct MYTHIC_API FMythicEntityAttentionVisibilityState {
    double RawStateSinceSeconds = 0.0;
    bool bInitialized = false;
    bool bRawHasLineOfSight = false;
    bool bStableHasLineOfSight = false;
};

/** Pure attention math shared by runtime code and automation tests. */
struct MYTHIC_API FMythicEntityAttentionRules {
    /** Returns a sanitized copy whose cadence, thresholds, distances, and hard budgets are safe for runtime use. */
    static FMythicEntityAttentionConfig SanitizeConfig(const FMythicEntityAttentionConfig &Config);

    /** Returns the acquire or wider incumbent-retention gaze threshold without consulting world state. */
    static float GetGazeMinimumViewDot(bool bRetainingRecentGaze,
                                       const FMythicEntityAttentionConfig &Config);

    /** Advances a raw LOS transition and returns the debounced stable visibility verdict. */
    static bool UpdateStableLineOfSight(
        FMythicEntityAttentionVisibilityState &State, bool bRawHasLineOfSight,
        double NowSeconds, const FMythicEntityAttentionConfig &Config);

    /** Returns whether a trace-budget deferral may briefly preserve a previously stable positive LOS verdict. */
    static bool ShouldPreserveDeferredLineOfSight(
        const FMythicEntityAttentionVisibilityState &State,
        double SecondsSinceSample,
        const FMythicEntityAttentionConfig &Config);

    /** Returns true while a recent-gaze working-set entry is inside its bounded release grace. */
    static bool CanRetainRecentGaze(double SecondsSinceEligible,
                                    const FMythicEntityAttentionConfig &Config);

    /** Computes a deterministic nonnegative score without actor, world, relation, or private-state access. */
    static float CalculateScore(const FMythicEntityAttentionScoreInput &Input,
                                const FMythicEntityAttentionConfig &Config);

    /** Returns true when an explicit inspect, hard-target, or interaction owner should acquire focus immediately. */
    static bool IsForcedFocus(const FMythicEntityAttentionScoreInput &Input);

    /** Returns true once ordinary acquisition dwell has elapsed; explicit owners bypass dwell. */
    static bool CanAcquireFocus(float StableSeconds, bool bForced,
                                const FMythicEntityAttentionConfig &Config);

    /** Returns true while a valid incumbent is inside its configured release grace. */
    static bool CanRetainFocus(float SecondsSinceEligible,
                               const FMythicEntityAttentionConfig &Config);

    /** Returns true once a sufficiently stronger challenger has held its advantage for the replacement dwell. */
    static bool ShouldReplaceFocus(float IncumbentScore, float ChallengerScore,
                                   float ChallengerStableSeconds, bool bForced,
                                   const FMythicEntityAttentionConfig &Config);

    /** Stable semantic rank used before within-class score ordering. */
    static int32 GetPriorityRank(EMythicEntityAttentionPriorityClass PriorityClass);
};
