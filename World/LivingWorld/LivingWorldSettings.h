// Mythic Living World System — Settings data asset
// Centralizes all configurable parameters for the living world system.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "LivingWorldSettings.generated.h"

/**
 * Master settings for the Living World System.
 * Create an instance of this data asset in the editor and assign it to the
 * Living World Subsystem (via project settings or direct reference).
 *
 * All configurable parameters live here — no hardcoded values in code.
 */
UCLASS(BlueprintType, Const)
class MYTHIC_API UMythicLivingWorldSettings : public UDataAsset {
    GENERATED_BODY()

public:
    // ─── Shared Data ──────────────────────────────────────

    /** Causal Fabric ring buffer capacity. Determines how many events are in active memory. */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Causal Fabric", meta = (ClampMin = "256", ClampMax = "65536"))
    int32 FabricCapacity = 4096;

    /** Faction database settings (initial factions, max count) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Factions")
    TSoftObjectPtr<UMythicFactionDatabaseSettings> FactionSettings;

    /** Territory grid settings (dimensions, cell size, bleed rate) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Territory")
    TSoftObjectPtr<UMythicTerritoryGridSettings> TerritorySettings;

    // ─── Background Thread ────────────────────────────────

    /**
     * Real-time interval between background simulation ticks (seconds).
     * Lower = more responsive world. Higher = less CPU.
     * At 1.0s real time, the sim runs at reasonable game-time intervals.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Simulation", meta = (ClampMin = "0.1", ClampMax = "10.0"))
    float SimTickIntervalSeconds = 1.0f;

    // ─── Budget Caps ──────────────────────────────────────

    /** Max significance promotions per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "20"))
    int32 MaxPromotionsPerFrame = 4;

    /** Max significance rescores per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxRescoresPerFrame = 20;

    /** Max pressure evaluations per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "200"))
    int32 MaxPressureEvalsPerFrame = 50;

    /** Max witness evaluations per event per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "500"))
    int32 MaxWitnessEvalsPerFrame = 100;

    /** Max causal event propagations per game frame */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "50"))
    int32 MaxPropagationsPerFrame = 10;

    /** Max cognitive NPC actors alive at any time */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Budgets", meta = (ClampMin = "1", ClampMax = "100"))
    int32 MaxCognitiveActors = 30;

    // ─── Significance ─────────────────────────────────────

    /** Score threshold to promote an NPC to a higher tier */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float PromotionThreshold = 0.7f;

    /** Score threshold to demote an NPC to a lower tier (hysteresis gap prevents oscillation) */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float DemotionThreshold = 0.3f;

    // ─── Weights for significance scoring ─────────────────

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance")
    float ProximityWeight = 0.4f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance")
    float EventCountWeight = 0.3f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Significance")
    float EmotionalIntensityWeight = 0.3f;
};
