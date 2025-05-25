#include "MythicAttributeSet_Life.h"
#include "GameplayEffectExtension.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Net/UnrealNetwork.h"

void UMythicAttributeSet_Life::OnRep_MaxHealth(const FGameplayAttributeData &OldMaxHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Life, MaxHealth, OldMaxHealth);
}

void UMythicAttributeSet_Life::OnRep_Health(const FGameplayAttributeData &OldHealth) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Life, Health, OldHealth);
}

void UMythicAttributeSet_Life::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Life, MaxHealth, COND_None, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Life, Health, COND_None, REPNOTIFY_Always);
}

void UMythicAttributeSet_Life::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttributes(Attribute, NewValue);
}

bool UMythicAttributeSet_Life::PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data) {
    if (!Super::PreGameplayEffectExecute(Data)) {
        return false;
    }

    // Save the current health
    HealthBeforeAttributeChange = GetHealth();
    MaxHealthBeforeAttributeChange = GetMaxHealth();

    return true;
}

void UMythicAttributeSet_Life::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);
    ClampAttributes(Data.EvaluatedData.Attribute, Data.EvaluatedData.Magnitude);
    
    const FGameplayEffectContextHandle &EffectContext = Data.EffectSpec.GetEffectContext();
    AActor *Instigator = EffectContext.GetOriginalInstigator();
    AActor *Causer = EffectContext.GetEffectCauser();

    if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute()) {
        // Notify on any requested max health changes
        OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeAttributeChange, GetMaxHealth());
    }
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
        OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());
    }
}

void UMythicAttributeSet_Life::ClampAttributes(const FGameplayAttribute &Attribute, float &NewValue) {
    if (Attribute == GetHealthAttribute()) {
        // Do not allow health to go negative or above max health.
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
    }
    else if (Attribute == GetMaxHealthAttribute()) {
        // Do not allow max health to drop below 1.
        NewValue = FMath::Max(NewValue, 1.0f);
    }
}
