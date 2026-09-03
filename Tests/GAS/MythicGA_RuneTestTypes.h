#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "GameplayEffect.h"
#include "ScalableFloat.h"
#include "GAS/Abilities/MythicAbilityCost_Stamina.h"
#include "GAS/Abilities/MythicGA_Rune.h"
#include "GAS/Abilities/MythicGA_Skill.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Player/MythicCharacter.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "MythicGA_RuneTestTypes.generated.h"

namespace MythicRuneTestEffects {
inline void AddSetByCallerModifier(UGameplayEffect *Effect, const FGameplayAttribute &Attribute) {
    FGameplayModifierInfo Mod;
    Mod.Attribute = Attribute;
    Mod.ModifierOp = EGameplayModOp::Additive;
    FSetByCallerFloat SetByCaller;
    SetByCaller.DataTag = GAS_SETBYCALLER_GENERIC;
    Mod.ModifierMagnitude = FGameplayEffectModifierMagnitude(SetByCaller);
    Effect->Modifiers.Add(Mod);
}

inline void SetSetByCallerDuration(UGameplayEffect *Effect) {
    Effect->DurationPolicy = EGameplayEffectDurationType::HasDuration;
    FSetByCallerFloat Duration;
    Duration.DataTag = GAS_SETBYCALLER_DURATION;
    Effect->DurationMagnitude = FGameplayEffectModifierMagnitude(Duration);
}
}

/** A rune with no graph, so the base class's lifecycle, cooldown and tags can be measured on their own. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestAbility : public UMythicGA_Rune {
    GENERATED_BODY()
};

/** Stands in for a cheat-death rune's graph: on the lethal blow it keeps the owner alive at a quarter health. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestCheatDeathAbility : public UMythicGA_Rune {
    GENERATED_BODY()

public:
    int32 PreDeathCount = 0;
    bool bLastPreventResult = false;

protected:
    virtual void NotifyRunePreDeath(const FGameplayEventData &Payload) override {
        PreDeathCount++;
        bLastPreventResult = PreventDeath(0.25f);
        Super::NotifyRunePreDeath(Payload);
    }
};

/**
 * Stands in for a loot rune's graph: it records whose credit is rolling, then adds two drops, floors the rarity and
 * stops the credit paying at all. It edits every credit it hears, so a test can tell the event apart from the
 * ownership question. Test-only.
 */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestLootAbility : public UMythicGA_Rune {
    GENERATED_BODY()

public:
    int32 PreLootRollCount = 0;
    bool bLastForOwner = false;
    TWeakObjectPtr<APlayerController> LastCreditedTo;

protected:
    virtual void NotifyRunePreLootRoll(APlayerController *CreditedTo) override {
        PreLootRollCount++;
        LastCreditedTo = CreditedTo;
        bLastForOwner = IsRuneLootForOwner();
        AddRuneLootDrops(2);
        SetRuneLootMinRarity(3);
        SetRuneLootDropScale(0.0f);
        Super::NotifyRunePreLootRoll(CreditedTo);
    }
};

/** Records every native seam the base raises, so a test can say which fired and with what. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestSeamAbility : public UMythicGA_Rune {
    GENERATED_BODY()

public:
    int32 KillCount = 0;
    int32 LandedCount = 0;
    float LastImpactSpeed = 0.0f;
    float LastFallDamage = 0.0f;
    bool bLastPrevented = false;
    int32 FallBeganCount = 0;
    int32 StillBeganCount = 0;
    TArray<float> FallDepths;
    int32 DashStartedCount = 0;
    int32 DashEndedCount = 0;
    TWeakObjectPtr<UMythicGA_Skill> LastDashSkill;
    FVector LastDashStart = FVector::ZeroVector;
    FVector LastDashEnd = FVector::ZeroVector;
    int32 GuardEndedCount = 0;

protected:
    virtual void NotifyRuneKill(const FGameplayEventData &Payload) override {
        KillCount++;
        Super::NotifyRuneKill(Payload);
    }

    virtual void NotifyRuneLanded(float ImpactSpeed, float FallDamage, bool bPrevented) override {
        LandedCount++;
        LastImpactSpeed = ImpactSpeed;
        LastFallDamage = FallDamage;
        bLastPrevented = bPrevented;
        Super::NotifyRuneLanded(ImpactSpeed, FallDamage, bPrevented);
    }

    virtual void NotifyRuneFallBegan() override {
        FallBeganCount++;
        Super::NotifyRuneFallBegan();
    }

    virtual void NotifyRuneFallDepth(float Metres) override {
        FallDepths.Add(Metres);
        Super::NotifyRuneFallDepth(Metres);
    }

    virtual void NotifyRuneStillBegan() override {
        StillBeganCount++;
        Super::NotifyRuneStillBegan();
    }

    virtual void NotifyRuneSkillDashStarted(UMythicGA_Skill *Skill, const FVector &StartLocation) override {
        DashStartedCount++;
        LastDashSkill = Skill;
        LastDashStart = StartLocation;
        Super::NotifyRuneSkillDashStarted(Skill, StartLocation);
    }

    virtual void NotifyRuneSkillDashEnded(UMythicGA_Skill *Skill, const FVector &EndLocation) override {
        DashEndedCount++;
        LastDashSkill = Skill;
        LastDashEnd = EndLocation;
        Super::NotifyRuneSkillDashEnded(Skill, EndLocation);
    }

    virtual void NotifyRuneGuardEnded() override {
        GuardEndedCount++;
        Super::NotifyRuneGuardEnded();
    }
};

/** The shape of GE_FallDamage and GE_Rune_SelfDamage: Instant, Life.Damage += SetByCaller.Generic. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestDamageMetaEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicRuneTestDamageMetaEffect() {
        DurationPolicy = EGameplayEffectDurationType::Instant;
        MythicRuneTestEffects::AddSetByCallerModifier(this, UMythicAttributeSet_Life::GetDamageAttribute());
    }
};

/** The shape of GE_InstantHeal: Instant, Life.Healing += SetByCaller.Generic. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestHealMetaEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicRuneTestHealMetaEffect() {
        DurationPolicy = EGameplayEffectDurationType::Instant;
        MythicRuneTestEffects::AddSetByCallerModifier(this, UMythicAttributeSet_Life::GetHealingAttribute());
    }
};

/** The shape of GE_Rune_Guard_MaxShield: SetByCaller.Duration long, MaxShield += SetByCaller.Generic. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestGuardMaxShieldEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicRuneTestGuardMaxShieldEffect() {
        MythicRuneTestEffects::SetSetByCallerDuration(this);
        MythicRuneTestEffects::AddSetByCallerModifier(this, UMythicAttributeSet_Defense::GetMaxShieldAttribute());
    }
};

/** The shape of GE_Rune_Guard_Shield: SetByCaller.Duration long, Shield += SetByCaller.Generic. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestGuardShieldEffect : public UGameplayEffect {
    GENERATED_BODY()

public:
    UMythicRuneTestGuardShieldEffect() {
        MythicRuneTestEffects::SetSetByCallerDuration(this);
        MythicRuneTestEffects::AddSetByCallerModifier(this, UMythicAttributeSet_Defense::GetShieldAttribute());
    }
};

/** A skill that moves the caster and costs the default stamina, the way GA_Skill_Evade does. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestDashSkill : public UMythicGA_Skill {
    GENERATED_BODY()

public:
    UMythicRuneTestDashSkill() {
        Movement = EMythicSkillMovement::Dash;
        MovementDistance = 300.0f;
        AdditionalCosts.Add(CreateDefaultSubobject<UMythicAbilityCost_Stamina>(TEXT("StaminaCost")));
    }
};

/** A skill that stays put, so the dash seams can be shown to ignore it. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestStandSkill : public UMythicGA_Skill {
    GENERATED_BODY()

public:
    UMythicRuneTestStandSkill() { Movement = EMythicSkillMovement::None; }
};

/** Something a deferred rune delegate can land on. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class UMythicRuneTestDeferredTarget : public UObject {
    GENERATED_BODY()

public:
    int32 Fires = 0;

    UFUNCTION()
    void Fire() { Fires++; }
};

/**
 * A character that owns its ability system, so a landing and a lethal blow run against one actor with no player
 * state or controller behind it. Fall damage is switched on with the authored defaults. Test-only.
 */
UCLASS(NotBlueprintable, Hidden)
class AMythicRuneTestCharacter : public AMythicCharacter {
    GENERATED_BODY()

public:
    AMythicRuneTestCharacter() {
        AbilitySystem = CreateDefaultSubobject<UMythicAbilitySystemComponent>(TEXT("AbilitySystem"));
        LifeAttributes = CreateDefaultSubobject<UMythicAttributeSet_Life>(TEXT("LifeAttributes"));
        DefenseAttributes = CreateDefaultSubobject<UMythicAttributeSet_Defense>(TEXT("DefenseAttributes"));
        UtilityAttributes = CreateDefaultSubobject<UMythicAttributeSet_Utility>(TEXT("UtilityAttributes"));
        Life = CreateDefaultSubobject<UMythicLifeComponent>(TEXT("Life"));
        Ledger = CreateDefaultSubobject<UMythicStatLedgerComponent>(TEXT("Ledger"));
        // PostInitializeComponents fills these in a live world; the standalone test world never runs it.
        CachedASC = AbilitySystem;
        CachedLife = Life;
        bEnableFallDamage = true;
        FallDamageEffect = UMythicRuneTestDamageMetaEffect::StaticClass();
    }

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override { return AbilitySystem; }

    float GetSafeFallSpeed() const { return SafeFallSpeed; }

    UPROPERTY()
    TObjectPtr<UMythicAbilitySystemComponent> AbilitySystem;

    UPROPERTY()
    TObjectPtr<UMythicAttributeSet_Life> LifeAttributes;

    UPROPERTY()
    TObjectPtr<UMythicAttributeSet_Defense> DefenseAttributes;

    UPROPERTY()
    TObjectPtr<UMythicAttributeSet_Utility> UtilityAttributes;

    UPROPERTY()
    TObjectPtr<UMythicLifeComponent> Life;

    UPROPERTY()
    TObjectPtr<UMythicStatLedgerComponent> Ledger;
};
