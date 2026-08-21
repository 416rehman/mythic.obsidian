
#pragma once

#include "MythicAbilityCost.h"
#include "ScalableFloat.h"
#include "GameplayTagContainer.h"
#include "MythicAbilityCost_Stamina.generated.h"

class UMythicGameplayAbility;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilitySpecHandle;
struct FGameplayAbilityActivationInfo;

UCLASS(meta = (DisplayName = "Stamina"))
class UMythicAbilityCost_Stamina : public UMythicAbilityCost {
    GENERATED_BODY()

public:
    UMythicAbilityCost_Stamina();

    virtual bool CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           FGameplayTagContainer *OptionalRelevantTags) const override;
    virtual void ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo) override;

protected:
    /** Stamina spent per activation (keyed on ability level). StaminaCostReduction is applied on top. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FScalableFloat Cost;

    /** Tag sent back when the cost cannot be paid (drives "out of stamina" feedback, e.g. a UI flash). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FGameplayTag FailureTag;
};
