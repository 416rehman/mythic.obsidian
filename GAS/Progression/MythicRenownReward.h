
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "MythicRenownReward.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API URenownReward : public URewardBase {
    GENERATED_BODY()

public:
    // The renown scope this reward feeds (a Faction.* tag, a region tag, or RENOWN_SCOPE_GLOBAL). Every grant also
    // feeds the global aggregate automatically — do NOT author a second global reward alongside a scoped one.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown Reward")
    FGameplayTag Scope;

    // Signed renown delta (negative = infamy/loss).
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Renown Reward")
    float Amount = 100.0f;

    virtual bool Give(FRewardContext &Context) const override;
    virtual FText GetPreviewText() const override;

    virtual bool CanReapplyOnLoad() const override { return false; }
};
