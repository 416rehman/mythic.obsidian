
#pragma once
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Utility.generated.h"

UCLASS()
class MYTHIC_API UMythicAttributeSet_Utility : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    /** Resolve scales with player level and is the primary stat MaxStamina derives from. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_Resolve)
    FGameplayAttributeData Resolve;

    /** Maximum stamina available to combat, traversal, and other stamina-consuming actions. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_MaxStamina)
    FGameplayAttributeData MaxStamina;

    /** Current spendable stamina, clamped between zero and MaxStamina. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_CurrentStamina)
    FGameplayAttributeData CurrentStamina;

    /** Stamina restored per second while regeneration is permitted. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_StaminaRegenRate)
    FGameplayAttributeData StaminaRegenRate;

    /** Fractional stamina-cost reduction where 0.25 means twenty-five percent. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_StaminaCostReduction)
    FGameplayAttributeData StaminaCostReduction;

    /** Fractional cooldown reduction applied to eligible abilities. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_CooldownReduction)
    FGameplayAttributeData CooldownReduction;

    /** Runtime cap for CooldownReduction; defaults to 0.60. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_MaxCooldownReduction)
    FGameplayAttributeData MaxCooldownReduction;

    /** Fractional bonus applied to proficiency XP earned from eligible sources. */
    UPROPERTY(BlueprintReadOnly, Category="Utility", ReplicatedUsing=OnRep_ProficiencyXPBonus)
    FGameplayAttributeData ProficiencyXPBonus;

    /** Canonical movement scalar: 1.0 is authored speed and 2.0 is double speed. */
    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_MovementSpeedMultiplier)
    FGameplayAttributeData MovementSpeedMultiplier;

    /** Multiplier used only for item-rarity reward selection; 1.0 is the authored baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_ItemRarityFind)
    FGameplayAttributeData ItemRarityFind;

    /** Multiplier used only for item-quantity reward selection; 1.0 is the authored baseline. */
    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_ItemQuantityFind)
    FGameplayAttributeData ItemQuantityFind;

    /**
     * Multiplies authoritative harvesting work after exact-tool validation; 1.0 is baseline.
     * This is a normal data-driven Utility stat and never selects a tool, node, reward, or proficiency.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Utility", ReplicatedUsing = OnRep_HarvestWorkMultiplier)
    FGameplayAttributeData HarvestWorkMultiplier;

    virtual TConstArrayView<FMythicBoundedAttributePair> GetBoundedAttributePairs() const override;

public:
    UMythicAttributeSet_Utility();

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    virtual void PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const override;

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
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Utility, HarvestWorkMultiplier)

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
    UFUNCTION()
    virtual void OnRep_HarvestWorkMultiplier(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
