#pragma once
#include "AbilitySystemComponent.h"
#include "MythicAbilityTagRelationshipMapping.h"
#include "Abilities/MythicGameplayAbility.h"
#include "AttributeSets/MythicAttributeSet.h"
#include "MythicAbilitySystemComponent.generated.h"

UCLASS(BlueprintType, Blueprintable)
class MYTHIC_API UMythicAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()
public:    
    // If set, this table is used to look up tag relationships for activate and cancel
    UPROPERTY()
    TObjectPtr<UMythicAbilityTagRelationshipMapping> AbilityTagRelationshipMapping;

    /** Looks at ability tags and gathers additional required and blocking tags */
    // For MythicAbilityTagRelationshipMapping
    void GetAdditionalActivationTagRequirements(const FGameplayTagContainer& AbilityTags, FGameplayTagContainer& OutActivationRequired, FGameplayTagContainer& OutActivationBlocked) const;

    virtual void BeginPlay() override;

    // Returns all the AttributeSets that are currently registered to this AbilitySystemComponent
    UFUNCTION(BlueprintCallable, Category = MythicAbilitySystemComponent)
    const TArray<UMythicAttributeSet *> &GetAttributeSets() const;

    // On ANY attribute change, this will be called
    
};