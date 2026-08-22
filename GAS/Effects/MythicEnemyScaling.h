
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
    int32 ItemLevelBonus = 0;
};

/** One authored row of the enemy tier ladder. Lives in project settings so a designer owns the numbers. */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicEnemyTierScaling {
    GENERATED_BODY()

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (Categories = "AI.Tier"))
    FGameplayTag Tier;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
    float HealthMultiplier = 1.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
    float DamageMultiplier = 1.0f;

    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0.01"))
    float XpMultiplier = 1.0f;

    /** Added to the world's base item level for anything this tier drops. A tougher kill drops better gear. */
    UPROPERTY(config, EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0"))
    int32 ItemLevelBonus = 0;
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

    /** The item level a kill of this tier drops at, given the world's base. Never below 1: an item level of
     *  zero fails every affix tier gate and every socket cap, so a level-0 drop is silently empty. */
    static int32 ComputeDropItemLevel(float WorldItemLevelBase, const FGameplayTag &TierTag);
};

UCLASS()
class MYTHIC_API UMythicGE_CombatScaling : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_CombatScaling();

    virtual void PostInitProperties() override;
};
