
#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/Abilities/MythicGA_Passive.h"
#include "GAS/Abilities/MythicGA_Triggered.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/MythicTags_GAS.h"
#include "MythicTestEffects.generated.h"

/**
 * Stands in for the effect asset a talent would point at. One SetByCaller modifier, so a test can prove the
 * ability's rolled magnitude is what reaches the attribute. Test-only: nothing in the game grants this.
 */
UCLASS(NotBlueprintable, Hidden)
class UMythicTestPassiveEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicTestPassiveEffect() {
        DurationPolicy = EGameplayEffectDurationType::Infinite;

        FGameplayModifierInfo Mod;
        Mod.Attribute = UMythicAttributeSet_Offense::GetOutgoingDamageMultiplierAttribute();
        Mod.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat SetByCaller;
        SetByCaller.DataTag = GAS_STATE_HEALTH_CRITICAL;
        Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
        Modifiers.Add(Mod);
    }
};

/**
 * A passive talent whose single clause is a class default, because GiveAbility only accepts a class default object.
 * Test-only: nothing in the game grants this.
 */
UCLASS(NotBlueprintable, Hidden)
class UMythicTestPassiveAbility : public UMythicGA_Passive {
    GENERATED_BODY()

public:
    UMythicTestPassiveAbility() {
        FMythicPassiveClause Clause;
        Clause.EffectToApply = UMythicTestPassiveEffect::StaticClass();
        Clause.MagnitudeParameter = GAS_STATE_HEALTH_CRITICAL;
        Clause.Magnitude = 0.4f;
        Passives.Add(Clause);
    }
};

/**
 * A proc ability whose single clause is a class default. A clause written onto the live CDO never reaches the
 * instance GiveAbility creates, so the only honest fixture is a class that constructs with one. Test-only.
 */
UCLASS(NotBlueprintable, Hidden)
class UMythicTestTriggeredAbility : public UMythicGA_Triggered {
    GENERATED_BODY()

public:
    UMythicTestTriggeredAbility() {
        FMythicTriggerSpec Clause;
        Clause.TriggerEvent = GAS_EVENT_KILL;
        Triggers.Add(Clause);
    }
};
