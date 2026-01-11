#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "MythicAttributeSet.generated.h"

/**
 * This macro defines a set of helper functions for accessing and initializing attributes.
 *
 * The following example of the macro:
 *		ATTRIBUTE_ACCESSORS(UMythicHealthSet, Health)
 * will create the following functions:
 *		static FGameplayAttribute GetHealthAttribute();
 *		float GetHealth() const;
 *		void SetHealth(float NewVal);
 *		void InitHealth(float NewVal);
 */
#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

struct FGameplayEffectSpec;
/** 
 * Delegate used to broadcast attribute events, some of these parameters may be null on clients: 
 * @param EffectInstigator	The original instigating actor for this event
 * @param EffectCauser		The physical actor that caused the change
 * @param EffectSpec		The full effect spec for this change
 * @param EffectMagnitude	The raw magnitude, this is before clamping
 * @param OldValue			The value of the attribute before it was changed
 * @param NewValue			The value after it was changed
*/
DECLARE_MULTICAST_DELEGATE_SixParams(FMythicAttributeEvent, AActor* /*EffectInstigator*/, AActor* /*EffectCauser*/, const FGameplayEffectSpec* /*EffectSpec*/,
                                     float /*EffectMagnitude*/, float /*OldValue*/, float /*NewValue*/);

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
