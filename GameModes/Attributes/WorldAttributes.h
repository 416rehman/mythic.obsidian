
#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "WorldAttributes.generated.h"

UCLASS()
class MYTHIC_API UWorldTierAttributes : public UMythicAttributeSet {
    GENERATED_BODY()

protected:

    // Gold drop rate multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_GoldDropRateMultiplier)
    FGameplayAttributeData GoldDropRateMultiplier;

    // Experience gain multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ExperienceGainMultiplier)
    FGameplayAttributeData ExperienceGainMultiplier;

    // Legendary drop rate multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_LegendaryDropRateMultiplier)
    FGameplayAttributeData LegendaryDropRateMultiplier;

    // Mythic drop rate multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MythicDropRateMultiplier)
    FGameplayAttributeData MythicDropRateMultiplier;

    // Enemy health multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_EnemyHealthMultiplier)
    FGameplayAttributeData EnemyHealthMultiplier;

    // Enemy damage multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_EnemyDamageMultiplier)
    FGameplayAttributeData EnemyDamageMultiplier;

    /**
     * Item level every drop in this world starts from, before the slain enemy's tier bonus.
     *
     * An attribute rather than a setting so the same world-tier gameplay effect that pushes the enemy
     * multipliers above also moves what the world drops - one ladder, not two that can disagree.
     */
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_ItemLevelBase)
    FGameplayAttributeData ItemLevelBase;

public:
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, GoldDropRateMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, ExperienceGainMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, LegendaryDropRateMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, MythicDropRateMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, EnemyHealthMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, EnemyDamageMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, ItemLevelBase);

    UFUNCTION()
    virtual void OnRep_GoldDropRateMultiplier(const FGameplayAttributeData &OldGoldDropRate);
    UFUNCTION()
    virtual void OnRep_ExperienceGainMultiplier(const FGameplayAttributeData &OldExperienceGain);
    UFUNCTION()
    virtual void OnRep_LegendaryDropRateMultiplier(const FGameplayAttributeData &OldLegendaryDropRate);
    UFUNCTION()
    virtual void OnRep_MythicDropRateMultiplier(const FGameplayAttributeData &OldMythicDropRate);
    UFUNCTION()
    virtual void OnRep_EnemyHealthMultiplier(const FGameplayAttributeData &OldEnemyHealth);
    UFUNCTION()
    virtual void OnRep_EnemyDamageMultiplier(const FGameplayAttributeData &OldEnemyDamage);
    UFUNCTION()
    virtual void OnRep_ItemLevelBase(const FGameplayAttributeData &OldItemLevelBase);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
