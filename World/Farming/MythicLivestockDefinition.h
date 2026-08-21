
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "MythicLivestockDefinition.generated.h"

class UProficiencyDefinition;

UCLASS(BlueprintType)
class MYTHIC_API UMythicLivestockDefinition : public UDataAsset {
    GENERATED_BODY()

public:
    // Display name (pen details / callouts).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock")
    FText DisplayName;

    // Species identity leaf (Item.Livestock.Chicken, ...) — also the registry key that mints this record.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock", meta = (Categories = "Item.Livestock"))
    FGameplayTag SpeciesTag;

    // Seconds of FED time per produce unit (egg/wool/milk). <= 0 = this species produces nothing (a pet).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Produce", meta = (ClampMin = "0.0"))
    float ProduceIntervalSeconds = 1800.0f;

    // Storage cap per animal — a full animal STALLS until collected (never dies, never overflows).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Produce", meta = (ClampMin = "1"))
    int32 MaxStoredUnitsPerAnimal = 3;

    // COMMON-tier produce rewards per unit (point ItemReward at the produce item def). The optional per-tier blocks
    // below manifest the P1 feed-quality tiers (C1: distinct item defs); unauthored tiers fall back down the ladder.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Produce")
    FRewardsToGive ProduceRewards;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Produce")
    FRewardsToGive ProduceRewardsFine;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Produce")
    FRewardsToGive ProduceRewardsPristine;

    // OPTIONAL manure co-product granted once per animal per collection (compost/bonemeal recipes are conversion
    // CONTENT — manure closes the feed→fertilizer→crop loop).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Produce")
    FRewardsToGive ManureRewards;

    // Seconds of fed time one Item.Feed.* unit grants.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Feed", meta = (ClampMin = "0.0"))
    float FeedSecondsPerUnit = 3600.0f;

    // Cap on how far ahead the feed clock may be banked (anti feed-dumping; activation-shaped upkeep). Default: two
    // feed units' worth.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Feed", meta = (ClampMin = "0.0"))
    float MaxFeedBankSeconds = 7200.0f;

    // Husbandry proficiency track granted XP on collection (a DATA asset — content). Unset = no XP.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Progression")
    TObjectPtr<UProficiencyDefinition> HusbandryProficiency;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Progression", meta = (ClampMin = "0.0"))
    float HusbandryXPPerUnit = 5.0f;

    // Anti-grind: no XP once the collector's level reaches/passes this (0 = no cap).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock|Progression", meta = (ClampMin = "0"))
    int32 XpNoGainAtOrAboveLevel = 0;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicLivestockRegistry : public UDataAsset {
    GENERATED_BODY()

public:
    // livestock item-type tag (Item.Livestock.*) → the species it mints. Mirrors UMythicCropRegistry::SeedToCrop.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Livestock Registry", meta = (Categories = "Item.Livestock"))
    TMap<FGameplayTag, TObjectPtr<UMythicLivestockDefinition>> ItemToLivestock;

    // Resolve the species a livestock item's effective type-probe mints: the first entry whose key the probe contains.
    // Returns nullptr if the probe carries no known livestock tag. Pure lookup — no engine state.
    UFUNCTION(BlueprintCallable, Category = "Livestock Registry")
    UMythicLivestockDefinition *ResolveLivestockForProbe(const FGameplayTagContainer &TypeProbe) const {
        for (const TPair<FGameplayTag, TObjectPtr<UMythicLivestockDefinition>> &Pair : ItemToLivestock) {
            if (Pair.Key.IsValid() && Pair.Value && TypeProbe.HasTag(Pair.Key)) {
                return Pair.Value;
            }
        }
        return nullptr;
    }
};
