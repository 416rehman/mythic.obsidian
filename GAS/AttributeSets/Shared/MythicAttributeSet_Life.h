
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Life.generated.h"

UENUM(BlueprintType)
enum class EMythicLethalOutcome : uint8 {
    Survive,
    EnterDownState,
    Die
};

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

    float HealthBeforeAttributeChange;
    float MaxHealthBeforeAttributeChange;

    bool bOutOfHealth = false;

    bool bInDeathPreHook = false;

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

    mutable FMythicAttributeEvent OnHealthChanged;
    mutable FMythicAttributeEvent OnMaxHealthChanged;
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    virtual bool PreGameplayEffectExecute(FGameplayEffectModCallbackData &Data) override;
    void SendEventToInstigator(const FGameplayEffectModCallbackData &Data, AActor *Instigator, UAbilitySystemComponent *InstigatorASC,
                               UAbilitySystemComponent *OwnerASC,
                               FGameplayTag EventTag, float Magnitude);
    void SendEventToOwner(const FGameplayEffectModCallbackData &Data, UAbilitySystemComponent *OwnerASC, AActor *Instigator, FGameplayTag EventTag,
                          float Magnitude);
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    virtual void ClampAttributes(const FGameplayAttribute &Attribute, float &NewValue);

    static bool ComputeOutOfHealthLatch(float NewHealth);

    static EMythicLethalOutcome ResolveLethalOutcome(bool bWouldBeLethal, bool bCoopDownStateEnabled, bool bAlreadyDowned, bool bRevivablePawn);

    // True once Health has hit zero (the server-authoritative death latch).
    UFUNCTION(BlueprintPure, Category = "Attributes")
    bool IsDead() const { return bOutOfHealth; }

    void ResetForRespawn();
};
