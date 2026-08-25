
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Offense.generated.h"

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

    // Strengthens applied control statuses (slow, weaken, terrify) - the applier stat their rolled magnitude scales by
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_ControlPotency)
    FGameplayAttributeData ControlPotency;

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

    // Increase damage from hammers
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusHammerDamage)
    FGameplayAttributeData BonusHammerDamage;

    // Increase damage to enemies under status effects
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_IncreasedDamageToEnemiesUnderStatusEffects)
    FGameplayAttributeData IncreasedDamageToEnemiesUnderStatusEffects;

    // increase damage to all superior enemies (Elites, Champions, Bosses)
    // - Minions: Lots of them, low health, low damage - i.e goblins, zombies - default pack size 5-10. Used for player to feel powerful
    // - Elites: Lead the minions, adding complexity and requiring players to prioritize targets - each elite will have a pack of minions. Used to break up flat difficulty curve
    // - Champions: Stronger than elites, used to provide a challenge to the player - each champion will have 1-3 elites (elites will have their own pack of minions). Used for pacing.
    // - Bosses: The strongest enemies in the game, requiring the player to use all their skills to defeat them - Solo 1v1 encounters. I.e a mercenary sent to kill the player
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BonusDamageToSuperiorEnemies)
    FGameplayAttributeData BonusDamageToSuperiorEnemies;

    // outgoing damage multiplier
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_OutgoingDamageMultiplier)
    FGameplayAttributeData OutgoingDamageMultiplier;

    // Scales how much status buildup each landed proc applies. 1.0 is normal; 2.0 reaches the threshold in half
    // the hits. This is what a status build stacks to make its status land sooner.
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_StatusBuildupMultiplier)
    FGameplayAttributeData StatusBuildupMultiplier;

    /**
     * Per-status scaling of what a status does once it lands, as opposed to how fast it lands. 1.0 is the authored
     * band untouched; gear adds fractions on top, so 1.35 reads as +35% on the sheet.
     *
     * These are per status rather than one global stat because a poison build and a fire build should not be the
     * same build. Only the three damage-over-time statuses carry a damage band, so only they get a damage stat -
     * a FreezeDamageMultiplier would be a stat with nothing to multiply.
     */
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BurnBonusDamage)
    FGameplayAttributeData BurnBonusDamage;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BleedBonusDamage)
    FGameplayAttributeData BleedBonusDamage;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_PoisonBonusDamage)
    FGameplayAttributeData PoisonBonusDamage;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BurnDurationMultiplier)
    FGameplayAttributeData BurnDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_BleedDurationMultiplier)
    FGameplayAttributeData BleedDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_PoisonDurationMultiplier)
    FGameplayAttributeData PoisonDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_SlowDurationMultiplier)
    FGameplayAttributeData SlowDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_FreezeDurationMultiplier)
    FGameplayAttributeData FreezeDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_StunDurationMultiplier)
    FGameplayAttributeData StunDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_WeakenDurationMultiplier)
    FGameplayAttributeData WeakenDurationMultiplier;

    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_TerrifyDurationMultiplier)
    FGameplayAttributeData TerrifyDurationMultiplier;

    /**
     * What a skill does, moved by gear instead of by re-authoring the skill. Each adds to the number the skill
     * authored: radius 400 with SkillRadiusBonus 50 queries 450.
     *
     * Generic rather than per-skill on purpose. Per-skill quantifiers are the eventual design, but sixteen
     * skills times four quantifiers is sixty-four attributes for a set with no content in it yet. A skill that
     * earns its own stat gets one added beside these.
     *
     * There is deliberately no skill damage stat here. BonusSkillDamage above already scales every hit tagged
     * GAS.Ability.Type.Skill inside the damage execution, so a second one would double-dip.
     */

    // Centimetres added to a skill's authored shape radius.
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_SkillRadiusBonus)
    FGameplayAttributeData SkillRadiusBonus;

    // Extra targets a skill's shape query may take beyond its authored MaxTargets.
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_SkillTargetCountBonus)
    FGameplayAttributeData SkillTargetCountBonus;

    // Seconds added to a skill's authored duration.
    UPROPERTY(Category = "Offense", EditAnywhere, BlueprintReadWrite, ReplicatedUsing = OnRep_SkillDurationBonus)
    FGameplayAttributeData SkillDurationBonus;


public:
    UMythicAttributeSet_Offense();

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
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, ControlPotency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSkillDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSwordDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusAxeDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusDaggerDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSickleDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusSpearDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusHammerDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, IncreasedDamageToEnemiesUnderStatusEffects);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BonusDamageToSuperiorEnemies);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, OutgoingDamageMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, StatusBuildupMultiplier);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BurnBonusDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BleedBonusDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, PoisonBonusDamage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BurnDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, BleedDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, PoisonDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, SlowDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, FreezeDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, StunDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, WeakenDurationMultiplier);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, TerrifyDurationMultiplier);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, SkillRadiusBonus);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, SkillTargetCountBonus);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Offense, SkillDurationBonus);

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    virtual void PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const override;

    static bool IsProbabilityAttribute(const FGameplayAttribute &Attribute);

    // True for the per-status scaling stats, which share one clamp: never negative, or gear would cure on hit.
    static bool IsStatusScalingAttribute(const FGameplayAttribute &Attribute);

    UFUNCTION()
    virtual void OnRep_Power(const FGameplayAttributeData &OldPower);
    UFUNCTION()
    virtual void OnRep_DamagePerHit(const FGameplayAttributeData &OldDamagePerHit);
    UFUNCTION()
    virtual void OnRep_AttackSpeed(const FGameplayAttributeData &OldAttackSpeed);
    UFUNCTION()
    virtual void OnRep_CriticalHitChance(const FGameplayAttributeData &OldCriticalHitChance);
    UFUNCTION()
    virtual void OnRep_CriticalHitDamage(const FGameplayAttributeData &OldCriticalHitDamage);
    UFUNCTION()
    virtual void OnRep_ApplyBurnOnHitChance(const FGameplayAttributeData &OldApplyBurnOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyBleedOnHitChance(const FGameplayAttributeData &OldApplyBleedOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyPoisonOnHitChance(const FGameplayAttributeData &OldApplyPoisonOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplySlowOnHitChance(const FGameplayAttributeData &OldApplySlowOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyFreezeOnHitChance(const FGameplayAttributeData &OldApplyFreezeOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyStunOnHitChance(const FGameplayAttributeData &OldApplyStunOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyWeakenOnHitChance(const FGameplayAttributeData &OldApplyWeakenOnHitChance);
    UFUNCTION()
    virtual void OnRep_ApplyTerrifyOnHitChance(const FGameplayAttributeData &OldApplyTerrifyOnHitChance);
    UFUNCTION()
    virtual void OnRep_ControlPotency(const FGameplayAttributeData &OldControlPotency);
    UFUNCTION()
    virtual void OnRep_BonusSkillDamage(const FGameplayAttributeData &OldBonusSkillDamage);
    UFUNCTION()
    virtual void OnRep_BonusSwordDamage(const FGameplayAttributeData &OldBonusSwordDamage);
    UFUNCTION()
    virtual void OnRep_BonusAxeDamage(const FGameplayAttributeData &OldBonusAxeDamage);
    UFUNCTION()
    virtual void OnRep_BonusDaggerDamage(const FGameplayAttributeData &OldBonusDaggerDamage);
    UFUNCTION()
    virtual void OnRep_BonusSickleDamage(const FGameplayAttributeData &OldBonusSickleDamage);
    UFUNCTION()
    virtual void OnRep_BonusSpearDamage(const FGameplayAttributeData &OldBonusSpearDamage);
    UFUNCTION()
    virtual void OnRep_BonusHammerDamage(const FGameplayAttributeData &OldBonusHammerDamage);
    UFUNCTION()
    virtual void OnRep_IncreasedDamageToEnemiesUnderStatusEffects(const FGameplayAttributeData &OldIncreasedDamageToEnemiesUnderStatusEffects);
    UFUNCTION()
    virtual void OnRep_BonusDamageToSuperiorEnemies(const FGameplayAttributeData &OldBonusDamageToSuperiorEnemies);
    UFUNCTION()
    virtual void OnRep_OutgoingDamageMultiplier(const FGameplayAttributeData &OldOutgoingDamageMultiplier);

    UFUNCTION()
    virtual void OnRep_StatusBuildupMultiplier(const FGameplayAttributeData &OldStatusBuildupMultiplier);

    UFUNCTION()
    virtual void OnRep_BurnBonusDamage(const FGameplayAttributeData &OldBurnBonusDamage);

    UFUNCTION()
    virtual void OnRep_BleedBonusDamage(const FGameplayAttributeData &OldBleedBonusDamage);

    UFUNCTION()
    virtual void OnRep_PoisonBonusDamage(const FGameplayAttributeData &OldPoisonBonusDamage);

    UFUNCTION()
    virtual void OnRep_BurnDurationMultiplier(const FGameplayAttributeData &OldBurnDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_BleedDurationMultiplier(const FGameplayAttributeData &OldBleedDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_PoisonDurationMultiplier(const FGameplayAttributeData &OldPoisonDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_SlowDurationMultiplier(const FGameplayAttributeData &OldSlowDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_FreezeDurationMultiplier(const FGameplayAttributeData &OldFreezeDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_StunDurationMultiplier(const FGameplayAttributeData &OldStunDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_WeakenDurationMultiplier(const FGameplayAttributeData &OldWeakenDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_TerrifyDurationMultiplier(const FGameplayAttributeData &OldTerrifyDurationMultiplier);

    UFUNCTION()
    virtual void OnRep_SkillRadiusBonus(const FGameplayAttributeData &OldSkillRadiusBonus);

    UFUNCTION()
    virtual void OnRep_SkillTargetCountBonus(const FGameplayAttributeData &OldSkillTargetCountBonus);

    UFUNCTION()
    virtual void OnRep_SkillDurationBonus(const FGameplayAttributeData &OldSkillDurationBonus);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
