
#pragma once
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Utility.generated.h"

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

    // Maximum cap for cooldown reduction, defaults to 0.60 (60%)
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_MaxCooldownReduction)
    FGameplayAttributeData MaxCooldownReduction;

    // bonus proficiency XP gained from all sources
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_ProficiencyXPBonus)
    FGameplayAttributeData ProficiencyXPBonus;

    // The one attribute that decides how fast the owner moves, read as a percentage: 1.0 is 100% (default speed),
    // 2.0 is double. Gear, slows, haste and every other speed effect move this and nothing else.
    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_MovementSpeedMultiplier)
    FGameplayAttributeData MovementSpeedMultiplier;

    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_ItemRarityFind)
    FGameplayAttributeData ItemRarityFind;

    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_ItemQuantityFind)
    FGameplayAttributeData ItemQuantityFind;

public:
    UMythicAttributeSet_Utility();

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    virtual void PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const override;

    virtual void PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) override;

    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    static bool IsReductionFractionAttribute(const FGameplayAttribute &Attribute);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, Resolve)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, MaxStamina)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, CurrentStamina)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, StaminaRegenRate)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, StaminaCostReduction)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, CooldownReduction)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, MaxCooldownReduction)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, ProficiencyXPBonus)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, MovementSpeedMultiplier)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, ItemRarityFind)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, ItemQuantityFind)

    UFUNCTION()
    virtual void OnRep_Resolve(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MaxStamina(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CurrentStamina(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_StaminaRegenRate(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_StaminaCostReduction(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CooldownReduction(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MaxCooldownReduction(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_ProficiencyXPBonus(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MovementSpeedMultiplier(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_ItemRarityFind(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_ItemQuantityFind(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
    mutable bool bIsUpdatingMaxStamina = false;
};
