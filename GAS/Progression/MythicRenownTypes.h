
#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "GAS/Progression/MythicRenownRules.h"
#include "Rewards/RewardBase.h"
#include "MythicRenownTypes.generated.h"

USTRUCT(BlueprintType)
struct FMythicRenownEntry {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Renown")
    FGameplayTag ScopeTag;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, SaveGame, Category = "Renown")
    float Value = 0.0f;

    FMythicRenownEntry() = default;
    FMythicRenownEntry(FGameplayTag InScopeTag, float InValue) : ScopeTag(InScopeTag), Value(InValue) {}
};

USTRUCT(BlueprintType)
struct FMythicRenownTierPayload {
    GENERATED_BODY()

    // Extra story tags stamped into the narrative ledger on reaching this tier, ON TOP of the automatic
    // "Renown.<ScopeLeaf>.<TierName>" mirror. Feed authored unlock rules / achievements / dialogue gates.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown")
    TArray<FGameplayTag> UnlockStoryTags;

    // Optional one-shot reward on reaching this tier (skipped during save-restore — restore never re-gives).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown")
    FRewardsToGive Rewards;
};

UCLASS(BlueprintType)
class MYTHIC_API UMythicRenownTierTable : public UDataAsset {
    GENERATED_BODY()

public:
    UMythicRenownTierTable() {
        Thresholds = {-6000.0f, -3000.0f, 0.0f, 3000.0f, 9000.0f, 21000.0f, 42000.0f};
        VendorDiscounts = {0.0f, 0.0f, 0.0f, 0.0f, 0.05f, 0.10f, 0.15f, 0.20f};
    }

    // 7 ASCENDING tier boundaries (Thresholds[i] = inclusive lower bound of tier i+1). Fewer entries = top tiers unreachable.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown")
    TArray<float> Thresholds;

    // Vendor discount fraction per tier (8 entries, Hated..Exalted). Short/empty = 0 for the missing tiers.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    TArray<float> VendorDiscounts;

    // Optional per-tier payloads, indexed by tier (up to 8 entries, Hated..Exalted). Missing entries grant nothing.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown")
    TArray<FMythicRenownTierPayload> TierPayloads;

    // The highest renown tier ACHIEVABLE toward a Faction.* scope while the player's faction STANDING tier is Hostile
    // (you cannot be Exalted with people actively hunting you). The stored value keeps accruing — recovering the
    // standing instantly restores the earned tier. Applied via the pure FMythicRenownRules::ClampToMaxTier.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown")
    EMythicRenownTier HostileStandingTierCap = EMythicRenownTier::Neutral;
};
