// Mythic Living World System — Moral Signature
// O(1) incremental moral tracking using Welford's online algorithm

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "MoralSignature.generated.h"

// ─────────────────────────────────────────────────────────────
// Moral Action — Input to moral evaluation
// ─────────────────────────────────────────────────────────────

/** A single moral action expressed as contribution per axis. Passed into the signature accumulator. */
USTRUCT()
struct MYTHIC_API FMythicMoralAction {
    GENERATED_BODY()

    /** Contribution on each moral axis. Positive = aligned, negative = opposed. */
    float AxisValues[MoralAxisCount] = {};
};

// ─────────────────────────────────────────────────────────────
// Per-Axis Running Statistics — Welford's algorithm
// ─────────────────────────────────────────────────────────────

/** Incrementally maintained mean + variance for one moral axis. Uses Welford's online algorithm: O(1) per update, O(1) per query. */
USTRUCT()
struct MYTHIC_API FMythicMoralAxisStats {
    GENERATED_BODY()

    /** Running count of observations */
    int32 Count = 0;

    /** Running mean of observed values */
    float Mean = 0.0f;

    /** Running M2 aggregator (sum of squared deviations from the current mean) */
    float M2 = 0.0f;

    /** Update this axis with a new observation. Welford's O(1) update. */
    void Accumulate(float Value) {
        ++Count;
        const float Delta = Value - Mean;
        Mean += Delta / static_cast<float>(Count);
        const float Delta2 = Value - Mean;
        M2 += Delta * Delta2;
    }

    /** Variance of observations. Returns 0 if fewer than 2 observations. */
    float GetVariance() const {
        return Count >= 2 ? M2 / static_cast<float>(Count) : 0.0f;
    }

    bool NetSerialize(FArchive &Ar, class UPackageMap *Map, bool &bOutSuccess) {
        Ar << Count;
        Ar << Mean;
        Ar << M2;
        bOutSuccess = true;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMythicMoralAxisStats> : public TStructOpsTypeTraitsBase2<FMythicMoralAxisStats> {
    enum { WithNetSerializer = true };
};

// ─────────────────────────────────────────────────────────────
// Moral Signature — Cached per-entity/per-player moral profile
// ─────────────────────────────────────────────────────────────

/**
 * Incremental moral profile maintained with O(1) cost per action.
 * Stored per-player for reputation tracking and per-NPC for moral evaluation.
 *
 * Evaluation against a faction tolerance vector is a single dot product: O(1).
 *
 * ContradictionScore tracks how self-contradictory an entity's actions are
 * (high variance across axes = unpredictable/chaotic actor).
 *
 * TrajectoryAngle tracks how much the entity's moral direction has shifted
 * recently vs. historical behavior (redemption / corruption arcs).
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicMoralSignature {
    GENERATED_BODY()

    /** Per-axis running statistics */
    FMythicMoralAxisStats Axes[MoralAxisCount];

    /**
     * High variance across axes means erratic, contradictory behavior.
     * Cached and recomputed on accumulation.
     */
    UPROPERTY(BlueprintReadOnly)
    float ContradictionScore = 0.0f;

    /**
     * Angle (radians, [0, PI]) between the RECENT behavior vector (an EMA, see RecentMean) and the historical mean
     * vector. ~0 = behaving consistently with history; large = the moral direction has shifted recently — a redemption
     * (toward the faction) or corruption (away) arc. Cached on accumulation (like ContradictionScore).
     */
    UPROPERTY(BlueprintReadOnly)
    float TrajectoryAngle = 0.0f;

    /**
     * Index of the axis with the highest absolute mean — the entity's dominant moral characteristic.
     * Cached on accumulation.
     */
    UPROPERTY(BlueprintReadOnly)
    uint8 DominantAxis = 0;

    /** Total count of moral actions accumulated */
    int32 TotalActions = 0;

    /**
     * Recent-behavior EMA per axis — the "recent" vector for TrajectoryAngle. TRANSIENT: deliberately NOT a UPROPERTY
     * and NOT in NetSerialize, so it is neither replicated nor saved (recent behavior is a live, session-local signal;
     * the derived TrajectoryAngle IS replicated/persisted). Seeded from the historical mean on the first accumulation
     * after construction OR load (while still all-zero), so a fresh signature reads "no arc" until behavior diverges.
     */
    float RecentMean[MoralAxisCount] = {};

    // ─────────────────────────────────────────────────────────

    /** Accumulate a new moral action. O(1). Updates all cached derived values. */
    void AccumulateAction(const FMythicMoralAction &Action) {
        ++TotalActions;

        float MaxAbsMean = 0.0f;
        float VarianceSum = 0.0f;

        for (int32 i = 0; i < MoralAxisCount; ++i) {
            Axes[i].Accumulate(Action.AxisValues[i]);

            const float AbsMean = FMath::Abs(Axes[i].Mean);
            if (AbsMean > MaxAbsMean) {
                MaxAbsMean = AbsMean;
                DominantAxis = static_cast<uint8>(i);
            }

            VarianceSum += Axes[i].GetVariance();
        }

        ContradictionScore = VarianceSum / static_cast<float>(MoralAxisCount);

        // ─── Recent-behavior EMA + trajectory arc ───
        // The historical mean was just updated above. Update the recent EMA, then cache the angle between recent and
        // historical. Seed the EMA from the mean whenever it is still all-zero (first-ever action OR a post-load/
        // post-construct signature — RecentMean is transient) so a fresh signature reads angle 0 until behavior diverges.
        float MeanVec[MoralAxisCount];
        bool bRecentZero = true;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            MeanVec[i] = Axes[i].Mean;
            if (RecentMean[i] != 0.0f) {
                bRecentZero = false;
            }
        }
        if (bRecentZero) {
            for (int32 i = 0; i < MoralAxisCount; ++i) {
                RecentMean[i] = MeanVec[i];
            }
        } else {
            constexpr float RecentAlpha = 0.25f; // ~4-action recent window
            for (int32 i = 0; i < MoralAxisCount; ++i) {
                RecentMean[i] = RecentMean[i] * (1.0f - RecentAlpha) + Action.AxisValues[i] * RecentAlpha;
            }
        }
        TrajectoryAngle = ComputeMoralTrajectoryAngle(RecentMean, MeanVec);
    }

    /**
     * Pure angle (radians, [0, PI]) between two moral vectors — the recent-behavior EMA and the historical mean.
     * Returns 0 when either vector is degenerate (no behaviour accumulated → no detectable arc). Static + pure so the
     * trajectory math is unit-testable without a populated signature.
     */
    static float ComputeMoralTrajectoryAngle(const float Recent[MoralAxisCount], const float Historical[MoralAxisCount]) {
        float Dot = 0.0f;
        float RecentSq = 0.0f;
        float HistSq = 0.0f;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            Dot += Recent[i] * Historical[i];
            RecentSq += Recent[i] * Recent[i];
            HistSq += Historical[i] * Historical[i];
        }
        const float Denom = FMath::Sqrt(RecentSq) * FMath::Sqrt(HistSq);
        if (Denom <= KINDA_SMALL_NUMBER) {
            return 0.0f;
        }
        const float CosAngle = FMath::Clamp(Dot / Denom, -1.0f, 1.0f);
        return FMath::Acos(CosAngle);
    }

    /**
     * Evaluate this signature against a faction's tolerance thresholds.
     * Returns a dot product: positive = aligned, negative = opposed.
     * O(1) — just one multiply-accumulate per axis.
     */
    float EvaluateAgainst(const FMythicIdeologyProfile &Ideology) const {
        float DotProduct = 0.0f;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            const EMythicMoralAxis Axis = static_cast<EMythicMoralAxis>(i);
            DotProduct += Axes[i].Mean * Ideology.GetAxis(Axis);
        }
        return DotProduct;
    }

    /**
     * Evaluate a single action against a tolerance vector to determine severity.
     * Used by the witness perception system to evaluate a crime in real-time.
     */
    static EMythicMoralSeverity EvaluateActionSeverity(
        const FMythicMoralAction &Action,
        const FMythicIdeologyProfile &Ideology,
        float DisapproveThreshold,
        float CondemnThreshold,
        float HostileThreshold) {
        float DotProduct = 0.0f;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            const EMythicMoralAxis Axis = static_cast<EMythicMoralAxis>(i);
            DotProduct += Action.AxisValues[i] * Ideology.GetAxis(Axis);
        }

        // Negative dot product means action opposes faction values
        const float Severity = -DotProduct;

        if (Severity >= HostileThreshold) {
            return EMythicMoralSeverity::Hostile;
        }
        if (Severity >= CondemnThreshold) {
            return EMythicMoralSeverity::Condemn;
        }
        if (Severity >= DisapproveThreshold) {
            return EMythicMoralSeverity::Disapprove;
        }
        return EMythicMoralSeverity::Ignore;
    }

    /**
     * The canonical moral vector of a lethal-violence / kill action. SINGLE SOURCE (Rule 3) for BOTH the live kill
     * path (MythicLifeComponent) and the persistent-NPC death record (PersistentNPCRegistry) so the sign can never
     * drift between them again. CONVENTION (FactionDatabase.h:66 "+1.0 glorifies combat / -1.0 total pacifism";
     * NPCGenerator Fight=+Violence; the whole test suite): a harmful act is POSITIVE on the Violence axis, so an
     * anti-violence faction (Ideology.Violence < 0) yields a POSITIVE Severity (= -DotProduct) → condemnation. Mercy
     * is NEGATIVE because a kill is the absence of mercy.
     */
    static FMythicMoralAction MakeKillActionMoralVector() {
        FMythicMoralAction V;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.9f;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] = -0.5f;
        return V;
    }

    /** Get the mean moral vector for signatures comparison / trajectory */
    void GetMeanVector(float OutVector[MoralAxisCount]) const {
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            OutVector[i] = Axes[i].Mean;
        }
    }

    /** Reset all accumulated data */
    void Reset() {
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            Axes[i] = FMythicMoralAxisStats();
        }
        ContradictionScore = 0.0f;
        TrajectoryAngle = 0.0f;
        DominantAxis = 0;
        TotalActions = 0;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            RecentMean[i] = 0.0f;
        }
    }

    bool NetSerialize(FArchive &Ar, class UPackageMap *Map, bool &bOutSuccess) {
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            Axes[i].NetSerialize(Ar, Map, bOutSuccess);
        }
        Ar << ContradictionScore;
        Ar << TrajectoryAngle;
        Ar << DominantAxis;
        Ar << TotalActions;
        bOutSuccess = true;
        return true;
    }
};

template <>
struct TStructOpsTypeTraits<FMythicMoralSignature> : public TStructOpsTypeTraitsBase2<FMythicMoralSignature> {
    enum { WithNetSerializer = true };
};
