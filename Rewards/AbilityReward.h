
#pragma once

#include "CoreMinimal.h"
#include "RewardBase.h"
#include "Abilities/GameplayAbility.h"
#include "AbilityReward.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UAbilityReward : public URewardBase {
    GENERATED_BODY()

public:
    /** Gameplay Ability class permanently granted to the receiving player's Ability System Component. */
    UPROPERTY(EditAnywhere, Blueprintable)
    TSubclassOf<UGameplayAbility> Ability;

    /** When enabled, immediately attempts to activate the newly granted ability after a successful grant. */
    UPROPERTY(EditAnywhere, Blueprintable)
    bool Activate = true;

    virtual bool Give(FRewardContext &Context) const override;

    virtual FText GetPreviewText() const override;

    virtual bool CanReapplyOnLoad() const override { return true; }

    /** Builds a reward context for PlayerController and grants the configured ability on authority. */
    UFUNCTION(BlueprintCallable)
    static bool GiveAbilityReward(UAbilityReward *Reward, APlayerController *PlayerController) {
        auto Context = FRewardContext(PlayerController);
        return Reward->Give(Context);
    }
};
