// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Life.generated.h"

/**
 * When Health gets changed by an effect, the following flow occurs:
 * (SERVER: PostGameplayEffectExecute) -> HealthChangeDelegate_Internal/MaxHealthChangeDelegate_Internal -> HealthComponent -> Broadcast to owner actor & Trigger(OnHit/OnDeath)GameplayEvent
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_Life : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MaxHealth)
    FGameplayAttributeData MaxHealth;
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_Health)
    FGameplayAttributeData Health;

    // Meta Attribute - NOT replicated, used for damage/healing calculations
    // Damage is applied to this, then converted to -Health in PostGameplayEffectExecute
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Damage;
    // Meta Attribute - NOT replicated, used for damage/healing calculations
    // Healing is applied to this, then converted to +Health in PostGameplayEffectExecute
    UPROPERTY(BlueprintReadOnly, Category = "Attributes")
    FGameplayAttributeData Healing;

    // Store the health before any changes are made by a gameplay effect
    float HealthBeforeAttributeChange;
    float MaxHealthBeforeAttributeChange;

    // Track if we're already dead to prevent re-triggering death
    bool bOutOfHealth = false;

public:
    UMythicAttributeSet_Life();

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, MaxHealth);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, Health);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, Damage);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Life, Healing);

    UFUNCTION()
    virtual void OnRep_MaxHealth(const FGameplayAttributeData &OldMaxHealth);
    UFUNCTION()
    virtual void OnRep_Health(const FGameplayAttributeData &OldHealth);

    //~ Delegate when health changes due to damage/healing, some information may be missing on the client
    // Effect -> Delegate -> HealthComponent -> Broadcast to owner actor & Trigger(OnHit/OnDeath)GameplayEvent
    mutable FMythicAttributeEvent OnHealthChanged;
    mutable FMythicAttributeEvent OnMaxHealthChanged;
    //~
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    // PreAttributeChange is called just before any modification to attributes takes place.
    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    // PreGameplayEffectExecute is called just before a GE is executed. This can be used to modify the GE or cancel it.
    virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data) override;
    // PostGameplayEffectExecute is called after a GE is executed. This can be used to show damage numbers, play sounds, etc.
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    // Clamp attributes to valid values
    virtual void ClampAttributes(const FGameplayAttribute &Attribute, float &NewValue);
};
