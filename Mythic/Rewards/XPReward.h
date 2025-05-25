// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "XPReward.generated.h"

struct FProficiency;

USTRUCT(BlueprintType, Blueprintable)
struct FXPRewardContext : public FRewardContext {
    GENERATED_BODY()

    // The level at which to evaluate the XP. I.e if player killed level 10 monster, this should be 10.
    // Final XP will be (Percentage * BaseXPPerAction) + ((TargetLevel - PlayerLevel) * (Percentage * BaseXPPerAction))
    // If TargetLevel is 0, then no scaling is done.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP Reward Context")
    int32 Level = 0;
};

/**
 * Gives static amount of XP to the player.
 */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UXPReward : public URewardBase {
    GENERATED_BODY()

public:
    // The proficiency to which the XP should be given
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP Reward")
    UProficiencyDefinition *ProficiencyDef;

    // The percentage of the BaseXPPerAction that should be given.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP Reward Context")
    float Percentage = 1.0f;

    // Overlevel XP Bonus Percent. For every level above the player's level, reward this much more XP per action.
    // i.e If BaseXPPerAction is 10 and OverlevelXPBonus is 0.5, and player is level 10 and enemy is level 15:
    // Final Reward XP = BaseXPPerAction + ((EnemyLevel - PlayerLevel) * BaseXPPerAction * OverlevelXPBonus)
    // 35 = 10 + ((15 - 10) * 10 * 0.5)
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    float OverlevelXPBonus = 0.5f;

    // Gives the reward to the player
    virtual bool Give(FRewardContext &Context) const override;

    // Helper function to get the context for the reward
    // Level is the level at which to evaluate the XP. I.e if player killed level 10 monster, this should be 10.
    // Final XP will be (Percentage * BaseXPPerAction) + ((TargetLevel - PlayerLevel) * (Percentage * BaseXPPerAction))
    // If TargetLevel is 0, then no scaling is done.
    UFUNCTION(BlueprintCallable)
    static bool GiveXPReward(UXPReward *Reward, FXPRewardContext Context) {
        return Reward->Give(Context);
    }

    // Algorithm for scaling XP based on level
    static float CalculateXP(UAbilitySystemComponent *AbilitySystemComponent, UProficiencyDefinition *Proficiency, int32 TargetLvl, float OverlevelBonus,
                             float PercentageOfActionXPtoGive);
};
