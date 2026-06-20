// Mythic — Stamina ability cost

#pragma once

#include "MythicAbilityCost.h"
#include "ScalableFloat.h"
#include "GameplayTagContainer.h"
#include "MythicAbilityCost_Stamina.generated.h"

class UMythicGameplayAbility;
struct FGameplayAbilityActorInfo;
struct FGameplayAbilitySpecHandle;
struct FGameplayAbilityActivationInfo;

/**
 * A cost that spends the avatar's CurrentStamina (UMythicAttributeSet_Utility) when an ability activates.
 * StaminaCostReduction is applied (by UMythicLifeComponent::TrySpendStamina — the single source of the spend rule).
 *
 * Add this to a UMythicGameplayAbility's AdditionalCosts to make it stamina-gated: activation is BLOCKED while stamina
 * is short (CheckCost adds the failure tag), and the pool is deducted server-side on activation (ApplyCost). Because
 * melee/weapon attacks fire through UMythicGameplayAbility, adding this to the attack ability gates attacks for free.
 */
UCLASS(meta = (DisplayName = "Stamina"))
class UMythicAbilityCost_Stamina : public UMythicAbilityCost {
    GENERATED_BODY()

public:
    UMythicAbilityCost_Stamina();

    //~UMythicAbilityCost interface
    virtual bool CheckCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           FGameplayTagContainer *OptionalRelevantTags) const override;
    virtual void ApplyCost(const UMythicGameplayAbility *Ability, const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                           const FGameplayAbilityActivationInfo ActivationInfo) override;
    //~End of UMythicAbilityCost interface

protected:
    /** Stamina spent per activation (keyed on ability level). StaminaCostReduction is applied on top. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FScalableFloat Cost;

    /** Tag sent back when the cost cannot be paid (drives "out of stamina" feedback, e.g. a UI flash). */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Costs)
    FGameplayTag FailureTag;
};
