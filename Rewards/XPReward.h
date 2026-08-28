
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "XPReward.generated.h"

struct FProficiency;

USTRUCT(BlueprintType, Blueprintable)
struct FXPRewardContext : public FRewardContext {
    GENERATED_BODY()

    /** Difficulty level of the rewarded action; zero disables relative-level scaling. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "XP Reward Context")
    int32 Level = 0;
};

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UXPReward : public URewardBase {
    GENERATED_BODY()

public:
    /** Canonical proficiency definition whose progress stat receives this reward. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP Reward")
    UProficiencyDefinition *ProficiencyDef;

    /** Fraction of the proficiency's Base XP Per Action awarded before relative-level scaling; 1.0 means 100%. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "XP Reward Context")
    float Percentage = 1.0f;

    /** Bonus fraction of the pre-scaled reward added per target level above the player's proficiency level. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Proficiency Track | Balancing")
    float OverlevelXPBonus = 0.5f;

    virtual bool Give(FRewardContext &Context) const override;

    /** Grants Reward using Context's receiving player and optional relative-level scaling inputs. */
    UFUNCTION(BlueprintCallable)
    static bool GiveXPReward(UXPReward *Reward, FXPRewardContext Context) {
        return Reward->Give(Context);
    }

    static float CalculateXP(UAbilitySystemComponent *AbilitySystemComponent, UProficiencyDefinition *Proficiency, int32 TargetLvl, float OverlevelBonus,
                             float PercentageOfActionXPtoGive);

    virtual FText GetPreviewText() const override;
};
