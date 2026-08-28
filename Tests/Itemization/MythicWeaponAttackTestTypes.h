#pragma once

#include "CoreMinimal.h"
#include "Destructible.h"
#include "GameFramework/Actor.h"
#include "MythicWeaponAttackTestTypes.generated.h"

/** Hidden actor-backed destructible used to verify canonical attack target identity. */
UCLASS(NotBlueprintable, Hidden)
class MYTHIC_API AMythicWeaponAttackActorDestructibleTestFixture final
    : public AActor,
      public IDestructible {
    GENERATED_BODY()

public:
    virtual FRewardsToGive GetOnKillRewards(AActor * = nullptr) override {
        return {};
    }
};
