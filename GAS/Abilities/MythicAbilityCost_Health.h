#pragma once
#include "CoreMinimal.h"
#include "MythicAbilityCost.h"
#include "MythicAbilityCost_Health.generated.h"

UCLASS(EditInlineNew)
class MYTHIC_API UMythicAbilityCost_Health : public UMythicAbilityCost {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Cost",meta=(ClampMin="0",ClampMax="0.95")) float MaxHealthFraction = 0.15f;
    UPROPERTY(EditAnywhere,BlueprintReadOnly,Category="Cost") FGameplayTag FractionParameter;
    virtual bool CheckCost(const UMythicGameplayAbility* Ability,FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,FGameplayTagContainer* OptionalRelevantTags) const override;
    virtual void ApplyCost(const UMythicGameplayAbility* Ability,FGameplayAbilitySpecHandle Handle,const FGameplayAbilityActorInfo* ActorInfo,FGameplayAbilityActivationInfo ActivationInfo) override;
private:
    float ResolveFraction(const UMythicGameplayAbility* Ability) const;
};
