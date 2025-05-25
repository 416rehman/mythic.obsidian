// 


#include "MythicAttributeSet_Offense.h"

#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Offense::OnRep_Power(const FGameplayAttributeData &OldPower) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, Power, OldPower);
}

void UMythicAttributeSet_Offense::OnRep_DamagePerHit(const FGameplayAttributeData &OldDamagePerHit) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, DamagePerHit, OldDamagePerHit);
}

void UMythicAttributeSet_Offense::OnRep_AttackSpeed(const FGameplayAttributeData &OldAttackSpeed) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, AttackSpeed, OldAttackSpeed);
}

void UMythicAttributeSet_Offense::OnRep_CriticalHitChance(const FGameplayAttributeData &OldCriticalHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, CriticalHitChance, OldCriticalHitChance);
}

void UMythicAttributeSet_Offense::OnRep_CriticalHitDamage(const FGameplayAttributeData &OldCriticalHitDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, CriticalHitDamage, OldCriticalHitDamage);
}

void UMythicAttributeSet_Offense::OnRep_ApplyBurnOnHitChance(const FGameplayAttributeData &OldApplyBurnOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyBurnOnHitChance, OldApplyBurnOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplyBleedOnHitChance(const FGameplayAttributeData &OldApplyBleedOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyBleedOnHitChance, OldApplyBleedOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplyPoisonOnHitChance(const FGameplayAttributeData &OldApplyPoisonOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyPoisonOnHitChance, OldApplyPoisonOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplySlowOnHitChance(const FGameplayAttributeData &OldApplySlowOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplySlowOnHitChance, OldApplySlowOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplyFreezeOnHitChance(const FGameplayAttributeData &OldApplyFreezeOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyFreezeOnHitChance, OldApplyFreezeOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplyStunOnHitChance(const FGameplayAttributeData &OldApplyStunOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyStunOnHitChance, OldApplyStunOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplyWeakenOnHitChance(const FGameplayAttributeData &OldApplyWeakenOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyWeakenOnHitChance, OldApplyWeakenOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_ApplyTerrifyOnHitChance(const FGameplayAttributeData &OldApplyTerrifyOnHitChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, ApplyTerrifyOnHitChance, OldApplyTerrifyOnHitChance);
}

void UMythicAttributeSet_Offense::OnRep_BonusSkillDamage(const FGameplayAttributeData &OldBonusSkillDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusSkillDamage, OldBonusSkillDamage);
}

void UMythicAttributeSet_Offense::OnRep_BonusSwordDamage(const FGameplayAttributeData &OldBonusSwordDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusSwordDamage, OldBonusSwordDamage);
}

void UMythicAttributeSet_Offense::OnRep_BonusAxeDamage(const FGameplayAttributeData &OldBonusAxeDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusAxeDamage, OldBonusAxeDamage);
}

void UMythicAttributeSet_Offense::OnRep_BonusDaggerDamage(const FGameplayAttributeData &OldBonusDaggerDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusDaggerDamage, OldBonusDaggerDamage);
}

void UMythicAttributeSet_Offense::OnRep_BonusSickleDamage(const FGameplayAttributeData &OldBonusSickleDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusSickleDamage, OldBonusSickleDamage);
}

void UMythicAttributeSet_Offense::OnRep_BonusSpearDamage(const FGameplayAttributeData &OldBonusSpearDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusSpearDamage, OldBonusSpearDamage);
}

void UMythicAttributeSet_Offense::
OnRep_IncreasedDamageToEnemiesUnderStatusEffects(const FGameplayAttributeData &OldIncreasedDamageToEnemiesUnderStatusEffects) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, IncreasedDamageToEnemiesUnderStatusEffects,
                                OldIncreasedDamageToEnemiesUnderStatusEffects);
}

void UMythicAttributeSet_Offense::OnRep_BonusDamageToSuperiorEnemies(const FGameplayAttributeData &OldBonusDamageToSuperiorEnemies) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusDamageToSuperiorEnemies, OldBonusDamageToSuperiorEnemies);
}

/// ----------------
/// Replication
/// ----------------
void UMythicAttributeSet_Offense::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, Power, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, DamagePerHit, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, AttackSpeed, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, CriticalHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, CriticalHitDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyBurnOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyBleedOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyPoisonOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplySlowOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyFreezeOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyStunOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyWeakenOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyTerrifyOnHitChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSkillDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSwordDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusAxeDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusDaggerDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSickleDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSpearDamage, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, IncreasedDamageToEnemiesUnderStatusEffects, COND_None,
                                   REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusDamageToSuperiorEnemies, COND_None, REPNOTIFY_Always);
}
