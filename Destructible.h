#pragma once

#include "GameplayTagContainer.h"
#include "Rewards/RewardBase.h"
#include "Destructible.generated.h"

class UMythicDestructiblesManagerComponent;
// This class does not need to be modified.
UINTERFACE(NotBlueprintable, MinimalAPI)
class UDestructible : public UInterface {
    GENERATED_BODY()
};

/**
 * This interface should be implemented by any actor that can be damaged
 */
class MYTHIC_API IDestructible {
    GENERATED_BODY()

public:
    // The rewards to give to the player when they kill this actor.
    virtual FRewardsToGive GetOnKillRewards(AActor *Killer = nullptr) = 0;

    // CalculateHitsLeft: Returns hits till destruction. If returns 0, the actor is destroyed, and should call "Destruct()".
    UFUNCTION(BlueprintCallable)
    virtual int CalculateHitsLeft(int32 InstanceIndex, AActor *DamageCauser, float Damage = 1) = 0;

    UFUNCTION(BlueprintCallable)
    virtual FGameplayTag GetDestructibleType() = 0;
};
