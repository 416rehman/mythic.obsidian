
#pragma once

#include "CoreMinimal.h"
#include "MythicHarvestPressureRules.generated.h"

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicHarvestPressureConfig {
    GENERATED_BODY()

    /** Pressure a single COMPLETED gather pushes onto its cell's Pressure.Harvest (the PUSH amount when
     *  ServerRegisterHarvest is called with a non-positive amount). Harmless while the read weights are 0. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0"))
    float HarvestPressurePerGather = 1.0f;

    /** DEPLETION WEIGHT — fraction of yield lost per unit of harvest pressure (multiplier = 1 - this*pressure, floored).
     *  0 (default) = INERT: yield never changes however hammered. Raise to enable commons depletion. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0"))
    float YieldDepletionPerPressure = 0.0f;

    /** Floor the yield multiplier can never drop below (a strip-mined vein still drops SOMETHING — no zero-yield). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float MinYieldMultiplier = 0.35f;

    /** Harvest pressure at/above which respawn is GATED (a ruined grove won't come back until it lies fallow enough to
     *  decay below this). 0 (default) = never gated (disabled — codebase "threshold <= 0" convention). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0"))
    float RespawnGateThreshold = 0.0f;

    /** DEPLETION WEIGHT — respawn-delay lengthening per unit of harvest pressure (delay ×= 1 + this*pressure, capped).
     *  0 (default) = INERT: regrowth time never changes. Raise so over-harvested cells regrow slower. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0"))
    float RespawnLengthenPerPressure = 0.0f;

    /** Cap on the respawn-delay multiplier (so an extreme pressure can't push regrowth to absurd durations). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "1.0"))
    float MaxRespawnDelayMultiplier = 4.0f;

    /** DEPLETION WEIGHT — harvest pressure per ONE produced-quality-tier drop (Pristine→Fine→Common, never below the
     *  source floor). 0 (default) = INERT: quality never degrades. Raise so a hammered cell yields shabbier goods. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0"))
    float PressurePerQualityTierDrop = 0.0f;

    /** Fallow recovery rate (pressure shed per second while a cell lies fallow) — DOCUMENTATION + the pure recovery
     *  curve for tests. PRODUCTION recovery is the shared RegionalPressure.DecayPerSecond the Harvest channel inherits
     *  (no Harvest-specific timer); this mirrors that so FallowRecover() models the same linear decay. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Harvest Pressure", meta = (ClampMin = "0.0"))
    float FallowRecoveryPerSecond = 0.005f;
};

struct FMythicHarvestPressureRules {
    static float DepletionYieldMultiplier(float HarvestPressure, const FMythicHarvestPressureConfig &Cfg) {
        const float P = FMath::Max(0.0f, HarvestPressure);
        const float Drop = FMath::Max(0.0f, Cfg.YieldDepletionPerPressure) * P;
        const float Floor = FMath::Clamp(Cfg.MinYieldMultiplier, 0.0f, 1.0f);
        return FMath::Clamp(1.0f - Drop, Floor, 1.0f);
    }

    static bool IsRespawnGated(float HarvestPressure, float Threshold) {
        return Threshold > 0.0f && FMath::Max(0.0f, HarvestPressure) >= Threshold;
    }

    static float RespawnDelayMultiplier(float HarvestPressure, const FMythicHarvestPressureConfig &Cfg) {
        const float P = FMath::Max(0.0f, HarvestPressure);
        const float Mult = 1.0f + FMath::Max(0.0f, Cfg.RespawnLengthenPerPressure) * P;
        return FMath::Clamp(Mult, 1.0f, FMath::Max(1.0f, Cfg.MaxRespawnDelayMultiplier));
    }

    static float FallowRecover(float Pressure, float DeltaSeconds, float Rate) {
        const float Dt = FMath::Max(0.0f, DeltaSeconds);
        return FMath::Max(0.0f, FMath::Max(0.0f, Pressure) - FMath::Max(0.0f, Rate) * Dt);
    }

    static int32 QualityTierDropSteps(float HarvestPressure, float PressurePerQualityTierDrop) {
        if (PressurePerQualityTierDrop <= 0.0f) {
            return 0;
        }
        return FMath::FloorToInt(FMath::Max(0.0f, HarvestPressure) / PressurePerQualityTierDrop);
    }
};
