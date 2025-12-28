#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RewardBase.generated.h"

class UItemReward;
class ULootReward;
class UAbilityReward;
class UAttributeReward;
class UXPReward;

USTRUCT(Blueprintable, BlueprintType)
struct FRewardContext {
    GENERATED_BODY()

    // Constructor for the reward context
    FRewardContext() {}
    FRewardContext(APlayerController *PlayerController) : PlayerController(PlayerController) {}

    // The player that is receiving the reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Reward Context")
    APlayerController *PlayerController = nullptr;
};

/**
 * This is a base class for all rewards that can be given to the player.
 *
 * Rewards can be anything from items, to experience, to currency, to abilities, etc.
 * This should be subclassed to create specific rewards.
 */
UCLASS(Abstract)
class MYTHIC_API URewardBase : public UDataAsset {
    GENERATED_BODY()

public:
    virtual bool Give(FRewardContext &Context) const {
        return false;
    };

    /** 
     * Returns true if this reward can be safely reapplied on character load.
     * Override in subclasses:
     * - AttributeReward: true (idempotent, sets attribute values)
     * - AbilityReward: true (grants ability if not already granted)
     * - ItemReward: FALSE (items are saved in inventory, would duplicate)
     * - LootReward: FALSE (already spawned/claimed)
     * - XPReward: FALSE (would cause progression loops)
     */
    virtual bool CanReapplyOnLoad() const { return false; }
};

USTRUCT(BlueprintType, Blueprintable)
struct FRewardsToGive {
    GENERATED_BODY()

    // XP Reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UXPReward> XPReward;

    // Item Reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UItemReward> ItemReward;

    // Loot Reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<ULootReward> LootReward;

    // Ability Reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UAbilityReward> AbilityReward;

    // Attribute Reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UAttributeReward> AttributeReward;

    // Default helper to give reward to player. Uses default for each reward's context.
    // IsPrivateItem: if set, any items dropped will be private to the player
    // ItemLevel: if not ZERO, any items dropped will be at this level (otherwise, uses player's level)
    // Returns false if any error occured.
    bool Give(APlayerController *PlayerController, bool IsPrivateItem = true, int32 ItemLevel = 0) const;
};
