// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayEffectExtension.h"
#include "MythicAttributeSet_Life.h"
#include "Components/GameFrameworkComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "GameplayEffectTypes.h"
#include "MythicLifeComponent.generated.h"

class UMythicAbilitySystemComponent;

UCLASS(Blueprintable, BlueprintType)
class UDamageResult : public UObject {
    GENERATED_BODY()

public:
    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float OldHealth;

    UPROPERTY(BlueprintReadOnly, Category = "Damage")
    float NewHealth;

};

// Event for HealthChange
DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FMythicHealthChanged, float, New, float, Old, FGameplayAttribute,
                                              Attribute, const FGameplayEffectContextHandle&, ContextHandle);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicLifeComponent : public UGameFrameworkComponent {
    GENERATED_BODY()

public:
    UMythicLifeComponent(const FObjectInitializer &ObjectInitializer);

    // Returns the health component if one exists on the specified actor.
    UFUNCTION(BlueprintPure, Category = "Mythic|Health")
    static UMythicLifeComponent *FindHealthComponent(const AActor *Actor) {
        return (Actor ? Actor->FindComponentByClass<UMythicLifeComponent>() : nullptr);
    }

    // Initialize the component using an ability system component.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void InitializeWithAbilitySystem(UAbilitySystemComponent *InASC);

    // Uninitialize the component, clearing any references to the ability system.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    void UninitializeFromAbilitySystem();

    // Returns the current health value.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    float GetHealth() const;

    // Returns the current maximum health value.
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    float GetMaxHealth() const;

    // Returns the current health in the range [0.0, 1.0].
    UFUNCTION(BlueprintCallable, Category = "Mythic|Health")
    float GetHealthNormalized() const;

    // SERVER: On Instigator ASC, trigger gameplay event with tag OnDeliveredHitGameplayEventTag.
    void TriggerGameplayEvent_DeliveredHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                           NewValue) const;
    // SERVER: On Instigator ASC, trigger gameplay event with tag OnDeliveredHealGameplayEventTag.
    void TriggerGameplayEvent_DeliveredHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                            NewValue);
    // SERVER: On Instigator ASC, trigger gameplay event with tag OnDeathGameplayEventTag.
    void TriggerGameplayEvent_Kill(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue,
                                   float NewValue);
    // SERVER: On owning ASC, trigger gameplay event with tag OnReceivedHitGameplayEventTag.
    void TriggerGameplayEvent_ReceivedHit(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                          NewValue) const;
    // SERVER: On owning ASC, trigger gameplay event with tag OnHealReceivedGameplayEventTag.
    void TriggerGameplayEvent_ReceivedHeal(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                           NewValue);
    // SERVER: On owning ASC, trigger gameplay event with tag OnDeathGameplayEventTag.
    void TriggerGameplayEvent_Death(AActor *DamageInstigator, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude, float OldValue, float
                                    NewValue);

public:
    void TriggerHealthChange(const FOnAttributeChangeData &OnAttributeChangeData) const {
        if (OnHealthChanged.IsBound()) {
            FGameplayEffectContextHandle Context = FGameplayEffectContextHandle();
            if (auto ModData = OnAttributeChangeData.GEModData) {
                Context = ModData->EffectSpec.GetEffectContext();
            }
            
            OnHealthChanged.Broadcast(OnAttributeChangeData.NewValue, OnAttributeChangeData.OldValue, OnAttributeChangeData.Attribute, Context);
        }
    }

    void TriggerMaxHealthChange(const FOnAttributeChangeData &OnAttributeChangeData) const {
        if (OnMaxHealthChanged.IsBound()) {
            FGameplayEffectContextHandle Context = FGameplayEffectContextHandle();
            if (auto ModData = OnAttributeChangeData.GEModData) {
                Context = ModData->EffectSpec.GetEffectContext();
            }

            OnMaxHealthChanged.Broadcast(OnAttributeChangeData.NewValue, OnAttributeChangeData.OldValue, OnAttributeChangeData.Attribute, Context);
        }
    }

    UPROPERTY(BlueprintAssignable, Blueprintable)
    FMythicHealthChanged OnHealthChanged;

    UPROPERTY(BlueprintAssignable, Blueprintable)
    FMythicHealthChanged OnMaxHealthChanged;

    // If set, gameplay event with this tag will be triggered on the Instigator AbilitySystemComponent when the health value goes down (if currently: 0 < health)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Abilities activated by this event have magnitude set to the amount of damage dealt, and OptionalObject2 set to a UDamageResult object.
    // Use Case: On delivering hit, heal for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Instigator")
    FGameplayTag OnDeliveredHitGameplayEventTag = GAS_EVENT_DMG_DELIVERED;

    // If set, gameplay event with this tag will be triggered on the Instigator AbilitySystemComponent when the health value goes to 0 or below (if currently: 0 < health)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: Killing enemies has a chance to restore health for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Instigator")
    FGameplayTag OnKillGameplayEventTag = GAS_EVENT_KILL;

    // If set, gameplay event with this tag will be triggered on the Instigator AbilitySystemComponent when the health value goes up (if currently: 0 < health <= MaxHealth)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: Healing others restores your health for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS|Instigator")
    FGameplayTag OnDeliveredHealGameplayEventTag = GAS_EVENT_HEAL_DELIVERED;

    // If set, gameplay event with this tag will be triggered on the owning AbilitySystemComponent when the health value goes down (if currently: health > 0)
    // To disable this, unset the tag (OnReceivedHit can be used to trigger similar GameplayEvents).
    // Abilities activated by this event have magnitude set to the amount of damage dealt, and OptionalObject2 set to a UDamageResult object.
    // Use Case: When health below threshold, +30% crit chance.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    FGameplayTag OnReceivedHitGameplayEventTag = GAS_EVENT_DMG_RECEIVED;

    // If set, gameplay event with this tag will be triggered on the owning AbilitySystemComponent when the health value goes up (if currently: 0 < health <= MaxHealth)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: When healed, increase critical chance by X amount for Y seconds
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    FGameplayTag OnHealReceivedGameplayEventTag = GAS_EVENT_HEAL_RECEIVED;

    // If set, gameplay event with this tag will be triggered on the owning AbilitySystemComponent when the health value goes to 0 or below (if currently: 0 < health)
    // To disable this, unset the tag (HealthChangeDelegate_Internal can be used to trigger similar GameplayEvents).
    // Use Case: When you die and you have allies nearby, create an aura at your death location that heals allies for X amount
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GAS")
    FGameplayTag OnDeathGameplayEventTag = GAS_EVENT_DEATH;

protected:
    virtual void OnUnregister() override;
    void BroadcastInitialValues();

    void ClearGameplayTags() const;

    // Internal handler which when the health is changed, sends out the gameplay events.
    virtual void HandleHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                     float OldValue, float NewValue);

    // Internal handler which when the health is changed, sends out the gameplay events.
    virtual void HandleMaxHealthChanged(AActor *DamageInstigator, AActor *DamageCauser, const FGameplayEffectSpec *DamageEffectSpec, float DamageMagnitude,
                                        float OldValue, float NewValue);

protected:
    // Ability system used by this component.
    UPROPERTY()
    TObjectPtr<UAbilitySystemComponent> AbilitySystemComponent;

    // Health set used by this component.
    UPROPERTY()
    TObjectPtr<const UMythicAttributeSet_Life> LifeSet;
};
