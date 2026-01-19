#include "MythicAttributeSet_Life.h"
#include "GameplayEffectExtension.h"
#include "GAS/MythicGameplayEffectContext.h"
#include "GAS/MythicTags_GAS.h"
#include "Net/UnrealNetwork.h"

UMythicAttributeSet_Life::UMythicAttributeSet_Life()
    : MaxHealth(100.0f)
      , Health(100.0f)
      , Damage(0.0f)
      , Healing(0.0f)
      , HealthBeforeAttributeChange(0.0f)
      , MaxHealthBeforeAttributeChange(0.0f) {}

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

    const FGameplayEffectContextHandle &EffectContext = Data.EffectSpec.GetEffectContext();
    AActor *Instigator = EffectContext.GetOriginalInstigator();
    AActor *Causer = EffectContext.GetEffectCauser();

    // Handle Damage meta attribute - converts to -Health
    if (Data.EvaluatedData.Attribute == GetDamageAttribute()) {
        // Store the damage magnitude before we clear it
        const float DamageDone = GetDamage();

        // Convert damage to -Health and clamp
        const float NewHealth = FMath::Clamp(GetHealth() - DamageDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);
        SetDamage(0.0f); // Reset meta attribute

        // Broadcast health change with the DAMAGE magnitude (for UI/cues)
        if (DamageDone > 0.0f) {
            OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, -DamageDone, HealthBeforeAttributeChange, GetHealth());
        }

        // Check for death
        if (GetHealth() <= 0.0f && !bOutOfHealth) {
            bOutOfHealth = true;
            // OnOutOfHealth event could be added here
        }
    }
    // Handle Healing meta attribute - converts to +Health
    else if (Data.EvaluatedData.Attribute == GetHealingAttribute()) {
        const float HealingDone = GetHealing();

        // Convert healing to +Health and clamp
        const float NewHealth = FMath::Clamp(GetHealth() + HealingDone, 0.0f, GetMaxHealth());
        SetHealth(NewHealth);
        SetHealing(0.0f); // Reset meta attribute

        if (HealingDone > 0.0f) {
            OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, HealingDone, HealthBeforeAttributeChange, GetHealth());
        }

        // Reset death flag if healed back
        if (bOutOfHealth && GetHealth() > 0.0f) {
            bOutOfHealth = false;
        }
    }
    // Handle direct Health changes
    else if (Data.EvaluatedData.Attribute == GetHealthAttribute()) {
        // Clamp health
        SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
        OnHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, HealthBeforeAttributeChange, GetHealth());

        if (GetHealth() <= 0.0f && !bOutOfHealth) {
            bOutOfHealth = true;
        }
        else if (bOutOfHealth && GetHealth() > 0.0f) {
            bOutOfHealth = false;
        }
    }
    // Handle MaxHealth changes
    else if (Data.EvaluatedData.Attribute == GetMaxHealthAttribute()) {
        // Clamp current health to new max
        if (GetHealth() > GetMaxHealth()) {
            SetHealth(GetMaxHealth());
        }
        OnMaxHealthChanged.Broadcast(Instigator, Causer, &Data.EffectSpec, Data.EvaluatedData.Magnitude, MaxHealthBeforeAttributeChange, GetMaxHealth());
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
