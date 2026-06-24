// 


#include "MythicAttributeSet_Utility.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UMythicAttributeSet_Utility::UMythicAttributeSet_Utility() {
    // baseline so stamina is usable before any data-driven init applies (otherwise max=0 -> regen/spend no-op)
    InitMaxStamina(100.0f);
    InitCurrentStamina(100.0f);
    InitStaminaRegenRate(10.0f);

    // default dynamic cooldown reduction cap to 60 percent
    InitMaxCooldownReduction(0.60f);
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
    else if (Attribute == GetCooldownReductionAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, FMath::Max(0.0f, GetMaxCooldownReduction()));
    }
    else if (Attribute == GetMaxCooldownReductionAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
    else if (Attribute == GetStaminaCostReductionAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
    }
}

void UMythicAttributeSet_Utility::PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const {
    Super::PreAttributeBaseChange(Attribute, NewValue);

    if (Attribute == GetCooldownReductionAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, FMath::Max(0.0f, GetMaxCooldownReduction()));
    }
    else if (Attribute == GetMaxCooldownReductionAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
}

void UMythicAttributeSet_Utility::PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) {
    Super::PostAttributeChange(Attribute, OldValue, NewValue);

    // apply saturation curve for diminishing returns resolve to max stamina mapping
    if (Attribute == GetResolveAttribute()) {
        if (!bIsUpdatingMaxStamina) {
            bIsUpdatingMaxStamina = true;
            const float BaseMaxStaminaValue = 100.0f;
            float ClampedResolve = FMath::Max(0.0f, NewValue);
            float ResolveScalar = ClampedResolve / (ClampedResolve + 40.0f);
            float MaxStaminaBonus = 150.0f * ResolveScalar;
            SetMaxStamina(BaseMaxStaminaValue + MaxStaminaBonus);
            bIsUpdatingMaxStamina = false;
        }
    }

    // if max cooldown reduction drops below current cdr, pull current cdr down to the new limit
    if (Attribute == GetMaxCooldownReductionAttribute()) {
        if (GetCooldownReduction() > NewValue) {
            SetCooldownReduction(NewValue);
        }
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

void UMythicAttributeSet_Utility::OnRep_StaminaRegenRate(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, StaminaRegenRate, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_StaminaCostReduction(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, StaminaCostReduction, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_CooldownReduction(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, CooldownReduction, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_MaxCooldownReduction(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, MaxCooldownReduction, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_ProficiencyXPBonus(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, ProficiencyXPBonus, OldValue);
}

void UMythicAttributeSet_Utility::OnRep_BonusSprintSpeed(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Utility, BonusSprintSpeed, OldValue);
}

void UMythicAttributeSet_Utility::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    // register the utility attributes for network replication
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, Resolve, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, MaxStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, CurrentStamina, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, StaminaRegenRate, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, StaminaCostReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, CooldownReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, MaxCooldownReduction, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, ProficiencyXPBonus, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Utility, BonusSprintSpeed, COND_None, REPNOTIFY_Always);
}
