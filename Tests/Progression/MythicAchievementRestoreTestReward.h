#pragma once
#include "Rewards/AbilityReward.h"
#include "MythicAchievementRestoreTestReward.generated.h"

/** Counts delivery through the real reward bundle to detect replay during restore. */
UCLASS(NotBlueprintable,Hidden)
class UMythicAchievementRestoreTestReward final : public UAbilityReward {
    GENERATED_BODY()
public:
    mutable int32 GiveCount=0;
    virtual bool Give(FRewardContext& Context) const override {
        ++GiveCount;
        return Super::Give(Context);
    }
};
