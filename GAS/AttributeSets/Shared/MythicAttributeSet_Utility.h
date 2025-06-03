// 

#pragma once
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Utility.generated.h"

/**
 * Utility attributes
 * These attributes are used for utility purposes such as stamina, resolve, and other non-combat attributes.
 * These can influence combat, but are not directly related to damage or defense.
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_Utility : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    // Resolve scales with the player's level, and increases the player's maximum stamina.
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_Resolve)
    FGameplayAttributeData Resolve;

    // Stamina is a resource that is used to power actions such as skills, attacks, sprints, jumps, etc.
    // This is the only resource in the game and is used in combat, exploration, and other activities.
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_MaxStamina)
    FGameplayAttributeData MaxStamina;
    
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_CurrentStamina)
    FGameplayAttributeData CurrentStamina;

    // Resolve Stamina Rate is the rate at which resolve regenerates per second.
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_StaminaRegenRate)
    FGameplayAttributeData StaminaRegenRate;

    // Stamina Cost Reduction is a percentage that reduces the cost of stamina for actions.
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_StaminaCostReduction)
    FGameplayAttributeData StaminaCostReduction;
    
    // Reduces cooldown of all abilities (Q/E)
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_CooldownReduction)
    FGameplayAttributeData CooldownReduction;

    // Bonus experience gained from all sources
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_ExperienceBonus)
    FGameplayAttributeData ExperienceBonus;

    // Bonus Sprint Speed
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_BonusSprintSpeed)
    FGameplayAttributeData BonusSprintSpeed;

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, Resolve)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, MaxStamina)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, CurrentStamina)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, StaminaRegenRate)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, StaminaCostReduction)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, CooldownReduction)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, ExperienceBonus)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, BonusSprintSpeed)

    // Replication
    UFUNCTION()
    virtual void OnRep_Resolve(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_MaxStamina(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_CurrentStamina(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_StaminaCostReduction(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_CooldownReduction(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_ExperienceBonus(const FGameplayAttributeData& OldValue);
    UFUNCTION()
    virtual void OnRep_BonusSprintSpeed(const FGameplayAttributeData& OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
};