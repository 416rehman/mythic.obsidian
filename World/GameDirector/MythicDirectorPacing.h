
#pragma once

#include "CoreMinimal.h"
#include "MythicDirectorPacing.generated.h"

UENUM(BlueprintType)
enum class EMythicDirectorPhase : uint8 {
    BuildUp,
    Peak,
    SustainPeak,
    Relax,
    Rest
};

USTRUCT(BlueprintType)
struct FMythicDirectorInputs {
    GENERATED_BODY()

    /** Party damage taken over the recent window, normalized to party max-health (0 = untouched; ~1 = a full party-worth of HP lost). */
    UPROPERTY(BlueprintReadWrite, Category = "Director")
    float RecentDamageTakenNorm = 0.0f;

    /** Co-op "downs" (incapacitations) counted within the recent window. */
    UPROPERTY(BlueprintReadWrite, Category = "Director")
    int32 DownsInWindow = 0;

    /** Party kills per second over the recent window (combat throughput / how fast enemies are being cleared). */
    UPROPERTY(BlueprintReadWrite, Category = "Director")
    float KillsPerSec = 0.0f;

    /** Seconds since the party last took damage / was downed (the "is anything happening?" idle detector). */
    UPROPERTY(BlueprintReadWrite, Category = "Director")
    float TimeSinceLastThreat = 0.0f;

    /** Mean normalized party health [0,1] (1 = full party health). */
    UPROPERTY(BlueprintReadWrite, Category = "Director")
    float AvgPartyHealthPct = 1.0f;
};

USTRUCT(BlueprintType)
struct FMythicDirectorConfig {
    GENERATED_BODY()


    /** Weight of normalized recent damage in the intensity blend. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Intensity", meta = (ClampMin = "0.0"))
    float DamageWeight = 0.5f;

    /** Weight of kill-throughput in the intensity blend. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Intensity", meta = (ClampMin = "0.0"))
    float KillsWeight = 0.3f;

    /** Weight of downs in the intensity blend. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Intensity", meta = (ClampMin = "0.0"))
    float DownsWeight = 0.2f;

    /** Kills/sec that maps to "full" kill intensity (throughput is clamped to this reference). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Intensity", meta = (ClampMin = "0.01"))
    float KillsPerSecReference = 1.5f;

    /** Downs-in-window that maps to "full" down intensity. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Intensity", meta = (ClampMin = "1"))
    int32 DownsReference = 2;


    /** Blended intensity at/above which BuildUp is allowed to crest into Peak. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float HighIntensityThreshold = 0.6f;

    /** Blended intensity at/below which SustainPeak is allowed to relax (combat calming). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowIntensityThreshold = 0.2f;

    /** Avg party HP at/below which the party is "exhausted" → forces SustainPeak to relax. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float LowHealthPct = 0.4f;

    /** Avg party HP at/above which the party is "recovered" → allowed to leave Rest for a new BuildUp. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Thresholds", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float RecoveredHealthPct = 0.75f;

    /** Downs-in-window at/above which SustainPeak is forced to relax (the party is getting wrecked). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Thresholds", meta = (ClampMin = "1"))
    int32 DownsForcingRelax = 1;

    /** Seconds of no-threat that lets SustainPeak relax early even if intensity hasn't fully decayed. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Thresholds", meta = (ClampMin = "0.0"))
    float ThreatIdleForRelax = 8.0f;


    /** Lower bound of the spawn-intensity multiplier (Rest floor). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Multiplier", meta = (ClampMin = "0.0"))
    float MinSpawnIntensityMultiplier = 0.35f;

    /** Upper bound of the spawn-intensity multiplier (Peak ceiling). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Multiplier", meta = (ClampMin = "0.0"))
    float MaxSpawnIntensityMultiplier = 1.75f;

    /** How fast the multiplier can rise toward its phase target (units/sec). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Multiplier", meta = (ClampMin = "0.0"))
    float MultiplierRisePerSec = 0.25f;

    /** How fast the multiplier can fall toward its phase target (units/sec). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Multiplier", meta = (ClampMin = "0.0"))
    float MultiplierFallPerSec = 0.5f;


    /** BuildUp will crest to Peak after this long even without high intensity (guarantees the world eventually escalates). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Dwell", meta = (ClampMin = "0.0"))
    float BuildUpDuration = 20.0f;

    /** Minimum time held in Peak before SustainPeak is entered (a single calm sample can't collapse the crest). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Dwell", meta = (ClampMin = "0.0"))
    float MinPeakDuration = 15.0f;

    /** Hard cap on continuous SustainPeak before a forced relax (no infinite peak). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Dwell", meta = (ClampMin = "0.0"))
    float SustainPeakMax = 30.0f;

    /** Time held in Relax before dropping to Rest. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Dwell", meta = (ClampMin = "0.0"))
    float RelaxDuration = 12.0f;

    /** Guaranteed breather: minimum time in Rest before a new BuildUp may begin (gated additionally on recovered HP). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Director|Dwell", meta = (ClampMin = "0.0"))
    float RestDuration = 15.0f;
};

USTRUCT(BlueprintType)
struct FMythicDirectorState {
    GENERATED_BODY()

    /** Current pacing phase. */
    UPROPERTY(BlueprintReadOnly, Category = "Director")
    EMythicDirectorPhase Phase = EMythicDirectorPhase::BuildUp;

    /** The multiplier consumers (EncounterDirector spawn probability, PopulationSpawner budget, …) read. Always within [Min,Max]. */
    UPROPERTY(BlueprintReadOnly, Category = "Director")
    float SpawnIntensityMultiplier = 1.0f;

    /** Seconds spent in the current phase (reset to 0 on every phase change). */
    UPROPERTY(BlueprintReadOnly, Category = "Director")
    float TimeInPhase = 0.0f;
};

struct FMythicDirectorPacing {
    static float ComputeIntensity(const FMythicDirectorInputs& In, const FMythicDirectorConfig& Cfg) {
        const float WSum = FMath::Max(KINDA_SMALL_NUMBER, Cfg.DamageWeight + Cfg.KillsWeight + Cfg.DownsWeight);
        const float DamageTerm = FMath::Clamp(In.RecentDamageTakenNorm, 0.0f, 1.0f);
        const float KillsTerm  = FMath::Clamp(In.KillsPerSec / FMath::Max(0.01f, Cfg.KillsPerSecReference), 0.0f, 1.0f);
        const float DownsTerm  = FMath::Clamp(static_cast<float>(In.DownsInWindow) / FMath::Max(1.0f, static_cast<float>(Cfg.DownsReference)), 0.0f, 1.0f);
        const float Blended = (Cfg.DamageWeight * DamageTerm + Cfg.KillsWeight * KillsTerm + Cfg.DownsWeight * DownsTerm) / WSum;
        return FMath::Clamp(Blended, 0.0f, 1.0f);
    }

    static float PhaseTargetMultiplier(EMythicDirectorPhase Phase, const FMythicDirectorConfig& Cfg) {
        switch (Phase) {
            case EMythicDirectorPhase::BuildUp:
            case EMythicDirectorPhase::Peak:
            case EMythicDirectorPhase::SustainPeak:
                return Cfg.MaxSpawnIntensityMultiplier;
            case EMythicDirectorPhase::Relax:
            case EMythicDirectorPhase::Rest:
            default:
                return Cfg.MinSpawnIntensityMultiplier;
        }
    }

    static FMythicDirectorState Step(const FMythicDirectorInputs& In, const FMythicDirectorConfig& Cfg,
                                     const FMythicDirectorState& Prev, float DeltaSeconds) {
        const float Dt = FMath::Max(0.0f, DeltaSeconds);

        const float Intensity = ComputeIntensity(In, Cfg);
        const bool bExhausted  = (In.AvgPartyHealthPct <= Cfg.LowHealthPct) || (In.DownsInWindow >= Cfg.DownsForcingRelax);
        const bool bCombatHot  = Intensity >= Cfg.HighIntensityThreshold;
        const bool bCombatCalm = (Intensity <= Cfg.LowIntensityThreshold) || (In.TimeSinceLastThreat >= Cfg.ThreatIdleForRelax);
        const bool bRecovered  = In.AvgPartyHealthPct >= Cfg.RecoveredHealthPct;

        FMythicDirectorState Next = Prev;
        Next.TimeInPhase = Prev.TimeInPhase + Dt;

        auto EnterPhase = [&Next](EMythicDirectorPhase NewPhase) {
            Next.Phase = NewPhase;
            Next.TimeInPhase = 0.0f;
        };

        switch (Prev.Phase) {
            case EMythicDirectorPhase::BuildUp:
                if (bCombatHot || Next.TimeInPhase >= Cfg.BuildUpDuration) {
                    EnterPhase(EMythicDirectorPhase::Peak);
                }
                break;
            case EMythicDirectorPhase::Peak:
                if (Next.TimeInPhase >= Cfg.MinPeakDuration) {
                    EnterPhase(EMythicDirectorPhase::SustainPeak);
                }
                break;
            case EMythicDirectorPhase::SustainPeak:
                if (bExhausted || bCombatCalm || Next.TimeInPhase >= Cfg.SustainPeakMax) {
                    EnterPhase(EMythicDirectorPhase::Relax);
                }
                break;
            case EMythicDirectorPhase::Relax:
                if (Next.TimeInPhase >= Cfg.RelaxDuration) {
                    EnterPhase(EMythicDirectorPhase::Rest);
                }
                break;
            case EMythicDirectorPhase::Rest:
                if (Next.TimeInPhase >= Cfg.RestDuration && bRecovered) {
                    EnterPhase(EMythicDirectorPhase::BuildUp);
                }
                break;
            default:
                break;
        }

        const float MinM = FMath::Min(Cfg.MinSpawnIntensityMultiplier, Cfg.MaxSpawnIntensityMultiplier);
        const float MaxM = FMath::Max(Cfg.MinSpawnIntensityMultiplier, Cfg.MaxSpawnIntensityMultiplier);
        const float Target = FMath::Clamp(PhaseTargetMultiplier(Next.Phase, Cfg), MinM, MaxM);
        float M = Prev.SpawnIntensityMultiplier;
        if (M < Target) {
            M = FMath::Min(Target, M + FMath::Max(0.0f, Cfg.MultiplierRisePerSec) * Dt);
        } else if (M > Target) {
            M = FMath::Max(Target, M - FMath::Max(0.0f, Cfg.MultiplierFallPerSec) * Dt);
        }
        Next.SpawnIntensityMultiplier = FMath::Clamp(M, MinM, MaxM);

        return Next;
    }

    static FMythicDirectorState MakeInitialState(const FMythicDirectorConfig& Cfg) {
        const float MinM = FMath::Min(Cfg.MinSpawnIntensityMultiplier, Cfg.MaxSpawnIntensityMultiplier);
        const float MaxM = FMath::Max(Cfg.MinSpawnIntensityMultiplier, Cfg.MaxSpawnIntensityMultiplier);
        FMythicDirectorState S;
        S.Phase = EMythicDirectorPhase::BuildUp;
        S.SpawnIntensityMultiplier = FMath::Clamp(1.0f, MinM, MaxM);
        S.TimeInPhase = 0.0f;
        return S;
    }
};
