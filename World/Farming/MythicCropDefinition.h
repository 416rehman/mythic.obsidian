
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "MythicCropDefinition.generated.h"

class UStaticMesh;
class UProficiencyDefinition;

UENUM(BlueprintType)
enum class EMythicCropStage : uint8 {
    Empty    UMETA(DisplayName = "Empty"),
    Growing  UMETA(DisplayName = "Growing"),
    Mature   UMETA(DisplayName = "Mature"),
    Withered UMETA(DisplayName = "Withered"),
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicCropDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Display name (details panel / harvest callout).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop")
    FText DisplayName;

    // Seconds each growth stage lasts before advancing to the next. Num() == number of growth stages; the crop is MATURE
    // once all of them have elapsed (mature stage index == Num()). Non-positive entries advance instantly.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Growth")
    TArray<float> StageDurations;

    // Per-stage soft mesh, indexed by the growth stage the plot is IN (0 = first seedling stage ... Num() = mature).
    // Ideally has StageDurations.Num() + 1 entries (one per growth stage plus the mature visual); the cosmetic BP event
    // indexes this defensively. Soft so unplanted plots don't force-load every crop mesh.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Growth")
    TArray<TSoftObjectPtr<UStaticMesh>> StageMeshes;

    // Rewards granted on harvest (items via ItemReward/LootReward — the yield count is scaled by YieldMin/Max + level).
    // This is the COMMON-tier block; the optional per-tier blocks below manifest the P1 quality tiers (C1: quality on
    // stackable produce = distinct item DEFINITIONS per tier). Unauthored tier blocks fall back down the ladder
    // (Pristine → Fine → this), so an unauthored crop routes these rewards for EVERY rolled tier — the inert default.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest")
    FRewardsToGive HarvestRewards;

    // OPTIONAL: rewards routed when the harvest rolls FINE quality (point ItemReward at the Fine item definition,
    // e.g. Wheat_Fine carrying Itemization.Quality.Fine). Empty = fall back to HarvestRewards.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest|Quality")
    FRewardsToGive HarvestRewardsFine;

    // OPTIONAL: rewards routed when the harvest rolls PRISTINE quality. Empty = fall back to Fine, then HarvestRewards.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest|Quality")
    FRewardsToGive HarvestRewardsPristine;

    // OPTIONAL: compost-feedstock rewards a WITHERED crop yields when cleared (C6: wither is never a total loss).
    // Only reachable when Farming.WitherAfterDrySeconds is authored > 0 (crops never wither by default).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest|Quality")
    FRewardsToGive WitheredHarvestRewards;

    // Farming proficiency track to grant XP to on harvest (reuses the existing Farming/Harvesting proficiency).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest")
    TObjectPtr<UProficiencyDefinition> FarmingProficiency;

    // Flat Farming XP granted per harvest (before the component's own scaling).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest")
    float FarmingXPOnHarvest = 10.0f;

    // Anti-grind: no Farming XP once the harvester's Farming level reaches/passes this (0 = no cap). Mirrors
    // ConversionStationComponent's XpNoGainAtOrAboveLevel.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest")
    int32 XpNoGainAtOrAboveFarmingLevel = 0;

    // Base yield range (inclusive) rolled at harvest, THEN scaled by the harvester's Farming level via
    // FMythicFarmingRules::ComputeHarvestYield.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest", meta = (ClampMin = "0"))
    int32 YieldMin = 1;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest", meta = (ClampMin = "0"))
    int32 YieldMax = 1;

    // Extra yield fraction per Farming level (fed to ComputeHarvestYield as BonusPerLevel).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest", meta = (ClampMin = "0"))
    float YieldBonusPerFarmingLevel = 0.05f;

    // Minimum Farming level required to plant this crop (0 = anyone can plant).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Plant", meta = (ClampMin = "0"))
    int32 MinFarmingLevelToPlant = 0;

    // Optional: this crop needs watering (the plot's secondary interact) to advance past a stage. Cosmetic hook for the
    // plot's SECONDARY interact; the base plot treats water as an optional accelerant.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Plant")
    bool bRequiresWater = false;

    // ── Wave L (FARMING DEPTH) ──
    // Crop IDENTITY leaf under Crop.Type.* (e.g. Crop.Type.Wheat). The bee hive counts DISTINCT leaves inside its
    // pollination radius for diversity-resonance honey. Unset = this crop doesn't count toward hive diversity.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Plant", meta = (Categories = "Crop.Type"))
    FGameplayTag CropTypeTag;

    // Seasons (Environment.Season.*) this crop may be PLANTED in. EMPTY = plantable year-round (the inert default).
    // Only bites when a calendar source exists (UMythicEnvironmentSubsystem::GetSeasonTag resolves); a season-less
    // world never blocks planting. Already-growing crops are never killed by a season change (gentle law).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Plant", meta = (Categories = "Environment.Season"))
    FGameplayTagContainer AllowedSeasonTags;

    // OPTIONAL preferred season (Environment.Season.*): harvesting during it adds the season-match quality bonus.
    // Unset = no seasonal quality input.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Plant", meta = (Categories = "Environment.Season"))
    FGameplayTag PreferredSeasonTag;

    // Multi-harvest: after harvesting, regrow from RegrowToStage instead of clearing the plot to empty.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest")
    bool bRegrowable = false;

    // The growth stage a regrowable crop resets to after harvest (skips the seedling stage so regrowth is faster).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Crop|Harvest", meta = (ClampMin = "0", EditCondition = "bRegrowable"))
    int32 RegrowToStage = 1;

    // Number of growth stages (== duration count). The MATURE stage index is exactly this value.
    UFUNCTION(BlueprintPure, Category = "Crop")
    int32 GetMatureStageIndex() const { return StageDurations.Num(); }
};
