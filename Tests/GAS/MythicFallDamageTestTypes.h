#pragma once

#include "CoreMinimal.h"
#include "GameplayEffect.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Player/MythicCharacter.h"
#include "MythicFallDamageTestTypes.generated.h"

/** The shape GE_FallDamage has: Instant, Life.Damage += SetByCaller.Generic. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicFallDamageTestEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicFallDamageTestEffect() {
        DurationPolicy = EGameplayEffectDurationType::Instant;

        FGameplayModifierInfo Mod;
        Mod.Attribute = UMythicAttributeSet_Life::GetDamageAttribute();
        Mod.ModifierOp = EGameplayModOp::Additive;
        FSetByCallerFloat SetByCaller;
        SetByCaller.DataTag = GAS_SETBYCALLER_GENERIC;
        Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
        Modifiers.Add(Mod);
    }
};

/**
 * A character that owns its ability system, Life and Defense sets, with fall damage on at the authored defaults and
 * the SetByCaller fall effect above. Records what the Blueprint-facing hook was handed. Test-only.
 */
UCLASS(NotBlueprintable, Hidden)
class AMythicFallDamageTestCharacter : public AMythicCharacter {
    GENERATED_BODY()

public:
    AMythicFallDamageTestCharacter() {
        AbilitySystem = CreateDefaultSubobject<UMythicAbilitySystemComponent>(TEXT("AbilitySystem"));
        LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>(TEXT("LifeAttributes"));
        DefenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Defense>(TEXT("DefenseAttributes"));
        Life = CreateDefaultSubobject<UMythicLifeComponent>(TEXT("Life"));
        // PostInitializeComponents fills these in a live world; the standalone test world never runs it.
        CachedASC = AbilitySystem;
        CachedLife = Life;
        bEnableFallDamage = true;
        FallDamageEffect = UMythicFallDamageTestEffect::StaticClass();
    }

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override { return AbilitySystem; }

    float GetSafeFallSpeed() const { return SafeFallSpeed; }
    float GetFallDamagePerSpeed() const { return FallDamagePerSpeed; }
    float GetMaxFallDamage() const { return MaxFallDamage; }

    int32 ComputedHookCalls = 0;
    float LastHookDamage = 0.0f;

    UPROPERTY()
    TObjectPtr<UMythicAbilitySystemComponent> AbilitySystem;

    UPROPERTY()
    TObjectPtr<UMythicAttributeSet_Life> LifeAttributes;

    UPROPERTY()
    TObjectPtr<UMythicAttributeSet_Defense> DefenseAttributes;

    UPROPERTY()
    TObjectPtr<UMythicLifeComponent> Life;

protected:
    virtual float OnFallDamageComputed_Implementation(float ImpactSpeed, float Damage) override {
        ComputedHookCalls++;
        LastHookDamage = Damage;
        return Damage;
    }
};
