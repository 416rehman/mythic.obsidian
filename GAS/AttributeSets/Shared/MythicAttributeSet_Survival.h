
#pragma once

#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Survival.generated.h"

UCLASS()
class MYTHIC_API UMythicAttributeSet_Survival : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    // Food. 0 = starving, Max = fully fed. Decays slowly over time; restored by eating.
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Nourishment)
    FGameplayAttributeData Nourishment;
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxNourishment)
    FGameplayAttributeData MaxNourishment;

    // Water. 0 = dehydrated, Max = fully hydrated. Decays slightly faster than food; restored by drinking.
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Hydration)
    FGameplayAttributeData Hydration;
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxHydration)
    FGameplayAttributeData MaxHydration;

    // Body warmth. 0 = freezing (Cold debuff), Max = hot (Overheated). NEUTRAL ~50 at rest. Warm sources raise it,
    // cold/wet weather lowers it.
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Warmth)
    FGameplayAttributeData Warmth;
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxWarmth)
    FGameplayAttributeData MaxWarmth;

    // Wetness. 0 = dry, Max = soaked. Rises standing in rain/snow (unless sheltered/warm), dries otherwise. High wetness
    // aggravates cold (see FMythicSurvivalCore::ResolveStatus).
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Wetness)
    FGameplayAttributeData Wetness;
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxWetness)
    FGameplayAttributeData MaxWetness;

public:
    UMythicAttributeSet_Survival();

    virtual void PreAttributeChange(const FGameplayAttribute &Attribute, float &NewValue) override;
    virtual void PreAttributeBaseChange(const FGameplayAttribute &Attribute, float &NewValue) const override;
    virtual void PostGameplayEffectExecute(const FGameplayEffectModCallbackData &Data) override;

    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, Nourishment)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, MaxNourishment)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, Hydration)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, MaxHydration)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, Warmth)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, MaxWarmth)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, Wetness)
    ATTRIBUTE_ACCESSORS(UMythicAttributeSet_Survival, MaxWetness)

    UFUNCTION()
    virtual void OnRep_Nourishment(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MaxNourishment(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_Hydration(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MaxHydration(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_Warmth(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MaxWarmth(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_Wetness(const FGameplayAttributeData &OldValue);
    UFUNCTION()
    virtual void OnRep_MaxWetness(const FGameplayAttributeData &OldValue);

    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

private:
    void ClampAttribute(const FGameplayAttribute &Attribute, float &NewValue) const;
};
