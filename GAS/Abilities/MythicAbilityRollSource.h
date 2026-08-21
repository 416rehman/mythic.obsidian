
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"
#include "UObject/Interface.h"
#include "MythicAbilityRollSource.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UMythicAbilityRollSource : public UInterface {
    GENERATED_BODY()
};

/**
 * Implemented by whatever granted a passive ability and holds the values it rolled — a talent fragment today,
 * runes and affixes on the same footing later. It exists so an ability can read its own rolled magnitudes without
 * knowing which system granted it.
 */
class MYTHIC_API IMythicAbilityRollSource {
    GENERATED_BODY()

public:
    /**
     * Rolled value this source stored for one granted ability under one parameter tag. Handle identifies which
     * grant is asking, since one source can grant several abilities with separate rolls.
     */
    virtual bool GetRolledAbilityValue(const FGameplayAbilitySpecHandle &Handle, const FGameplayTag &Parameter, float &OutValue) const = 0;
};
