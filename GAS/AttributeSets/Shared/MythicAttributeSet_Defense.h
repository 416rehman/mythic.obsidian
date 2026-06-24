// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"

#include "MythicAttributeSet_Defense.generated.h"
/**
 * Defensive attributes
 * These attributes should be used to resist status effects and reduce incoming damage in the DamageApplicationEffect, after the damage has been calculated in the DamageCalculationEffect. 
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_Defense : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    // Reduces incoming damage from same level or lower enemies - Increased by leveling up (i.e level 1 = 1, level 2 = 2, etc.)
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Armor)
    FGameplayAttributeData Armor;
    // Chance to dodge incoming attack - cancels the attack
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_DodgeChance)
    FGameplayAttributeData DodgeChance;
    // Reduces chance to be burnt - Health reduced over time
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_BurnResistance)
    FGameplayAttributeData BurnResistance;
    // Reduces chance to bleed - Health reduced over time
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_BleedResistance)
    FGameplayAttributeData BleedResistance;
    // Reduces chance to be poisoned - Health reduced over time
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_PoisonResistance)
    FGameplayAttributeData PoisonResistance;
    // Reduces chance to be slowed - Movement speed reduced
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_SlowResistance)
    FGameplayAttributeData SlowResistance;
    // Reduces chance to be frozen - Cannot move or attack
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_FreezeResistance)
    FGameplayAttributeData FreezeResistance;
    // Reduces chance to be stunned - Cannot move or attack
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_StunResistance)
    FGameplayAttributeData StunResistance;
    
    // Tracks current buildup towards Burn
    UPROPERTY(BlueprintReadOnly, Category = "Buildup", ReplicatedUsing = OnRep_BurnBuildup)
    FGameplayAttributeData BurnBuildup;
    
    // Tracks current buildup towards Bleed
    UPROPERTY(BlueprintReadOnly, Category = "Buildup", ReplicatedUsing = OnRep_BleedBuildup)
    FGameplayAttributeData BleedBuildup;
    
    // Tracks current buildup towards Poison
    UPROPERTY(BlueprintReadOnly, Category = "Buildup", ReplicatedUsing = OnRep_PoisonBuildup)
    FGameplayAttributeData PoisonBuildup;
    
    // Tracks current buildup towards Slow
    UPROPERTY(BlueprintReadOnly, Category = "Buildup", ReplicatedUsing = OnRep_SlowBuildup)
    FGameplayAttributeData SlowBuildup;
    
    // Tracks current buildup towards Freeze
    UPROPERTY(BlueprintReadOnly, Category = "Buildup", ReplicatedUsing = OnRep_FreezeBuildup)
    FGameplayAttributeData FreezeBuildup;
    
    // Tracks current buildup towards Stun
    UPROPERTY(BlueprintReadOnly, Category = "Buildup", ReplicatedUsing = OnRep_StunBuildup)
    FGameplayAttributeData StunBuildup;

    // Reduces incoming damage from enemies under status effects
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_DecreasedDamageFromEnemiesUnderStatusEffects)
    FGameplayAttributeData DecreasedDamageFromEnemiesUnderStatusEffects;

    // Health Regen Rate is the rate at which life.health regenerates per second.
    // Life is its own AttributeSet due to its common use. 
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_HealthRegenRate)
    FGameplayAttributeData HealthRegenRate;

    // Max Shield is the maximum amount of shield a player can have. Shield is a secondary health bar that absorbs damage before health is affected.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxShield)
    FGameplayAttributeData MaxShield;

    // Shield is a secondary health bar that absorbs damage before health is affected.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Shield)
    FGameplayAttributeData Shield;

    // Shield Regen Rate is the rate at which shield regenerates per second.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ShieldRegenRate)
    FGameplayAttributeData ShieldRegenRate;

    // LifePerHit is the amount of life that is restored when dealing damage.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_LifePerHit)
    FGameplayAttributeData LifePerHit;

    // LifePerKill is the amount of life that is restored when killing an enemy.
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_LifePerKill)
    FGameplayAttributeData LifePerKill;

    // incoming damage multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_IncomingDamageMultiplier)
    FGameplayAttributeData IncomingDamageMultiplier;

public:
    UMythicAttributeSet_Defense();

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, Armor);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, DodgeChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, BurnResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, BleedResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, PoisonResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, SlowResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, FreezeResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, StunResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, BurnBuildup);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, BleedBuildup);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, PoisonBuildup);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, SlowBuildup);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, FreezeBuildup);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, StunBuildup);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, HealthRegenRate);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, MaxShield);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, Shield);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, ShieldRegenRate);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, LifePerHit);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, LifePerKill);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, IncomingDamageMultiplier);

    // replication
    UFUNCTION()
    virtual void OnRep_Armor(const FGameplayAttributeData &OldArmor);
    UFUNCTION()
    virtual void OnRep_DodgeChance(const FGameplayAttributeData &OldDodgeChance);
    UFUNCTION()
    virtual void OnRep_BurnResistance(const FGameplayAttributeData &OldBurnResistance);
    UFUNCTION()
    virtual void OnRep_BleedResistance(const FGameplayAttributeData &OldBleedResistance);
    UFUNCTION()
    virtual void OnRep_PoisonResistance(const FGameplayAttributeData &OldPoisonResistance);
    UFUNCTION()
    virtual void OnRep_SlowResistance(const FGameplayAttributeData &OldSlowResistance);
    UFUNCTION()
    virtual void OnRep_FreezeResistance(const FGameplayAttributeData &OldFreezeResistance);
    UFUNCTION()
    virtual void OnRep_StunResistance(const FGameplayAttributeData &OldStunResistance);
    UFUNCTION()
    virtual void OnRep_BurnBuildup(const FGameplayAttributeData &OldBurnBuildup);
    UFUNCTION()
    virtual void OnRep_BleedBuildup(const FGameplayAttributeData &OldBleedBuildup);
    UFUNCTION()
    virtual void OnRep_PoisonBuildup(const FGameplayAttributeData &OldPoisonBuildup);
    UFUNCTION()
    virtual void OnRep_SlowBuildup(const FGameplayAttributeData &OldSlowBuildup);
    UFUNCTION()
    virtual void OnRep_FreezeBuildup(const FGameplayAttributeData &OldFreezeBuildup);
    UFUNCTION()
    virtual void OnRep_StunBuildup(const FGameplayAttributeData &OldStunBuildup);
    UFUNCTION()
    virtual void OnRep_DecreasedDamageFromEnemiesUnderStatusEffects(const FGameplayAttributeData &OldDecreasedDamageFromEnemiesUnderStatusEffects);
    UFUNCTION()
    virtual void OnRep_HealthRegenRate(const FGameplayAttributeData &OldHealthRegenRate);
    UFUNCTION()
    virtual void OnRep_MaxShield(const FGameplayAttributeData &OldMaxShield);
    UFUNCTION()
    virtual void OnRep_Shield(const FGameplayAttributeData &OldShield);
    UFUNCTION()
    virtual void OnRep_ShieldRegenRate(const FGameplayAttributeData &OldShieldRegenRate);
    UFUNCTION()
    virtual void OnRep_LifePerHit(const FGameplayAttributeData &OldLifePerHit);
    UFUNCTION()
    virtual void OnRep_LifePerKill(const FGameplayAttributeData &OldLifePerKill);
    UFUNCTION()
    virtual void OnRep_IncomingDamageMultiplier(const FGameplayAttributeData &OldIncomingDamageMultiplier);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // clamp shield to range, and armor, dodge chance, and resistances to non-negative
    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    // reclamp shield when max shield drops below current shield
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

private:
    // shield value before changes cached in PreAttributeChange to compute exact damage absorbed
    float ShieldBeforeChange = 0.0f;
};
