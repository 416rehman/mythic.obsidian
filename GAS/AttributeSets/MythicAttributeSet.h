#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MythicAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectSpec;
DECLARE_MULTICAST_DELEGATE_SixParams(FMythicAttributeEvent, AActor*, AActor*, const FGameplayEffectSpec*,
                                     float, float, float);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FMythicAttributeUpdateEvent, const FGameplayAttribute&, Attribute, float, OldValue, float, NewValue);

UCLASS(Abstract)
class MYTHIC_API UMythicAttributeSet : public UAttributeSet {
    GENERATED_BODY()

public:
    // On ANY Attribute Changed event
    UPROPERTY(BlueprintAssignable, Category = "AttributeSet")
    FMythicAttributeUpdateEvent OnAttributeChanged;

    // Returns all the attributes for this attribute set.
    UFUNCTION(BlueprintCallable, Category = "AttributeSet")
    void GetAttributes(TArray<FGameplayAttribute> &OutAttributes) const;

    virtual void PostAttributeChange(const FGameplayAttribute &Attribute, float OldValue, float NewValue) override;
};
