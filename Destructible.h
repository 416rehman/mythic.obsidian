#pragma once

#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "Destructible.generated.h"

UINTERFACE(NotBlueprintable, MinimalAPI)
class UDestructible : public UInterface {
    GENERATED_BODY()
};

class MYTHIC_API IDestructible {
    GENERATED_BODY()

public:
    virtual FRewardsToGive GetOnKillRewards(AActor *Killer = nullptr) = 0;
};
