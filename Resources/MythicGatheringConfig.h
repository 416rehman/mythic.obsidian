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
};
