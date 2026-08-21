
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "MythicGA_Passive.generated.h"

/**
 * One standing effect. Unlike a proc clause there is no event and no roll of the dice: the effect goes on when the
 * ability is granted and stays on until it is removed.
 *
 * When it actually COUNTS is the effect's own business. A modifier inside it carries GAS's normal source and target
 * tag requirements, so gating on GAS.State.Health.Critical is what makes a permanent +damage effect only pay out
 * against a dying foe. That keeps the condition next to the number it guards, where a designer edits it.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicPassiveClause {
    GENERATED_BODY()

    // Infinite effect held for as long as the ability is granted.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive")
    TSubclassOf<UGameplayEffect> EffectToApply;

    /**
     * SetByCaller tag the effect reads its magnitude from, and the key the granting item rolled it under — one tag
     * for both, so an authored effect and its rolled value cannot drift apart.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive")
    FGameplayTag MagnitudeParameter;

    // Magnitude used when MagnitudeParameter resolves no roll.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive")
    float Magnitude = 0.0f;
};

/**
 * Passive ability that holds a list of standing effects for as long as it is granted. The sibling of
 * UMythicGA_Triggered: that one turns events into effects, this one turns being equipped into effects.
 *
 * Like its sibling the behaviour lives entirely in data, so one C++ class backs every standing talent, rune and
 * affix in the game.
 */
UCLASS()
class MYTHIC_API UMythicGA_Passive : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UMythicGA_Passive(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Passive")
    TArray<FMythicPassiveClause> Passives;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                            const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

    // True when the clause would do something. A clause with no effect is inert.
    static bool HasPayload(const FMythicPassiveClause &Clause);

private:
    TArray<FActiveGameplayEffectHandle> AppliedEffects;
};
