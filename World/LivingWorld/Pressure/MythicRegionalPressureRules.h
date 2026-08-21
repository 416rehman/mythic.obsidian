
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicRegionalPressureRules.generated.h"

struct FMythicPressureKey {
    FIntPoint Cell = FIntPoint::ZeroValue;
    FGameplayTag Channel;

    FMythicPressureKey() = default;
    FMythicPressureKey(const FIntPoint &InCell, const FGameplayTag &InChannel) : Cell(InCell), Channel(InChannel) {}

    bool operator==(const FMythicPressureKey &Other) const { return Cell == Other.Cell && Channel == Other.Channel; }

    friend uint32 GetTypeHash(const FMythicPressureKey &Key) {
        return HashCombine(GetTypeHash(Key.Cell), GetTypeHash(Key.Channel));
    }
};

struct FMythicPressureCellState {
    float Value = 0.0f;
    double LastUpdateTime = 0.0;
};

USTRUCT(BlueprintType)
struct MYTHIC_API FMythicRegionalPressureConfig {
    GENERATED_BODY()

    /** Seconds between pressure checks (ONE repeating server timer, armed only while >= 1 rated source is live). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pressure", meta = (ClampMin = "10.0"))
    float CheckIntervalSeconds = 60.0f;

    /** Pressure shed per second per (cell, channel) accumulator — the lazy linear decay rate. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pressure", meta = (ClampMin = "0.0"))
    float DecayPerSecond = 0.005f;

    /** Cell edge (cm) for the fallback spatial quantizer when no territory grid is available. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Pressure", meta = (ClampMin = "1000.0"))
    float FallbackCellSizeCm = 12800.0f;

    // ── Farm channel (Pressure.Farm) ──
    /** Pressure a MATURE plot emits per second (a lone plot crosses the default threshold in ~25 min of ripeness). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "0.0"))
    float FarmPressurePerMaturePlotPerSecond = 0.01f;

    /** Farm pressure at which a raid telegraphs (before the scarecrow-deterrence threshold raise). <= 0 disables. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "0.0"))
    float FarmRaidThreshold = 15.0f;

    /** Each point of EFFECTIVE scarecrow deterrence raises the raid threshold by this fraction of itself
     *  (threshold × (1 + deterrence × this)). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "0.0"))
    float DeterrenceThresholdFactor = 0.5f;

    /** Seconds between the TELEGRAPH beat (chronicle line — "something stirs beyond the fields") and the spawn: the
     *  party ALWAYS gets a real warning window (P6 gate b). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "0.0"))
    float TelegraphDelaySeconds = 30.0f;

    /** A farm cell only raids while a player pawn is within this radius (cm) of the plot cluster — the ONLINE +
     *  PROXIMITY gate (P6 gate c + the C6 addition: no absent-owner losses, ever). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "500.0"))
    float PlayerNearRadius = 6000.0f;

    /** Minimum seconds between raids on the SAME farm cell (armed when a raid actually dispatches). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "0.0"))
    float PerCellCooldownSeconds = 1800.0f;

    /** NPC-type tag handed to UMythicNPCManager::SpawnRandomNPC per raider (CONTENT — unset = raids stay silent,
     *  warned once). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm")
    FGameplayTag RaidNPCType;

    /** Raiders in a base pack (pressure overshoot past the threshold scales it up to the max). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "1"))
    int32 RaidBaseCount = 2;

    /** Hard cap on raiders per raid. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "1"))
    int32 RaidMaxCount = 4;

    /** Raiders spawn on a ring near-but-not-on the farm: min ring radius (cm) — the perimeter grace window. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "500.0"))
    float MinSpawnDistance = 1500.0f;

    /** Max ring radius (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "500.0"))
    float MaxSpawnDistance = 3000.0f;

    /** Growth stages each plot in the raided cell regresses when the raid lands (NEVER uproots — C6 stage-regression,
     *  never seed loss; the plot floors at its seedling stage). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Farm", meta = (ClampMin = "1"))
    int32 StageRegressionPerRaid = 1;
};

struct FMythicRegionalPressureRules {
    static float ValueAtTime(float Value, double LastUpdateTime, double Now, float DecayPerSecond) {
        const double Gap = FMath::Max(0.0, Now - LastUpdateTime);
        return FMath::Max(0.0f, Value - FMath::Max(0.0f, DecayPerSecond) * static_cast<float>(Gap));
    }

    static float Resolve(FMythicPressureCellState &State, double Now, float DecayPerSecond) {
        State.Value = ValueAtTime(State.Value, State.LastUpdateTime, Now, DecayPerSecond);
        State.LastUpdateTime = FMath::Max(State.LastUpdateTime, Now);
        return State.Value;
    }

    static float Accumulate(FMythicPressureCellState &State, float Amount, double Now, float DecayPerSecond) {
        Resolve(State, Now, DecayPerSecond);
        State.Value += FMath::Max(0.0f, Amount);
        return State.Value;
    }

    static bool CrossesThreshold(float ValueBefore, float ValueAfter, float Threshold) {
        return Threshold > 0.0f && ValueBefore < Threshold && ValueAfter >= Threshold;
    }

    static float EffectiveRaidThreshold(float BaseThreshold, float EffectiveDeterrence, float DeterrenceThresholdFactor) {
        return FMath::Max(0.0f, BaseThreshold) *
               (1.0f + FMath::Max(0.0f, EffectiveDeterrence) * FMath::Max(0.0f, DeterrenceThresholdFactor));
    }

    static int32 RaidPackCount(float Value, float Threshold, int32 BaseCount, int32 MaxCount) {
        const int32 Base = FMath::Max(1, BaseCount);
        const int32 Max = FMath::Max(Base, MaxCount);
        if (Threshold <= 0.0f) {
            return Base;
        }
        const int32 Extra = FMath::FloorToInt(FMath::Max(0.0f, Value - Threshold) / Threshold);
        return FMath::Clamp(Base + Extra, Base, Max);
    }

    static FIntPoint QuantizeToCell(const FVector &WorldLocation, float CellSizeCm) {
        const float Size = FMath::Max(1.0f, CellSizeCm);
        return FIntPoint(FMath::FloorToInt(WorldLocation.X / Size), FMath::FloorToInt(WorldLocation.Y / Size));
    }
};
