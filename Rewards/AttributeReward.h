
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectTypes.h"
#include "RewardBase.h"
#include "Stats/MythicStatTypes.h"
#include "AttributeReward.generated.h"

/** Reusable permanent-stat reward template applied through the authoritative source-addressed stat ledger. */
UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UAttributeReward : public URewardBase {
    GENERATED_BODY()

public:
    UAttributeReward() {}

    /**
     * Stable operational identity for direct grants. Contextual compilers such as proficiency tracks replace this on
     * a transient clone with an identity derived from the typed owning definition and reward placement.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    FGuid PermanentSourceGuid;

    /** Canonical Stat Definition whose registered GAS attribute receives this permanent reward. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    FMythicStatDefinitionHandle TargetStat;

    /** Arithmetic operation applied to the unmodified permanent base. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    TEnumAsByte<EGameplayModOp::Type> Modifier = EGameplayModOp::Additive;

    /** Finite operand used by the selected arithmetic operation. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attribute Reward")
    float Magnitude = 1.0f;

    virtual bool Give(FRewardContext &Context) const override;

    virtual FText GetPreviewText() const override;

    virtual bool CanReapplyOnLoad() const override { return true; }

#if WITH_EDITOR
    /** Validates the typed stat target and permanent-ledger operation before the reward can ship. */
    virtual EDataValidationResult IsDataValid(FDataValidationContext &Context) const override;
#endif

    /** Gives this reward through the supplied player's authoritative permanent-stat ledger. */
    UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Attribute Reward")
    static bool GiveAttributeReward(UAttributeReward *Reward, APlayerController *PlayerController) {
        if (!Reward || !PlayerController) {
            return false;
        }
        auto Context = FRewardContext(PlayerController);
        return Reward->Give(Context);
    }
};
