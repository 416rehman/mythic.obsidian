// 


#include "MythicAttributeSet_Utility.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UMythicAttributeSet_Utility::UMythicAttributeSet_Utility() {
    // Baseline so stamina is usable before any data-driven init applies (otherwise Max=0 -> regen/spend no-op).
    InitMaxStamina(100.0f);
    InitCurrentStamina(100.0f);
    InitStaminaRegenRate(10.0f);
}

bool UMythicAttributeSet_Utility::IsReductionFractionAttribute(const FGameplayAttribute &Attribute) {
    return Attribute == GetStaminaCostReductionAttribute() || Attribute == GetCooldownReductionAttribute();
}

void UMythicAttributeSet_Utility::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);

    if (Attribute == GetCurrentStaminaAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxStamina());
    }
    else if (Attribute == GetMaxStaminaAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
    else if (IsReductionFractionAttribute(Attribute)) {
        // StaminaCostReduction + CooldownReduction: reduction fractions, clamped [0,1] so a buff can't push past 100%
        // (negative would INVERT into a penalty — e.g. CooldownReduction < 0 lengthens cooldowns) and raw readers
        // (UI/tooltips) never see out-of-range. CooldownReduction's consumer (ApplyCooldown) also clamps to [0,MaxCDR];
        // this enforces the same invariant StaminaCostReduction already had, at the source.
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
}

void UMythicAttributeSet_Utility::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);

    // When a GE lowers MaxStamina, PreAttributeChange only re-clamps the attribute actually written (MaxStamina), never
    // CurrentStamina — so CurrentStamina would sit ABOVE the new max (the bar overfills, the player over-spends; regen
    // only ADDS when Cur<Max, so it never pulls it down). Re-clamp here, mirroring the Life/Defense attribute sets.
    if (Data.EvaluatedData.Attribute == GetMaxStaminaAttribute()) {
        if (GetCurrentStamina() > GetMaxStamina()) {
            SetCurrentStamina(GetMaxStamina());
        }
    }
}

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

void UMythicAttributeSet_Utility::OnRep_ProficiencyXPBonus(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::OnRep_BonusSprintSpeed(const FGameplayAttributeData &OldValue) {}

void UMythicAttributeSet_Utility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // These were declared ReplicatedUsing but never registered, so nothing replicated (stale client HUD).
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, Resolve, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, MaxStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, CurrentStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, StaminaRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, StaminaCostReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, CooldownReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, ProficiencyXPBonus, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, BonusSprintSpeed, COND_None, REPNOTIFY_Always);
}
