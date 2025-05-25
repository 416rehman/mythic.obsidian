// 

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayEffectTypes.h"
#include "RewardBase.h"
#include "AttributeReward.generated.h"

// Attribute Reward. This improves an attribute of the player.
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UAttributeReward : public URewardBase {
    GENERATED_BODY()

public:
    // constructor
    UAttributeReward() {}

    UAttributeReward(FGameplayAttribute InAttribute, EGameplayModOp::Type InModifier, float InMagnitude)
        : Attribute(InAttribute), Modifier(InModifier), Magnitude(InMagnitude) {}

    // The attribute to modify
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    FGameplayAttribute Attribute;

    // The operation to perform on the attribute
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    TEnumAsByte<EGameplayModOp::Type> Modifier = EGameplayModOp::Additive;

    // The amount to modify the attribute by
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    float Magnitude = 1.0f;

    // Applies the reward to the attribute
    virtual bool Give(FRewardContext &Context) const override;

    // Helper function to get the context for the reward
    UFUNCTION(BlueprintCallable)
    static bool GiveAttributeReward(UAttributeReward *Reward, APlayerController *PlayerController) {
        auto Context = FRewardContext(PlayerController);
        return Reward->Give(Context);
    }
};
