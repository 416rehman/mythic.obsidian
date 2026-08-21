// Copyright Stellar Games. All Rights Reserved.

#include "MythicPlayerStatusViewModel.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Utility.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Defense.h"
#include "GAS/MythicTags_GAS.h"

void UMythicPlayerStatusViewModel::InitializeForASC(UAbilitySystemComponent *InASC) {
    if (!InASC) {
        return;
    }
    Unbind();
    ASC = InASC;

    CurHealth = InASC->GetNumericAttribute(UMythicAttributeSet_Life::GetHealthAttribute());
    MaxHealthV = InASC->GetNumericAttribute(UMythicAttributeSet_Life::GetMaxHealthAttribute());
    CurStamina = InASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute());
    MaxStaminaV = InASC->GetNumericAttribute(UMythicAttributeSet_Utility::GetMaxStaminaAttribute());
    CurShield = InASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetShieldAttribute());
    MaxShieldV = InASC->GetNumericAttribute(UMythicAttributeSet_Defense::GetMaxShieldAttribute());
    SetHealthPercent(MaxHealthV > 0.0f ? FMath::Clamp(CurHealth / MaxHealthV, 0.0f, 1.0f) : 0.0f);
    SetStaminaPercent(MaxStaminaV > 0.0f ? FMath::Clamp(CurStamina / MaxStaminaV, 0.0f, 1.0f) : 0.0f);
    SetShieldPercent(MaxShieldV > 0.0f ? FMath::Clamp(CurShield / MaxShieldV, 0.0f, 1.0f) : 0.0f);

    SetInCombat(InASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT));
    SetExhausted(InASC->HasMatchingGameplayTag(GAS_STATE_EXHAUSTED));
    SetBurning(InASC->HasMatchingGameplayTag(GAS_DEBUFF_BURNING));
    SetBleeding(InASC->HasMatchingGameplayTag(GAS_DEBUFF_BLEEDING));
    SetPoisoned(InASC->HasMatchingGameplayTag(GAS_DEBUFF_POISONED));
    SetStunned(InASC->HasMatchingGameplayTag(GAS_DEBUFF_STUNNED));
    SetSlowed(InASC->HasMatchingGameplayTag(GAS_DEBUFF_SLOWED));
    SetFrozen(InASC->HasMatchingGameplayTag(GAS_DEBUFF_FROZEN));

    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetMaxHealthAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetMaxStaminaAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetShieldAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);
    InASC->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetMaxShieldAttribute()).AddUObject(this, &UMythicPlayerStatusViewModel::HandleAttributeChanged);

    const FGameplayTag StatusTags[] = {GAS_STATE_INCOMBAT, GAS_STATE_EXHAUSTED, GAS_DEBUFF_BURNING, GAS_DEBUFF_BLEEDING, GAS_DEBUFF_POISONED, GAS_DEBUFF_STUNNED, GAS_DEBUFF_SLOWED, GAS_DEBUFF_FROZEN};
    for (const FGameplayTag &Tag : StatusTags) {
        if (Tag.IsValid()) {
            InASC->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).AddUObject(this, &UMythicPlayerStatusViewModel::HandleTagChanged);
        }
    }
}

void UMythicPlayerStatusViewModel::HandleAttributeChanged(const FOnAttributeChangeData &Data) {
    const FGameplayAttribute &A = Data.Attribute;
    if (A == UMythicAttributeSet_Life::GetHealthAttribute()) {
        const float Old = CurHealth;
        CurHealth = Data.NewValue;
        SetHealthPercent(MaxHealthV > 0.0f ? FMath::Clamp(CurHealth / MaxHealthV, 0.0f, 1.0f) : 0.0f);
        if (Data.NewValue < Old) {
            OnHealthDamaged.Broadcast((Old - Data.NewValue) / FMath::Max(1.0f, MaxHealthV), HealthPercent);
        }
    }
    else if (A == UMythicAttributeSet_Life::GetMaxHealthAttribute()) {
        MaxHealthV = Data.NewValue;
        SetHealthPercent(MaxHealthV > 0.0f ? FMath::Clamp(CurHealth / MaxHealthV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()) {
        CurStamina = Data.NewValue;
        SetStaminaPercent(MaxStaminaV > 0.0f ? FMath::Clamp(CurStamina / MaxStaminaV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Utility::GetMaxStaminaAttribute()) {
        MaxStaminaV = Data.NewValue;
        SetStaminaPercent(MaxStaminaV > 0.0f ? FMath::Clamp(CurStamina / MaxStaminaV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Defense::GetShieldAttribute()) {
        CurShield = Data.NewValue;
        SetShieldPercent(MaxShieldV > 0.0f ? FMath::Clamp(CurShield / MaxShieldV, 0.0f, 1.0f) : 0.0f);
    }
    else if (A == UMythicAttributeSet_Defense::GetMaxShieldAttribute()) {
        MaxShieldV = Data.NewValue;
        SetShieldPercent(MaxShieldV > 0.0f ? FMath::Clamp(CurShield / MaxShieldV, 0.0f, 1.0f) : 0.0f);
    }
}

void UMythicPlayerStatusViewModel::HandleTagChanged(const FGameplayTag Tag, int32 NewCount) {
    const bool bOn = NewCount > 0;
    if (Tag == GAS_STATE_INCOMBAT) {
        SetInCombat(bOn);
    }
    else if (Tag == GAS_STATE_EXHAUSTED) {
        SetExhausted(bOn);
    }
    else if (Tag == GAS_DEBUFF_BURNING) {
        SetBurning(bOn);
    }
    else if (Tag == GAS_DEBUFF_BLEEDING) {
        SetBleeding(bOn);
    }
    else if (Tag == GAS_DEBUFF_POISONED) {
        SetPoisoned(bOn);
    }
    else if (Tag == GAS_DEBUFF_STUNNED) {
        SetStunned(bOn);
    }
    else if (Tag == GAS_DEBUFF_SLOWED) {
        SetSlowed(bOn);
    }
    else if (Tag == GAS_DEBUFF_FROZEN) {
        SetFrozen(bOn);
    }
}

void UMythicPlayerStatusViewModel::Unbind() {
    UAbilitySystemComponent *A = ASC.Get();
    if (!A) {
        return;
    }
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetHealthAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Life::GetMaxHealthAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetCurrentStaminaAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Utility::GetMaxStaminaAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetShieldAttribute()).RemoveAll(this);
    A->GetGameplayAttributeValueChangeDelegate(UMythicAttributeSet_Defense::GetMaxShieldAttribute()).RemoveAll(this);

    const FGameplayTag StatusTags[] = {GAS_STATE_INCOMBAT, GAS_STATE_EXHAUSTED, GAS_DEBUFF_BURNING, GAS_DEBUFF_BLEEDING, GAS_DEBUFF_POISONED, GAS_DEBUFF_STUNNED, GAS_DEBUFF_SLOWED, GAS_DEBUFF_FROZEN};
    for (const FGameplayTag &Tag : StatusTags) {
        if (Tag.IsValid()) {
            A->RegisterGameplayTagEvent(Tag, EGameplayTagEventType::NewOrRemoved).RemoveAll(this);
        }
    }
    ASC = nullptr;
}

void UMythicPlayerStatusViewModel::BeginDestroy() {
    Unbind();
    Super::BeginDestroy();
}

void UMythicPlayerStatusViewModel::SetHealthPercent(float V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(HealthPercent, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(HealthPercent);
    }
}
void UMythicPlayerStatusViewModel::SetStaminaPercent(float V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(StaminaPercent, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(StaminaPercent);
    }
}
void UMythicPlayerStatusViewModel::SetShieldPercent(float V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(ShieldPercent, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(ShieldPercent);
    }
}
void UMythicPlayerStatusViewModel::SetInCombat(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bInCombat, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bInCombat);
    }
}
void UMythicPlayerStatusViewModel::SetExhausted(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bExhausted, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bExhausted);
    }
}
void UMythicPlayerStatusViewModel::SetBurning(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bBurning, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bBurning);
    }
}
void UMythicPlayerStatusViewModel::SetBleeding(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bBleeding, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bBleeding);
    }
}
void UMythicPlayerStatusViewModel::SetPoisoned(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bPoisoned, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bPoisoned);
    }
}
void UMythicPlayerStatusViewModel::SetStunned(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bStunned, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bStunned);
    }
}
void UMythicPlayerStatusViewModel::SetSlowed(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bSlowed, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bSlowed);
    }
}
void UMythicPlayerStatusViewModel::SetFrozen(bool V) {
    if (UE_MVVM_SET_PROPERTY_VALUE(bFrozen, V)) {
        UE_MVVM_BROADCAST_FIELD_VALUE_CHANGED(bFrozen);
    }
}
