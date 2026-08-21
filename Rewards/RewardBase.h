#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RewardBase.generated.h"

class UItemReward;
class ULootReward;
class UAbilityReward;
class UAttributeReward;
class UXPReward;
class URenownReward;
USTRUCT(Blueprintable, BlueprintType)
struct FRewardContext {
    GENERATED_BODY()

    FRewardContext() {}
    FRewardContext(APlayerController *PlayerController) : PlayerController(PlayerController) {}

    // The player that is receiving the reward
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Reward Context")
    APlayerController *PlayerController = nullptr;
};

UCLASS(Abstract)
class MYTHIC_API URewardBase : public UDataAsset {
    GENERATED_BODY()

public:
    virtual bool Give(FRewardContext &Context) const {
        return false;
    };

    virtual FText GetPreviewText() const { return FText::GetEmpty(); }

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

    // Renown Reward (scoped reputation toward a faction/region/global scope — see GAS/Progression/MythicRenownReward.h)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<URenownReward> RenownReward;

    bool Give(APlayerController *PlayerController, bool IsPrivateItem = true, int32 ItemLevel = 0, FVector SpawnLocation = FVector::ZeroVector) const;

    FText GetPreviewText() const;
};
