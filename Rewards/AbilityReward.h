
#pragma once

#include "CoreMinimal.h"
#include "RewardBase.h"
#include "Abilities/GameplayAbility.h"
#include "AbilityReward.generated.h"

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

    virtual bool Give(FRewardContext &Context) const override;

    virtual FText GetPreviewText() const override;

    virtual bool CanReapplyOnLoad() const override { return true; }

    // Helper function to get the context for the reward
    UFUNCTION(BlueprintCallable)
    static bool GiveAbilityReward(UAbilityReward *Reward, APlayerController *PlayerController) {
        auto Context = FRewardContext(PlayerController);
        return Reward->Give(Context);
    }
};
