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

    /** Authoritative player controller that owns and receives this reward. */
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

    /** Optional proficiency XP reward granted by this bundle. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UXPReward> XPReward;

    /** Optional exact item reward granted by this bundle. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UItemReward> ItemReward;

    /** Optional randomized loot-table reward granted by this bundle. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<ULootReward> LootReward;

    /** Optional Gameplay Ability reward granted by this bundle. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UAbilityReward> AbilityReward;

    /** Optional source-addressed permanent stat reward granted by this bundle. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<UAttributeReward> AttributeReward;

    /** Optional scoped faction, region, or global renown reward granted by this bundle. */
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Rewards")
    TObjectPtr<URenownReward> RenownReward;

    bool Give(APlayerController *PlayerController, bool IsPrivateItem = true, int32 ItemLevel = 0, FVector SpawnLocation = FVector::ZeroVector) const;

    FText GetPreviewText() const;
};
