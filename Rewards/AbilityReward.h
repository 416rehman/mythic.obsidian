// 

#pragma once

#include "CoreMinimal.h"
#include "RewardBase.h"
#include "Abilities/GameplayAbility.h"
#include "AbilityReward.generated.h"

/**
 * This reward gives and or activates an ability.
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UAbilityReward : public URewardBase {
    GENERATED_BODY()

public:
    // The ability to give
    UPROPERTY(EditAnywhere, Blueprintable)
    TSubclassOf<UGameplayAbility> Ability;

    // Whether to activate it too
    UPROPERTY(EditAnywhere, Blueprintable)
    bool Activate = true;

    // Gives/activates an ability
    virtual bool Give(FRewardContext &Context) const override;

    // Helper function to get the context for the reward
    UFUNCTION(BlueprintCallable)
    static bool GiveAbilityReward(UAbilityReward *Reward, APlayerController *PlayerController) {
        auto Context = FRewardContext(PlayerController);
        return Reward->Give(Context);
    }
};
