
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "MythicRenownReward.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API URenownReward : public URewardBase {
    GENERATED_BODY()

public:
    /**
     * Faction, region, or global scope receiving this reward. Scoped grants also feed the global aggregate, so do
     * not author a duplicate global reward beside them.
     */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown Reward")
    FGameplayTag Scope;

    /** Signed renown delta; negative values represent infamy or reputation loss. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown Reward")
    float Amount = 100.0f;

    virtual bool Give(FRewardContext &Context) const override;
    virtual FText GetPreviewText() const override;

    virtual bool CanReapplyOnLoad() const override { return false; }
};
