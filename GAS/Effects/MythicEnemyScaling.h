
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GameplayTagContainer.h"
#include "NativeGameplayTags.h"
#include "MythicEnemyScaling.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_SCALING_HEALTH);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_SETBYCALLER_SCALING_DAMAGE);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(GAS_STATE_COMBATSCALING);

struct FMythicTierScaling {
    float HealthMult = 1.0f;
    float DamageMult = 1.0f;
    float XpMult = 1.0f;
};

struct MYTHIC_API FMythicEnemyScaling {
    /**
      * Party size and world tier are independent axes, so they compose multiplicatively. World tier arrives as the
      * already-evaluated EnemyHealth/EnemyDamage multiplier from UWorldTierAttributes rather than a per-NPC linear
      * term, so a gameplay effect can push it and one curve decides the whole tier ladder.
      */
    static FVector2D ComputeStatMultiplier(int32 PartySize, float PerExtraMemberHealth, float PerExtraMemberDamage,
                                           float WorldHealthMultiplier, float WorldDamageMultiplier);

    static FMythicTierScaling GetTierScaling(const FGameplayTag &TierTag);
};

UCLASS()
class MYTHIC_API UMythicGE_CombatScaling : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_CombatScaling();

    virtual void PostInitProperties() override;
};
