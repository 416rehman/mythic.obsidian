// 

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "WorldAttributes.generated.h"

/**
 * Contains the attributes for the current world tier
 */
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

    // Exotic drop rate multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_MythicDropRateMultiplier)
    FGameplayAttributeData ExoticDropRateMultiplier;

    // Enemy health multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_EnemyHealthMultiplier)
    FGameplayAttributeData EnemyHealthMultiplier;

    // Enemy damage multiplier
    UPROPERTY(BlueprintReadOnly, Category = "Attributes", ReplicatedUsing = OnRep_EnemyDamageMultiplier)
    FGameplayAttributeData EnemyDamageMultiplier;

public:
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, GoldDropRateMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, ExperienceGainMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, LegendaryDropRateMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, ExoticDropRateMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, EnemyHealthMultiplier);
    ATTRIBUTE_ACCESSORS(UWorldTierAttributes, EnemyDamageMultiplier);

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

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;
};
