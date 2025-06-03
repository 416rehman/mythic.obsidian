// 


#include "MythicAttributeSet_Utility.h"

void UMythicAttributeSet_Utility::OnRep_Resolve(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, Resolve, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_MaxStamina(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, MaxStamina, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_CurrentStamina(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, CurrentStamina, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_StaminaRegenRate(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::OnRep_StaminaCostReduction(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::OnRep_CooldownReduction(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::OnRep_ExperienceBonus(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::OnRep_BonusSprintSpeed(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}
