

#include "MythicAttributeSet_Offense.h"

#include "Net/UnrealNetwork.h"

UMythicAttributeSet_Offense::UMythicAttributeSet_Offense() {
    InitOutgoingDamageMultiplier(1.0f);
    InitStatusBuildupMultiplier(1.0f);
    InitAilmentDamageMultiplier(1.0f);
    InitAilmentDurationMultiplier(1.0f);
}

bool UMythicAttributeSet_Offense::IsProbabilityAttribute(const FGameplayAttribute &Attribute) {
    return Attribute == GetCriticalHitChanceAttribute()
        || Attribute == GetApplyBurnOnHitChanceAttribute()
        || Attribute == GetApplyBleedOnHitChanceAttribute()
        || Attribute == GetApplyPoisonOnHitChanceAttribute()
        || Attribute == GetApplySlowOnHitChanceAttribute()
        || Attribute == GetApplyFreezeOnHitChanceAttribute()
        || Attribute == GetApplyStunOnHitChanceAttribute()
        || Attribute == GetApplyWeakenOnHitChanceAttribute()
        || Attribute == GetApplyTerrifyOnHitChanceAttribute();
}

void UMythicAttributeSet_Offense::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (IsProbabilityAttribute(Attribute)) {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    // A negative multiplier would drain buildup on hit, which reads as curing the ailment by attacking.
    else if (Attribute == GetStatusBuildupMultiplierAttribute()
             || Attribute == GetAilmentDamageMultiplierAttribute()
             || Attribute == GetAilmentDurationMultiplierAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
}

void UMythicAttributeSet_Offense::PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const {
    Super::PreAttributeBaseChange(Attribute, NewValue);

    if (IsProbabilityAttribute(Attribute)) {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
    // A negative multiplier would drain buildup on hit, which reads as curing the ailment by attacking.
    else if (Attribute == GetStatusBuildupMultiplierAttribute()
             || Attribute == GetAilmentDamageMultiplierAttribute()
             || Attribute == GetAilmentDurationMultiplierAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
}

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

void UMythicAttributeSet_Offense::OnRep_BonusHammerDamage(const FGameplayAttributeData &OldBonusHammerDamage) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusHammerDamage, OldBonusHammerDamage);
}

void UMythicAttributeSet_Offense::
OnRep_IncreasedDamageToEnemiesUnderStatusEffects(const FGameplayAttributeData &OldIncreasedDamageToEnemiesUnderStatusEffects) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, IncreasedDamageToEnemiesUnderStatusEffects,
                                OldIncreasedDamageToEnemiesUnderStatusEffects);
}

void UMythicAttributeSet_Offense::OnRep_BonusDamageToSuperiorEnemies(const FGameplayAttributeData &OldBonusDamageToSuperiorEnemies) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, BonusDamageToSuperiorEnemies, OldBonusDamageToSuperiorEnemies);
}

void UMythicAttributeSet_Offense::OnRep_OutgoingDamageMultiplier(const FGameplayAttributeData &OldOutgoingDamageMultiplier) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, OutgoingDamageMultiplier, OldOutgoingDamageMultiplier);
}

void UMythicAttributeSet_Offense::OnRep_StatusBuildupMultiplier(const FGameplayAttributeData &OldStatusBuildupMultiplier) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, StatusBuildupMultiplier, OldStatusBuildupMultiplier);
}

void UMythicAttributeSet_Offense::OnRep_AilmentDamageMultiplier(const FGameplayAttributeData &OldAilmentDamageMultiplier) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, AilmentDamageMultiplier, OldAilmentDamageMultiplier);
}

void UMythicAttributeSet_Offense::OnRep_AilmentDurationMultiplier(const FGameplayAttributeData &OldAilmentDurationMultiplier) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Offense, AilmentDurationMultiplier, OldAilmentDurationMultiplier);
}

void UMythicAttributeSet_Offense::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, Power, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, DamagePerHit, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, AttackSpeed, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, CriticalHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, CriticalHitDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyBurnOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyBleedOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyPoisonOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplySlowOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyFreezeOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyStunOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyWeakenOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, ApplyTerrifyOnHitChance, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSkillDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSwordDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusAxeDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusDaggerDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSickleDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusSpearDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusHammerDamage, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, IncreasedDamageToEnemiesUnderStatusEffects, COND_OwnerOnly,
                                   REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, BonusDamageToSuperiorEnemies, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, OutgoingDamageMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, StatusBuildupMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, AilmentDamageMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Offense, AilmentDurationMultiplier, COND_OwnerOnly, REPNOTIFY_Always);
}
