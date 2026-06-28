#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MythicGatheringConfig.generated.h"

class UProficiencyDefinition;

USTRUCT(BlueprintType)
struct FGatheringProficiencyConfig {
    GENERATED_BODY()

    // bonus damage per proficiency level (additive multiplier)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float BonusDamagePerLevel = 0.02f;

    // chance per proficiency level to double yield on node destruction
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    float BonusYieldChancePerLevel = 0.01f;

    // maps resource type tags to proficiency definitions
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
    TMap<FGameplayTag, TObjectPtr<UProficiencyDefinition>> ResourceToProficiency;

    // Proficiency XP granted to the gatherer (in the resource's mapped proficiency) per harvested node. 0 = no XP
    // (conservative default — gathering trains a skill only once a designer opts in). Without this, the gathering
    // proficiencies that drive BonusDamagePerLevel + BonusYieldChancePerLevel can never climb from gathering.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0"))
    float XpPerHarvest = 0.0f;

    // Anti-grind: once the gatherer's level in the mapped proficiency is at or above this, a harvest grants no more XP
    // (mirrors the crafting anti-grind cap). 0 = no cap (always grants while XpPerHarvest > 0).
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0"))
    int32 XpNoGainAtOrAboveLevel = 0;
};
