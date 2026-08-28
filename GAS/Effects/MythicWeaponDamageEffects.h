#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MythicDamageContainer.h"
#include "MythicWeaponDamageEffects.generated.h"

/**
 * Sealed source-side stage of the canonical weapon-hit pipeline.
 *
 * The effect is native and cannot be subclassed or authored as a Blueprint, so its execution can never drift from
 * UMythicDamageCalculation while item and combat data continue to supply the live GAS attribute values.
 */
UCLASS(NotBlueprintable)
class MYTHIC_API UMythicGE_WeaponDamageCalculation final : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_WeaponDamageCalculation();
};

/**
 * Sealed target-side stage of the canonical weapon-hit pipeline.
 *
 * Each resolved living target receives this instant effect. It owns the exact UMythicDamageApplication execution
 * and the canonical hit Gameplay Cue, keeping damage application and player feedback on one native contract.
 */
UCLASS(NotBlueprintable)
class MYTHIC_API UMythicGE_WeaponDamageApplication final : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicGE_WeaponDamageApplication();
};

/** Native accessors for systems that consume the canonical weapon-damage pair. */
namespace MythicWeaponDamage {
MYTHIC_API TSubclassOf<UGameplayEffect> GetCalculationEffectClass();
MYTHIC_API TSubclassOf<UGameplayEffect> GetApplicationEffectClass();
MYTHIC_API FMythicDamageContainer MakeDamageContainer();
}
