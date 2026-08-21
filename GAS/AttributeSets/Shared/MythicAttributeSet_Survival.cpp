#include "MythicAttributeSet_Survival.h"

#include "GameplayEffectExtension.h"
#include "Net/UnrealNetwork.h"

UMythicAttributeSet_Survival::UMythicAttributeSet_Survival() {
    InitMaxNourishment(100.0f);
    InitNourishment(100.0f);
    InitMaxHydration(100.0f);
    InitHydration(100.0f);
    InitMaxWarmth(100.0f);
    InitWarmth(50.0f);
    InitMaxWetness(100.0f);
    InitWetness(0.0f);
}

void UMythicAttributeSet_Survival::ClampAttribute(const FGameplayAttribute &Attribute, float &NewValue) const {
    if (Attribute == GetNourishmentAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxNourishment());
    }
    else if (Attribute == GetHydrationAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHydration());
    }
    else if (Attribute == GetWarmthAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxWarmth());
    }
    else if (Attribute == GetWetnessAttribute()) {
        NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxWetness());
    }
    else if (Attribute == GetMaxNourishmentAttribute() || Attribute == GetMaxHydrationAttribute() ||
             Attribute == GetMaxWarmthAttribute() || Attribute == GetMaxWetnessAttribute()) {
        NewValue = FMath::Max(0.0f, NewValue);
    }
}

void UMythicAttributeSet_Survival::PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) {
    Super::PreAttributeChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UMythicAttributeSet_Survival::PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const {
    Super::PreAttributeBaseChange(Attribute, NewValue);
    ClampAttribute(Attribute, NewValue);
}

void UMythicAttributeSet_Survival::PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) {
    Super::PostGameplayEffectExecute(Data);

    if (Data.EvaluatedData.Attribute == GetMaxNourishmentAttribute() && GetNourishment() > GetMaxNourishment()) {
        SetNourishment(GetMaxNourishment());
    }
    else if (Data.EvaluatedData.Attribute == GetMaxHydrationAttribute() && GetHydration() > GetMaxHydration()) {
        SetHydration(GetMaxHydration());
    }
    else if (Data.EvaluatedData.Attribute == GetMaxWarmthAttribute() && GetWarmth() > GetMaxWarmth()) {
        SetWarmth(GetMaxWarmth());
    }
    else if (Data.EvaluatedData.Attribute == GetMaxWetnessAttribute() && GetWetness() > GetMaxWetness()) {
        SetWetness(GetMaxWetness());
    }
}

void UMythicAttributeSet_Survival::OnRep_Nourishment(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, Nourishment, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_MaxNourishment(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, MaxNourishment, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_Hydration(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, Hydration, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_MaxHydration(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, MaxHydration, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_Warmth(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, Warmth, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_MaxWarmth(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, MaxWarmth, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_Wetness(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, Wetness, OldValue);
}
void UMythicAttributeSet_Survival::OnRep_MaxWetness(const FGameplayAttributeData &OldValue) {
    GAMEPLAYATTRIBUTE_REPNOTIFY(UMythicAttributeSet_Survival, MaxWetness, OldValue);
}

void UMythicAttributeSet_Survival::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, Nourishment, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, MaxNourishment, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, Hydration, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, MaxHydration, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, Warmth, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, MaxWarmth, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, Wetness, COND_OwnerOnly, REPNOTIFY_Always);
    DOREPLIFETIME_CONDITION_NOTIFY(UMythicAttributeSet_Survival, MaxWetness, COND_OwnerOnly, REPNOTIFY_Always);
}
