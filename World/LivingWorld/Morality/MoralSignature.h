
#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "MoralSignature.generated.h"


USTRUCT()
struct MYTHIC_API FMythicMoralAction {
    GENERATED_BODY()

    float AxisValues[MoralAxisCount] = {};
};


USTRUCT()
struct MYTHIC_API FMythicMoralAxisStats {
    GENERATED_BODY()

    int32 Count = 0;

    float Mean = 0.0f;

    float M2 = 0.0f;

    void Accumulate(float Value) {
        ++Count;
        const float Delta = Value - Mean;
        Mean += Delta / static_cast<float>(Count);
        const float Delta2 = Value - Mean;
        M2 += Delta * Delta2;
    }

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


USTRUCT(BlueprintType)
struct MYTHIC_API FMythicMoralSignature {
    GENERATED_BODY()

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

    int32 TotalActions = 0;

    float RecentMean[MoralAxisCount] = {};


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
            constexpr float RecentAlpha = 0.25f;
            for (int32 i = 0; i < MoralAxisCount; ++i) {
                RecentMean[i] = RecentMean[i] * (1.0f - RecentAlpha) + Action.AxisValues[i] * RecentAlpha;
            }
        }
        TrajectoryAngle = ComputeMoralTrajectoryAngle(RecentMean, MeanVec);
    }

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

    float EvaluateAgainst(const FMythicIdeologyProfile &Ideology) const {
        float DotProduct = 0.0f;
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            const EMythicMoralAxis Axis = static_cast<EMythicMoralAxis>(i);
            DotProduct += Axes[i].Mean * Ideology.GetAxis(Axis);
        }
        return DotProduct;
    }

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

    static FMythicMoralAction MakeKillActionMoralVector() {
        FMythicMoralAction V;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = 0.9f;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] = -0.5f;
        return V;
    }

    static FMythicMoralAction MakeMercyActionMoralVector() {
        FMythicMoralAction V;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Mercy)] = 0.8f;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Violence)] = -0.3f;
        return V;
    }

    static FMythicMoralAction MakeTrespassActionMoralVector() {
        FMythicMoralAction V;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Authority)] = -0.4f;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Loyalty)] = -0.2f;
        return V;
    }

    static FMythicMoralAction MakeTheftActionMoralVector() {
        FMythicMoralAction V;
        V.AxisValues[static_cast<int32>(EMythicMoralAxis::Theft)] = 0.7f;
        return V;
    }

    void GetMeanVector(float OutVector[MoralAxisCount]) const {
        for (int32 i = 0; i < MoralAxisCount; ++i) {
            OutVector[i] = Axes[i].Mean;
        }
    }

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
