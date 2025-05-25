// 

#include "MythicAttributeSet_Defense.h"

#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Defense::OnRep_Armor(const FGameplayAttributeData &OldArmor) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, Armor, OldArmor);
}

void UMythicAttributeSet_Defense::OnRep_DodgeChance(const FGameplayAttributeData &OldDodgeChance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, DodgeChance, OldDodgeChance);
}

void UMythicAttributeSet_Defense::OnRep_BurnResistance(const FGameplayAttributeData &OldBurnResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, BurnResistance, OldBurnResistance);
}

void UMythicAttributeSet_Defense::OnRep_BleedResistance(const FGameplayAttributeData &OldBleedResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, BleedResistance, OldBleedResistance);
}

void UMythicAttributeSet_Defense::OnRep_PoisonResistance(const FGameplayAttributeData &OldPoisonResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, PoisonResistance, OldPoisonResistance);
}

void UMythicAttributeSet_Defense::OnRep_SlowResistance(const FGameplayAttributeData &OldSlowResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, SlowResistance, OldSlowResistance);
}

void UMythicAttributeSet_Defense::OnRep_FreezeResistance(const FGameplayAttributeData &OldFreezeResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, FreezeResistance, OldFreezeResistance);

}

void UMythicAttributeSet_Defense::OnRep_StunResistance(const FGameplayAttributeData &OldStunResistance) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, StunResistance, OldStunResistance);
}

void UMythicAttributeSet_Defense::OnRep_DecreasedDamageFromEnemiesUnderStatusEffects(
    const FGameplayAttributeData &OldDecreasedDamageFromEnemiesUnderStatusEffects) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects,
                                OldDecreasedDamageFromEnemiesUnderStatusEffects);
}

void UMythicAttributeSet_Defense::OnRep_HealthRegenRate(const FGameplayAttributeData &OldHealthRegenRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, HealthRegenRate, OldHealthRegenRate);
}

void UMythicAttributeSet_Defense::OnRep_MaxShield(const FGameplayAttributeData &OldMaxShield) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, MaxShield, OldMaxShield);
}

void UMythicAttributeSet_Defense::OnRep_Shield(const FGameplayAttributeData &OldShield) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, Shield, OldShield);
}

void UMythicAttributeSet_Defense::OnRep_ShieldRegenRate(const FGameplayAttributeData &OldShieldRegenRate) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, ShieldRegenRate, OldShieldRegenRate);
}

void UMythicAttributeSet_Defense::OnRep_LifePerHit(const FGameplayAttributeData &OldLifePerHit) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, LifePerHit, OldLifePerHit);
}

void UMythicAttributeSet_Defense::OnRep_LifePerKill(const FGameplayAttributeData &OldLifePerKill) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Defense, LifePerKill, OldLifePerKill);
}

void UMythicAttributeSet_Defense::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Armor, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DodgeChance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BurnResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, BleedResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, PoisonResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, SlowResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, FreezeResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, StunResistance, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, DecreasedDamageFromEnemiesUnderStatusEffects, COND_None,
                                   REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, HealthRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, MaxShield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, Shield, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, ShieldRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerHit, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Defense, LifePerKill, COND_None, REPNOTIFY_Always);
}
