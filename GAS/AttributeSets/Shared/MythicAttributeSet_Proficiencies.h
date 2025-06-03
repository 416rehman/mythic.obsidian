// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Proficiencies.generated.h"

/**
 * 
 */
UCLASS()
class MYTHIC_API UMythicAttributeSet_Proficiencies : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    // Killing, Combat, Fighting etc
    UPROPERTY(BlueprintReadOnly, Category = "Combat Proficiency", ReplicatedUsing = OnRep_CombatProficiency)
    FGameplayAttributeData CombatProficiency;

    // Cutting wood, trees.
    UPROPERTY(BlueprintReadOnly, Category = "Woodcutting Proficiency", ReplicatedUsing = OnRep_WoodcuttingProficiency)
    FGameplayAttributeData WoodcuttingProficiency;

    // Mining, digging, mining ores.
    UPROPERTY(BlueprintReadOnly, Category = "Mining Proficiency", ReplicatedUsing = OnRep_MiningProficiency)
    FGameplayAttributeData MiningProficiency;

    // Construction, building, repairing.
    UPROPERTY(BlueprintReadOnly, Category = "Construction Proficiency", ReplicatedUsing = OnRep_ConstructionProficiency)
    FGameplayAttributeData ConstructionProficiency;

    // Trading, buying and selling items.
    UPROPERTY(BlueprintReadOnly, Category = "Trading Proficiency", ReplicatedUsing = OnRep_TradingProficiency)
    FGameplayAttributeData TradingProficiency;

    // Hunting, killing animals.
    UPROPERTY(BlueprintReadOnly, Category = "Hunting Proficiency", ReplicatedUsing = OnRep_HuntingProficiency)
    FGameplayAttributeData HuntingProficiency;

    // Fishing, catching fish.
    UPROPERTY(BlueprintReadOnly, Category = "Fishing Proficiency", ReplicatedUsing = OnRep_FishingProficiency)
    FGameplayAttributeData FishingProficiency;

    // Farming, planting, watering plants.
    UPROPERTY(BlueprintReadOnly, Category = "Farming Proficiency", ReplicatedUsing = OnRep_FarmingProficiency)
    FGameplayAttributeData FarmingProficiency;

    // Harvesting, gathering crops, shrubs, plants, etc.
    UPROPERTY(BlueprintReadOnly, Category = "Harvesting Proficiency", ReplicatedUsing = OnRep_HarvestingProficiency)
    FGameplayAttributeData HarvestingProficiency;

    // Crafting, making items.
    UPROPERTY(BlueprintReadOnly, Category = "Crafting Proficiency", ReplicatedUsing = OnRep_CraftingProficiency)
    FGameplayAttributeData CraftingProficiency;

    // Alchemy, making potions, poisons, etc.
    UPROPERTY(BlueprintReadOnly, Category = "Alchemy Proficiency", ReplicatedUsing = OnRep_AlchemyProficiency)
    FGameplayAttributeData AlchemyProficiency;

    // Cooking, cooking food.
    UPROPERTY(BlueprintReadOnly, Category = "Cooking Proficiency", ReplicatedUsing = OnRep_CookingProficiency)
    FGameplayAttributeData CookingProficiency;

    // Map of FGameplayAttributes and their Maximum values.
    // This can be updated at runtime to change the maximum values of the attributes by ProficiencyDefinitions.
    UPROPERTY(BlueprintReadOnly, Category = "Proficiencies")
    TMap<FString, float> ProficiencyMaximums;

public:
    // Set the maximum value for an attribute by name.
    // The target attribute will be clamped to this value on changes.
    void SetMaximumValue(const FString &AttributeName, float Value) {
        ProficiencyMaximums.Add(AttributeName, Value);
    }

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CombatProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, WoodcuttingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, MiningProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, ConstructionProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, TradingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, HuntingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, FishingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, FarmingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, HarvestingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CraftingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, AlchemyProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CookingProficiency);

public:
    UFUNCTION()
    virtual void OnRep_CombatProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_WoodcuttingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MiningProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_ConstructionProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_TradingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_HuntingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_FishingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_FarmingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_HarvestingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CraftingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_AlchemyProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CookingProficiency(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;
};
