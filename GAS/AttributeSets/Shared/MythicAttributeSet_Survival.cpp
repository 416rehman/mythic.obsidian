#include "MythicAttributeSet_Survival.h"

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

TConstArrayView<FMythicBoundedAttributePair>
UMythicAttributeSet_Survival::GetBoundedAttributePairs() const {
    static const FMythicBoundedAttributePair Pairs[] = {
        {GetNourishmentAttribute(), GetMaxNourishmentAttribute(), 0.0f,
         0.0f, EMythicAttributeBaseOverflowPolicy::Discard},
        {GetHydrationAttribute(), GetMaxHydrationAttribute(), 0.0f, 0.0f,
         EMythicAttributeBaseOverflowPolicy::Discard},
        {GetWarmthAttribute(), GetMaxWarmthAttribute(), 0.0f, 0.0f,
         EMythicAttributeBaseOverflowPolicy::Discard},
        {GetWetnessAttribute(), GetMaxWetnessAttribute(), 0.0f, 0.0f,
         EMythicAttributeBaseOverflowPolicy::Discard}
    };
    return Pairs;
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
