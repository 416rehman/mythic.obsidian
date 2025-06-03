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

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, Armor);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, DodgeChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, BurnResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, BleedResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, PoisonResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, SlowResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, FreezeResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, StunResistance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, HealthRegenRate);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, MaxShield);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, Shield);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, ShieldRegenRate);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, LifePerHit);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Defense, LifePerKill);

    // Replication
    UFUNCTION()
    virtual void OnRep_Armor(const FGameplayAttributeData& OldArmor);
    UFUNCTION()
    virtual void OnRep_DodgeChance(const FGameplayAttributeData& OldDodgeChance);
    UFUNCTION()
    virtual void OnRep_BurnResistance(const FGameplayAttributeData& OldBurnResistance);
    UFUNCTION()
    virtual void OnRep_BleedResistance(const FGameplayAttributeData& OldBleedResistance);
    UFUNCTION()
    virtual void OnRep_PoisonResistance(const FGameplayAttributeData& OldPoisonResistance);
    UFUNCTION()
    virtual void OnRep_SlowResistance(const FGameplayAttributeData& OldSlowResistance);
    UFUNCTION()
    virtual void OnRep_FreezeResistance(const FGameplayAttributeData& OldFreezeResistance);
    UFUNCTION()
    virtual void OnRep_StunResistance(const FGameplayAttributeData& OldStunResistance);
    UFUNCTION()
    virtual void OnRep_DecreasedDamageFromEnemiesUnderStatusEffects(const FGameplayAttributeData& OldDecreasedDamageFromEnemiesUnderStatusEffects);
    UFUNCTION()
    virtual void OnRep_HealthRegenRate(const FGameplayAttributeData& OldHealthRegenRate);
    UFUNCTION()
    virtual void OnRep_MaxShield(const FGameplayAttributeData& OldMaxShield);
    UFUNCTION()
    virtual void OnRep_Shield(const FGameplayAttributeData& OldShield);
    UFUNCTION()
    virtual void OnRep_ShieldRegenRate(const FGameplayAttributeData& OldShieldRegenRate);
    UFUNCTION()
    virtual void OnRep_LifePerHit(const FGameplayAttributeData& OldLifePerHit);
    UFUNCTION()
    virtual void OnRep_LifePerKill(const FGameplayAttributeData& OldLifePerKill);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
