// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Offense.generated.h"

/**
 * Offensive attributes
 * These attributes should be used to calculate the damage in the DamageCalculationEffect.
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_Offense : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    // Increased by items and leveling up - affects damage dealt
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_Power)
    FGameplayAttributeData Power;

    // Minimum Damage per Hit
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_DamagePerHit)
    FGameplayAttributeData DamagePerHit;
    
    // Attack Speed
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_AttackSpeed)
    FGameplayAttributeData AttackSpeed;

    // Critical hit chance increases the chance of dealing critical hits
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CriticalHitChance)
    FGameplayAttributeData CriticalHitChance;

    // Critical hit damage increases the damage dealt by critical hits
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_CriticalHitDamage)
    FGameplayAttributeData CriticalHitDamage;

    // Increases the chance of applying burn status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyBurnOnHitChance)
    FGameplayAttributeData ApplyBurnOnHitChance;

    // Increases the chance of applying bleed status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyBleedOnHitChance)
    FGameplayAttributeData ApplyBleedOnHitChance;

    // Increases the chance of applying poison status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyPoisonOnHitChance)
    FGameplayAttributeData ApplyPoisonOnHitChance;

    // Increases the chance of applying slow status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplySlowOnHitChance)
    FGameplayAttributeData ApplySlowOnHitChance;

    // Increases the chance of applying freeze status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyFreezeOnHitChance)
    FGameplayAttributeData ApplyFreezeOnHitChance;

    // Increases the chance of applying stun status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyStunOnHitChance)
    FGameplayAttributeData ApplyStunOnHitChance;

    // Increases the chance of applying WEAKEN status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyWeakenOnHitChance)
    FGameplayAttributeData ApplyWeakenOnHitChance;

    // Increases the chance of applying TERRIFY status effect
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ApplyTerrifyOnHitChance)
    FGameplayAttributeData ApplyTerrifyOnHitChance;

    // Increase damage from skills - If a skill was used to deal damage, this attribute increases the damage dealt
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusSkillDamage)
    FGameplayAttributeData BonusSkillDamage;

    // Increase damage from swords - If a sword was used to deal damage, this attribute increases the damage dealt
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusSwordDamage)
    FGameplayAttributeData BonusSwordDamage;

    // Increase damage from axes - If an axe was used to deal damage, this attribute increases the damage dealt
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusAxeDamage)
    FGameplayAttributeData BonusAxeDamage;

    // Increase damage from daggers - If a dagger was used to deal damage, this attribute increases the damage dealt
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusDaggerDamage)
    FGameplayAttributeData BonusDaggerDamage;

    // Increase damage from Sickles - If a sickle was used to deal damage, this attribute increases the damage dealt
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusSickleDamage)
    FGameplayAttributeData BonusSickleDamage;

    // Increase damage from spears
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusSpearDamage)
    FGameplayAttributeData BonusSpearDamage;

    // Increase damage to enemies under status effects
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IncreasedDamageToEnemiesUnderStatusEffects)
    FGameplayAttributeData IncreasedDamageToEnemiesUnderStatusEffects;

    // Increase damage to all superior enemies (Elites, Champions, Bosses)
    // - Minions: Lots of them, low health, low damage - i.e goblins, zombies - default pack size 5-10. Used for player to feel powerful
    // - Elites: Lead the minions, adding complexity and requiring players to prioritize targets - each elite will have a pack of minions. Used to break up flat difficulty curve
    // - Champions: Stronger than elites, used to provide a challenge to the player - each champion will have 1-3 elites (elites will have their own pack of minions). Used for pacing.
    // - Bosses: The strongest enemies in the game, requiring the player to use all their skills to defeat them - Solo 1v1 encounters. I.e a mercenary sent to kill the player
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusDamageToSuperiorEnemies)
    FGameplayAttributeData BonusDamageToSuperiorEnemies;
    
public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, Power);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, DamagePerHit);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, AttackSpeed);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, CriticalHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, CriticalHitDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyBurnOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyBleedOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyPoisonOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplySlowOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyFreezeOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyStunOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyWeakenOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ApplyTerrifyOnHitChance);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSkillDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSwordDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusAxeDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusDaggerDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSickleDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSpearDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, IncreasedDamageToEnemiesUnderStatusEffects);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusDamageToSuperiorEnemies);

    UFUNCTION()
    virtual void OnRep_Power(const FGameplayAttributeData& OldPower);
    UFUNCTION()
    virtual void OnRep_DamagePerHit(const FGameplayAttributeData& OldDamagePerHit);
    UFUNCTION()
    virtual void OnRep_AttackSpeed(const FGameplayAttributeData& OldAttackSpeed);
    UFUNCTION()
    virtual void OnRep_CriticalHitChance(const FGameplayAttributeData& OldCriticalHitChance);
    UFUNCTION()
    virtual void OnRep_CriticalHitDamage(const FGameplayAttributeData& OldCriticalHitDamage);
    UFUNCTION()
    virtual void OnRep_ApplyBurnOnHitChance(const FGameplayAttributeData& OldApplyBurnOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyBleedOnHitChance(const FGameplayAttributeData& OldApplyBleedOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyPoisonOnHitChance(const FGameplayAttributeData& OldApplyPoisonOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplySlowOnHitChance(const FGameplayAttributeData& OldApplySlowOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyFreezeOnHitChance(const FGameplayAttributeData& OldApplyFreezeOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyStunOnHitChance(const FGameplayAttributeData& OldApplyStunOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyWeakenOnHitChance(const FGameplayAttributeData& OldApplyWeakenOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyTerrifyOnHitChance(const FGameplayAttributeData& OldApplyTerrifyOnHitChance);
    UFUNCTION()
    virtual void OnRep_BonusSkillDamage(const FGameplayAttributeData& OldBonusSkillDamage);
    UFUNCTION()
    virtual void OnRep_BonusSwordDamage(const FGameplayAttributeData& OldBonusSwordDamage);
    UFUNCTION()
    virtual void OnRep_BonusAxeDamage(const FGameplayAttributeData& OldBonusAxeDamage);
    UFUNCTION()
    virtual void OnRep_BonusDaggerDamage(const FGameplayAttributeData& OldBonusDaggerDamage);
    UFUNCTION()
    virtual void OnRep_BonusSickleDamage(const FGameplayAttributeData& OldBonusSickleDamage);
    UFUNCTION()
    virtual void OnRep_BonusSpearDamage(const FGameplayAttributeData& OldBonusSpearDamage);
    UFUNCTION()
    virtual void OnRep_IncreasedDamageToEnemiesUnderStatusEffects(const FGameplayAttributeData& OldIncreasedDamageToEnemiesUnderStatusEffects);
    UFUNCTION()
    virtual void OnRep_BonusDamageToSuperiorEnemies(const FGameplayAttributeData& OldBonusDamageToSuperiorEnemies);
    
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};
