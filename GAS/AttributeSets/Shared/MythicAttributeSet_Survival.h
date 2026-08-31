
#pragma once

#include "AbilitySystemComponent.h"
#include "GAS/AttributeSets/MythicAttributeSet.h"
#include "MythicAttributeSet_Survival.generated.h"

UCLASS()
class MYTHIC_API UMythicAttributeSet_Survival : public UMythicAttributeSet {
    GENERATED_BODY()

protected:
    /** Food: zero is starving and MaxNourishment is fully fed. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Nourishment)
    FGameplayAttributeData Nourishment;
    /** Maximum nourishment capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxNourishment)
    FGameplayAttributeData MaxNourishment;

    /** Hydration: zero is dehydrated and MaxHydration is fully hydrated. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Hydration)
    FGameplayAttributeData Hydration;
    /** Maximum hydration capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxHydration)
    FGameplayAttributeData MaxHydration;

    /** Body warmth; zero is freezing, roughly 50 is neutral, and MaxWarmth is overheated. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Warmth)
    FGameplayAttributeData Warmth;
    /** Maximum warmth gauge capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxWarmth)
    FGameplayAttributeData MaxWarmth;

    /** Wetness: zero is dry and MaxWetness is fully soaked. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_Wetness)
    FGameplayAttributeData Wetness;
    /** Maximum wetness gauge capacity. */
    UPROPERTY(BlueprintReadOnly, Category = "Survival", ReplicatedUsing = OnRep_MaxWetness)
    FGameplayAttributeData MaxWetness;

    virtual TConstArrayView<FMythicBoundedAttributePair> GetBoundedAttributePairs() const override;

public:
    UMythicAttributeSet_Survival();

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
};
