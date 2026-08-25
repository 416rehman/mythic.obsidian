
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Proficiencies.generated.h"

UCLASS()
class MYTHIC_API UMythicAttributeSet_Proficiencies : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    // Killing, Combat, Fighting etc
    UPROPERTY(BlueprintReadOnly, Category = "Combat Proficiency", ReplicatedUsing = OnRep_CombatProficiency)
    FGameplayAttributeData CombatProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Combat Proficiency", ReplicatedUsing = OnRep_CombatProficiencyMax)
    FGameplayAttributeData CombatProficiencyMax;

    // Cutting wood, trees.
    UPROPERTY(BlueprintReadOnly, Category = "Woodcutting Proficiency", ReplicatedUsing = OnRep_WoodcuttingProficiency)
    FGameplayAttributeData WoodcuttingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Woodcutting Proficiency", ReplicatedUsing = OnRep_WoodcuttingProficiencyMax)
    FGameplayAttributeData WoodcuttingProficiencyMax;

    // Mining, digging, mining ores.
    UPROPERTY(BlueprintReadOnly, Category = "Mining Proficiency", ReplicatedUsing = OnRep_MiningProficiency)
    FGameplayAttributeData MiningProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Mining Proficiency", ReplicatedUsing = OnRep_MiningProficiencyMax)
    FGameplayAttributeData MiningProficiencyMax;

    // Construction, building, repairing.
    UPROPERTY(BlueprintReadOnly, Category = "Construction Proficiency", ReplicatedUsing = OnRep_ConstructionProficiency)
    FGameplayAttributeData ConstructionProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Construction Proficiency", ReplicatedUsing = OnRep_ConstructionProficiencyMax)
    FGameplayAttributeData ConstructionProficiencyMax;

    // Trading, buying and selling items.
    UPROPERTY(BlueprintReadOnly, Category = "Trading Proficiency", ReplicatedUsing = OnRep_TradingProficiency)
    FGameplayAttributeData TradingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Trading Proficiency", ReplicatedUsing = OnRep_TradingProficiencyMax)
    FGameplayAttributeData TradingProficiencyMax;

    // Hunting, killing animals.
    UPROPERTY(BlueprintReadOnly, Category = "Hunting Proficiency", ReplicatedUsing = OnRep_HuntingProficiency)
    FGameplayAttributeData HuntingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Hunting Proficiency", ReplicatedUsing = OnRep_HuntingProficiencyMax)
    FGameplayAttributeData HuntingProficiencyMax;

    // Fishing, catching fish.
    UPROPERTY(BlueprintReadOnly, Category = "Fishing Proficiency", ReplicatedUsing = OnRep_FishingProficiency)
    FGameplayAttributeData FishingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Fishing Proficiency", ReplicatedUsing = OnRep_FishingProficiencyMax)
    FGameplayAttributeData FishingProficiencyMax;

    // Farming, planting, watering plants.
    UPROPERTY(BlueprintReadOnly, Category = "Farming Proficiency", ReplicatedUsing = OnRep_FarmingProficiency)
    FGameplayAttributeData FarmingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Farming Proficiency", ReplicatedUsing = OnRep_FarmingProficiencyMax)
    FGameplayAttributeData FarmingProficiencyMax;

    // Harvesting, gathering crops, shrubs, plants, etc.
    UPROPERTY(BlueprintReadOnly, Category = "Harvesting Proficiency", ReplicatedUsing = OnRep_HarvestingProficiency)
    FGameplayAttributeData HarvestingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Harvesting Proficiency", ReplicatedUsing = OnRep_HarvestingProficiencyMax)
    FGameplayAttributeData HarvestingProficiencyMax;

    // Crafting, making items.
    UPROPERTY(BlueprintReadOnly, Category = "Crafting Proficiency", ReplicatedUsing = OnRep_CraftingProficiency)
    FGameplayAttributeData CraftingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Crafting Proficiency", ReplicatedUsing = OnRep_CraftingProficiencyMax)
    FGameplayAttributeData CraftingProficiencyMax;

    // Alchemy, making potions, poisons, etc.
    UPROPERTY(BlueprintReadOnly, Category = "Alchemy Proficiency", ReplicatedUsing = OnRep_AlchemyProficiency)
    FGameplayAttributeData AlchemyProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Alchemy Proficiency", ReplicatedUsing = OnRep_AlchemyProficiencyMax)
    FGameplayAttributeData AlchemyProficiencyMax;

    // Cooking, cooking food.
    UPROPERTY(BlueprintReadOnly, Category = "Cooking Proficiency", ReplicatedUsing = OnRep_CookingProficiency)
    FGameplayAttributeData CookingProficiency;
    UPROPERTY(BlueprintReadOnly, Category = "Cooking Proficiency", ReplicatedUsing = OnRep_CookingProficiencyMax)
    FGameplayAttributeData CookingProficiencyMax;

    // AUTO-CALCULATED: Overall Xp based on all proficiencies
    UPROPERTY(BlueprintReadOnly, Category = "Overall Xp", ReplicatedUsing = OnRep_OverallXp)
    FGameplayAttributeData OverallXp;
    UPROPERTY(BlueprintReadOnly, Category = "Overall Xp", ReplicatedUsing = OnRep_OverallXpMax)
    FGameplayAttributeData OverallXpMax;

public:
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CombatProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CombatProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, WoodcuttingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, WoodcuttingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, MiningProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, MiningProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, ConstructionProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, ConstructionProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, TradingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, TradingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, HuntingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, HuntingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, FishingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, FishingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, FarmingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, FarmingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, HarvestingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, HarvestingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CraftingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CraftingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, AlchemyProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, AlchemyProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CookingProficiency);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, CookingProficiencyMax);

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, OverallXp);
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Proficiencies, OverallXpMax);
    const float COMBAT_WEIGHT = 2.2f;
    const float MINING_WEIGHT = 2.0f;
    const float WOODCUTTING_WEIGHT = 2.0f;
    const float HUNTING_WEIGHT = 2.0f;
    const float CRAFTING_WEIGHT = 1.8f;
    const float CONSTRUCTION_WEIGHT = 1.8f;
    const float FISHING_WEIGHT = 1.5f;
    const float HARVESTING_WEIGHT = 1.5f;
    const float FARMING_WEIGHT = 1.5f;
    const float TRADING_WEIGHT = 1.5f;
    const float ALCHEMY_WEIGHT = 1.3f;
    const float COOKING_WEIGHT = 1.0f;
    const float HIGH_SKILL_MULTIPLIER = 0.15f;
    const float MEDIUM_SKILL_MULTIPLIER = 0.10f;
    const float LOW_SKILL_MULTIPLIER = 0.05f;

public:
    UFUNCTION()
    virtual void OnRep_CombatProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CombatProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_WoodcuttingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_WoodcuttingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_MiningProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MiningProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_ConstructionProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_ConstructionProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_TradingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_TradingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_HuntingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_HuntingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_FishingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_FishingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_FarmingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_FarmingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_HarvestingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_HarvestingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_CraftingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CraftingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_AlchemyProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_AlchemyProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_CookingProficiency(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_CookingProficiencyMax(const FGameplayAttributeData &OldValue);

    UFUNCTION()
    virtual void OnRep_OverallXp(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_OverallXpMax(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
    float GetMaxValueForAttribute(const FGameplayAttribute &Attribute);

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;

    virtual void PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const override;

    /**
     * The multiplier a gain of XP is worth once everything that scales it is folded together. Rested multiplies
     * rather than adds, because it is a state the player earned away from the work rather than a stat they carry.
     */
    static float ComposeXpMultipliers(float ProficiencyXpBonus, float EnlightenBonus, float RestedMultiplier);

    float ScaleProficiencyXpGain(float BaseXp, const UAbilitySystemComponent *ASC) const;

    static float ApplyWorldTierXpMultiplier(float ScaledXp, float WorldTierMultiplier);

    virtual void PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) override;

    static int32 GetLevel(const UAbilitySystemComponent* ASC, bool& found);

    /** The one XP-to-level formula, shared by GetLevel and every magnitude calculation that captures XP. */
    static int32 LevelFromXp(float CurrentXp, float MaxXp);

    /**
     * The current level plus the XP window around it: how far into the level the character is and how
     * wide the level is, in the same units as CurrentXp. This is what a level readout shows - lifetime
     * totals mean nothing to a player mid-bar.
     */
    static void GetLevelXpWindow(float CurrentXp, float MaxXp, int32 &OutLevel, float &OutIntoLevel, float &OutLevelSpan);

private:
    float CalculateOverallXpMax();
    float CalculateOverallXp();
};
