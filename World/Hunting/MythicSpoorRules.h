
#pragma once

#include "CoreMinimal.h"
#include "MythicSpoorRules.generated.h"

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicSpoorConfig {
    GENERATED_BODY()

    /** Base lifetime (seconds) of a spoor node before it despawns (and before it reads fully stale). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoor", meta = (ClampMin = "10.0"))
    float NodeLifetimeSeconds = 600.0f;

    /** Lifetime multiplier while it is RAINING at spawn time (rain washes trails faster — < 1 shortens). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoor", meta = (ClampMin = "0.05", ClampMax = "1.0"))
    float RainLifetimeMultiplier = 0.5f;

    /** Freshness (0..1) below which a node is STALE — reading it reveals nothing (the trail went cold). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoor", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float StaleFreshnessThreshold = 0.15f;

    /** Distance (cm) between consecutive trail nodes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoor", meta = (ClampMin = "200.0"))
    float StepDistanceCm = 2500.0f;

    /** Max heading jitter (degrees, either side of dead-on) applied to each step toward the anchor — trails wander. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoor", meta = (ClampMin = "0.0", ClampMax = "80.0"))
    float StepJitterDegrees = 25.0f;

    /** ANTI-LITTER: hard cap on live spoor nodes per pressure cell-cluster region. Reads at the cap reveal nothing new. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spoor", meta = (ClampMin = "1"))
    int32 MaxNodesPerRegion = 12;
};

struct FMythicSpoorRules {
    static float EffectiveLifetime(float BaseLifetimeSeconds, bool bRaining, float RainMultiplier) {
        const float Base = FMath::Max(1.0f, BaseLifetimeSeconds);
        return bRaining ? Base * FMath::Clamp(RainMultiplier, 0.05f, 1.0f) : Base;
    }

    static float FreshnessAtAge(float AgeSeconds, float LifetimeSeconds) {
        const float Lifetime = FMath::Max(1.0f, LifetimeSeconds);
        return FMath::Clamp(1.0f - FMath::Max(0.0f, AgeSeconds) / Lifetime, 0.0f, 1.0f);
    }

    static bool IsStale(float Freshness, float StaleThreshold) {
        return StaleThreshold > 0.0f && Freshness < StaleThreshold;
    }

    static FVector NextStepLocation(const FVector &From, const FVector &Anchor, float StepDistanceCm, float JitterDegrees, float Rand01) {
        const FVector ToAnchor = (Anchor - From) * FVector(1.0f, 1.0f, 0.0f);
        const float Distance = ToAnchor.Size2D();
        const float Step = FMath::Max(1.0f, StepDistanceCm);
        if (Distance <= Step) {
            return FVector(Anchor.X, Anchor.Y, From.Z);
        }
        const FVector Dir = ToAnchor.GetSafeNormal2D();
        const float JitterRad = FMath::DegreesToRadians(FMath::Clamp(JitterDegrees, 0.0f, 80.0f)) * (FMath::Clamp(Rand01, 0.0f, 1.0f) * 2.0f - 1.0f);
        const FVector Jittered = Dir.RotateAngleAxisRad(JitterRad, FVector::UpVector);
        return From + Jittered * Step;
    }

    static bool IsFinalStep(const FVector &From, const FVector &Anchor, float StepDistanceCm) {
        return FVector::Dist2D(From, Anchor) <= FMath::Max(1.0f, StepDistanceCm);
    }
};
