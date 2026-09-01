// Copyright Stellar Games. All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "GameplayTagContainer.h"

class UAbilitySystemComponent;
class UAttackFragment;

/**
 * Canonical target-independent projection of one ordinary basic-weapon hit.
 *
 * Base is the composed DamagePerHit roll band. PrimaryAdjusted adds the authored primary-stat contribution (Power
 * today). Effective additionally applies the exact equipped weapon-class bonus. Critical hits, skills, temporary
 * combat states, target defenses, and environmental modifiers are deliberately excluded because they are not a
 * stable property of the character's ordinary white hit.
 */
struct MYTHIC_API FMythicWeaponDamageProjection {
    float BaseMinimumDamage = 0.0f;
    float BaseMaximumDamage = 0.0f;
    float BaseAverageDamage = 0.0f;

    float PrimaryAdjustedMinimumDamage = 0.0f;
    float PrimaryAdjustedMaximumDamage = 0.0f;
    float PrimaryAdjustedAverageDamage = 0.0f;

    float EffectiveMinimumDamage = 0.0f;
    float EffectiveMaximumDamage = 0.0f;
    float EffectiveAverageDamage = 0.0f;

    float PrimaryStatBonusFraction = 0.0f;
    float WeaponClassBonusMultiplier = 1.0f;
    FGameplayTag WeaponClassTag;
};

namespace MythicCombat {
/**
 * Resolves a basic-weapon damage projection from captured/live source values and the attack's authoritative type
 * tags. Invalid values fail atomically and clear OutProjection.
 */
MYTHIC_API bool ResolveWeaponDamageProjection(
    float DamagePerHit,
    const FGameplayTagContainer &AttackSourceTags,
    TFunctionRef<float(const FGameplayAttribute &)> ReadSourceStat,
    FMythicWeaponDamageProjection &OutProjection);

/**
 * Resolves and validates the exact weapon-class leaf owned by one live attack fragment. A weapon with no supported
 * class, or with several class leaves, is corrupt and fails closed instead of silently choosing a bonus.
 */
MYTHIC_API bool ResolveWeaponTypeTags(
    const UAttackFragment *AttackFragment,
    FGameplayTagContainer &OutWeaponTypeTags);

/**
 * Resolves the single active canonical weapon source owned by an ASC and returns its live item-type tags. Missing or
 * ambiguous weapon grants fail closed instead of guessing which class bonus should be displayed.
 */
MYTHIC_API bool ResolveActiveWeaponTypeTags(
    const UAbilitySystemComponent *AbilitySystem,
    FGameplayTagContainer &OutWeaponTypeTags);

/** Builds the complete character-effective white-hit projection from one live ASC and its active weapon source. */
MYTHIC_API bool BuildWeaponDamageProjection(
    const UAbilitySystemComponent *AbilitySystem,
    FMythicWeaponDamageProjection &OutProjection);
}
