// 


#include "MythicLifeComponent.h"

#include "Mythic.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"


UMythicLifeComponent::UMythicLifeComponent(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.bCanEverTick = false;

    SetIsReplicatedByDefault(true);

    AbilitySystemComponent = nullptr;
    LifeSet = nullptr;
}

void UMythicLifeComponent::OnUnregister() {
    UninitializeFromAbilitySystem();

    Super::OnUnregister();
}

void UMythicLifeComponent::InitializeWithAbilitySystem(UAbilitySystemComponent *InASC) {
    AActor *Owner = GetOwner();
    check(Owner);

    if (AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: Health component for owner [%s] has already been initialized with an ability system."),
               *GetNameSafe(Owner));
        return;
    }

    AbilitySystemComponent = InASC;
    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: Cannot initialize health component for owner [%s] with NULL ability system."), *GetNameSafe(Owner));
        return;
    }

    LifeSet = AbilitySystemComponent->GetSet<UMythicAttributeSet_Life>();
    if (!LifeSet) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: Cannot initialize health component for owner [%s] with NULL health set on the ability system."),
               *GetNameSafe(Owner));
        return;
    }

    // Register to listen for attribute changes.
    LifeSet->OnHealthChanged.AddUObject(this, &ThisClass::HandleHealthChanged);
    LifeSet->OnMaxHealthChanged.AddUObject(this, &ThisClass::HandleMaxHealthChanged);

    // TEMP: Reset attributes to default values.  Eventually this will be driven by a spread sheet.
    auto HealthAttr = UMythicAttributeSet_Life::GetHealthAttribute();
    auto MaxHealthAttr = UMythicAttributeSet_Life::GetMaxHealthAttribute();
    
    AbilitySystemComponent->SetNumericAttributeBase(HealthAttr, LifeSet->GetMaxHealth());

    ClearGameplayTags();
    
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(HealthAttr).AddUObject(this, &ThisClass::TriggerHealthChange);
    AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(MaxHealthAttr).AddUObject(this, &ThisClass::TriggerMaxHealthChange);

    // Initial broadcast of health values.
    auto Health = AbilitySystemComponent->GetNumericAttribute(HealthAttr);
    auto MaxHealth = AbilitySystemComponent->GetNumericAttribute(MaxHealthAttr);
    
    const FGameplayEffectContextHandle EmptyContextHandle = FGameplayEffectContextHandle();
    OnHealthChanged.Broadcast(Health, 0, HealthAttr, EmptyContextHandle);
    OnMaxHealthChanged.Broadcast(MaxHealth, 0, MaxHealthAttr, EmptyContextHandle);
}

void UMythicLifeComponent::UninitializeFromAbilitySystem() {
    ClearGameplayTags();

    if (LifeSet) {
        LifeSet->OnHealthChanged.RemoveAll(this);
        LifeSet->OnMaxHealthChanged.RemoveAll(this);
    }

    LifeSet = nullptr;
    AbilitySystemComponent = nullptr;
}

void UMythicLifeComponent::ClearGameplayTags() const {
    if (AbilitySystemComponent) {
        AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DYING, 0);
        AbilitySystemComponent->SetLooseGameplayTagCount(GAS_STATE_DEAD, 0);
    }
}

float UMythicLifeComponent::GetHealth() const {
    return (LifeSet ? LifeSet->GetHealth() : 0.0f);
}

float UMythicLifeComponent::GetMaxHealth() const {
    return (LifeSet ? LifeSet->GetMaxHealth() : 0.0f);
}

float UMythicLifeComponent::GetHealthNormalized() const {
    if (LifeSet) {
        const float Health = LifeSet->GetHealth();
        const float MaxHealth = LifeSet->GetMaxHealth();

        return ((MaxHealth > 0.0f) ? (Health / MaxHealth) : 0.0f);
    }

    return 0.0f;
}

// Trigger a gameplay event on the source ASC when the health value goes down.
// Target: is the owner of the health component.
// ContextHandle: is the context of the damage effect.
// EventMagnitude: is the amount of damage dealt.
void UMythicLifeComponent::TriggerGameplayEvent_DeliveredHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                             float OldValue, float NewValue) const {
    if (!OnDeliveredHitGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_DeliveredHit: OnDeliveredHitGameplayEventTag is not set."));
        return;
    }

    if (auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent()) {
        if (!AbilitySystemComponent) {
            UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHit: AbilitySystemComponent is NULL."));
            return;
        }

        UDamageResult *DamageResult = NewObject<UDamageResult>();
        DamageResult->OldHealth = OldValue;
        DamageResult->NewHealth = NewValue;

        // Send a gameplay event to the source ASC.
        FGameplayEventData Payload;
        Payload.EventTag = OnDeliveredHitGameplayEventTag;
        Payload.Instigator = DamageInstigator;
        Payload.Target = AbilitySystemComponent->GetAvatarActor();
        Payload.OptionalObject = DamageEffectSpec->Def;
        Payload.OptionalObject2 = DamageResult;
        Payload.ContextHandle = DamageEffectSpec->GetContext();
        Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
        Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = DamageMagnitude;

        FScopedPredictionWindow NewScopedWindow(SourceASC, true);

        // Source ASC
        SourceASC->HandleGameplayEvent(OnDeliveredHitGameplayEventTag, &Payload);
    }
    else {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHit: Source ASC is NULL."));
    }
}

void UMythicLifeComponent::TriggerGameplayEvent_ReceivedHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                            float OldValue, float NewValue) const {
    if (!OnReceivedHitGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_ReceivedHit: OnReceivedHitGameplayEventTag is not set."));
        return;
    }

    if (auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent()) {
        if (!AbilitySystemComponent) {
            UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_ReceivedHit: AbilitySystemComponent is NULL."));
            return;
        }

        UDamageResult *DamageResult = NewObject<UDamageResult>();
        DamageResult->OldHealth = OldValue;
        DamageResult->NewHealth = NewValue;

        // Send a gameplay event to the source ASC.
        FGameplayEventData Payload;
        Payload.EventTag = OnReceivedHitGameplayEventTag;
        Payload.Instigator = DamageInstigator;
        Payload.Target = SourceASC->GetAvatarActor();
        Payload.OptionalObject = DamageEffectSpec->Def;
        Payload.OptionalObject2 = DamageResult;
        Payload.ContextHandle = DamageEffectSpec->GetContext();
        Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
        Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = DamageMagnitude;

        FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);

        // Source ASC
        AbilitySystemComponent->HandleGameplayEvent(OnReceivedHitGameplayEventTag, &Payload);
    }
    else {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_ReceivedHit: Source ASC is NULL."));
    }
}

void UMythicLifeComponent::TriggerGameplayEvent_DeliveredHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                              float OldValue, float NewValue) {
    if (!OnDeliveredHealGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_DeliveredHeal: OnDeliveredHealGameplayEventTag is not set."));
        return;
    }

    if (auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent()) {
        if (!AbilitySystemComponent) {
            UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHeal: AbilitySystemComponent is NULL."));
            return;
        }

        UDamageResult *DamageResult = NewObject<UDamageResult>();
        DamageResult->OldHealth = OldValue;
        DamageResult->NewHealth = NewValue;

        FGameplayEventData Payload;
        Payload.EventTag = OnDeliveredHealGameplayEventTag;
        Payload.Instigator = DamageInstigator;
        Payload.Target = AbilitySystemComponent->GetAvatarActor();
        Payload.OptionalObject = DamageEffectSpec->Def;
        Payload.OptionalObject2 = DamageResult;
        Payload.ContextHandle = DamageEffectSpec->GetContext();
        Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
        Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
        Payload.EventMagnitude = DamageMagnitude;

        FScopedPredictionWindow NewScopedWindow(SourceASC, true);

        // Source ASC
        SourceASC->HandleGameplayEvent(OnDeliveredHealGameplayEventTag, &Payload);
    }
    else {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_DeliveredHeal: Source ASC is NULL."));
    }
}

void UMythicLifeComponent::TriggerGameplayEvent_ReceivedHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                             float OldValue, float NewValue) {
    if (!OnHealReceivedGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_ReceivedHeal: OnHealReceivedGameplayEventTag is not set."));
        return;
    }

    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_ReceivedHeal: AbilitySystemComponent is NULL."));
        return;
    }

    auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();
    if (!SourceASC) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_ReceivedHeal: Source ASC is NULL."));
        return;
    }

    // Create the damage result uobject.
    UDamageResult *DamageResult = NewObject<UDamageResult>();
    DamageResult->OldHealth = OldValue;
    DamageResult->NewHealth = NewValue;

    FGameplayEventData Payload;
    Payload.EventTag = OnHealReceivedGameplayEventTag;
    Payload.Instigator = DamageInstigator;
    Payload.Target = SourceASC->GetAvatarActor();
    Payload.OptionalObject = DamageEffectSpec->Def;
    Payload.OptionalObject2 = DamageResult;
    Payload.ContextHandle = DamageEffectSpec->GetContext();
    Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
    Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
    Payload.EventMagnitude = DamageMagnitude;

    FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);

    AbilitySystemComponent->HandleGameplayEvent(OnHealReceivedGameplayEventTag, &Payload);
}

void UMythicLifeComponent::TriggerGameplayEvent_Death(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                      float OldValue, float NewValue) {
    if (!OnDeathGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_Death: OnDeathGameplayEventTag is not set."));
        return;
    }
    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Death: AbilitySystemComponent is NULL."));
        return;
    }

    auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();
    if (!SourceASC) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Death: Source ASC is NULL."));
        return;
    }

    UDamageResult *DamageResult = NewObject<UDamageResult>();
    DamageResult->OldHealth = OldValue;
    DamageResult->NewHealth = NewValue;

    FGameplayEventData Payload;
    Payload.EventTag = OnDeathGameplayEventTag;
    Payload.Instigator = DamageInstigator;
    Payload.Target = SourceASC->GetAvatarActor();
    Payload.OptionalObject = DamageEffectSpec->Def;
    Payload.OptionalObject2 = DamageResult;
    Payload.ContextHandle = DamageEffectSpec->GetContext();
    Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
    Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
    Payload.EventMagnitude = DamageMagnitude;

    FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);

    AbilitySystemComponent->HandleGameplayEvent(OnDeathGameplayEventTag, &Payload);
}

void UMythicLifeComponent::TriggerGameplayEvent_Kill(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                                     float OldValue, float NewValue) {
    if (!OnDeathGameplayEventTag.IsValid()) {
        UE_LOG(Myth, Error, TEXT("Skipping TriggerGameplayEvent_Kill: OnKillGameplayEventTag is not set."));
        return;
    }
    if (!AbilitySystemComponent) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Kill: AbilitySystemComponent is NULL."));
        return;
    }

    auto SourceASC = DamageEffectSpec->GetEffectContext().GetOriginalInstigatorAbilitySystemComponent();
    if (!SourceASC) {
        UE_LOG(Myth, Error, TEXT("MythicHealthComponent: TriggerGameplayEvent_Kill: Source ASC is NULL."));
        return;
    }

    UDamageResult *DamageResult = NewObject<UDamageResult>();
    DamageResult->OldHealth = OldValue;
    DamageResult->NewHealth = NewValue;

    FGameplayEventData Payload;
    Payload.EventTag = OnKillGameplayEventTag;
    Payload.Instigator = DamageInstigator;
    Payload.Target = AbilitySystemComponent->GetAvatarActor();
    Payload.OptionalObject = DamageEffectSpec->Def;
    Payload.OptionalObject2 = DamageResult;
    Payload.ContextHandle = DamageEffectSpec->GetContext();
    Payload.InstigatorTags = *DamageEffectSpec->CapturedSourceTags.GetAggregatedTags();
    Payload.TargetTags = *DamageEffectSpec->CapturedTargetTags.GetAggregatedTags();
    Payload.EventMagnitude = DamageMagnitude;

    FScopedPredictionWindow NewScopedWindow(SourceASC, true);

    SourceASC->HandleGameplayEvent(OnKillGameplayEventTag, &Payload);
}

void UMythicLifeComponent::HandleHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec,
                                               float DamageMagnitude, float OldValue, float NewValue) {
    // Trigger Heal Received and Heal Delivered events if health is going up and the old value is greater than 0.
    if (NewValue > OldValue && OldValue > 0.0f) {
        TriggerGameplayEvent_DeliveredHeal(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
        TriggerGameplayEvent_ReceivedHeal(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }
    // Trigger Death event if health is going down and the new value is less than or equal to 0.
    else if (NewValue <= 0.0f) {
        TriggerGameplayEvent_Death(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
        TriggerGameplayEvent_Kill(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }
    // Trigger Hit Delivered event if health is going down and the old value is greater than 0.
    else if (NewValue < OldValue && OldValue > 0.0f) {
        TriggerGameplayEvent_DeliveredHit(DamageInstigator, DamageEffectSpec, DamageMagnitude, OldValue, NewValue);
    }

    auto ContextHandle = DamageEffectSpec->GetContext();

}

void UMythicLifeComponent::HandleMaxHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec,
                                                  float DamageMagnitude, float OldValue, float NewValue) {}
