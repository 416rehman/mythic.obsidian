
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GAS/Abilities/MythicGameplayAbility.h"
#include "MythicGA_Triggered.generated.h"

UENUM(BlueprintType)
enum class EMythicTriggerTarget : uint8 {
    // Whoever is on the other end of the event: the actor we hit, or the actor that hit us.
    Other,
    // This ability's own owner.
    Self,
};

/**
 * One "when X happens, sometimes do Y" clause. A proc ability is a list of these and nothing else, so a designer
 * builds a talent by authoring data rather than a graph.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTriggerSpec {
    GENERATED_BODY()

    // Gameplay event that fires this clause, e.g. GAS.Event.Dmg.Delivered or GAS.Event.Kill.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    FGameplayTag TriggerEvent;

    /**
     * Key into the granting FAbilityDefinition's ParameterRolls, so proc chance is a value the item rolled rather
     * than a constant. Falls back to Chance when the source carries no roll under this tag.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    FGameplayTag ChanceParameter;

    // Proc chance used when ChanceParameter resolves nothing. 1 means every qualifying event.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float Chance = 1.0f;

    // Status handed to the status registry when the clause lands.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger", meta = (Categories = "Status.Type"))
    FGameplayTag StatusToApply;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    EMythicTriggerTarget Target = EMythicTriggerTarget::Other;

    // Seconds before this clause may fire again, so attack speed alone cannot carry a proc build. 0 = no limit.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger", meta = (ClampMin = "0.0"))
    float InternalCooldown = 0.0f;
};

/**
 * Passive ability that turns gameplay events into status applications. Granted by a talent, rune or affix and
 * activated on spawn, it listens for its authored events for as long as it is granted.
 *
 * The behaviour lives entirely in Triggers, so one C++ class backs every proc in the game and each individual proc
 * is a data asset.
 */
UCLASS()
class MYTHIC_API UMythicGA_Triggered : public UMythicGameplayAbility {
    GENERATED_BODY()

public:
    UMythicGA_Triggered(const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    TArray<FMythicTriggerSpec> Triggers;

    virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                 const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) override;

    virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                            const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) override;

    // Chance and internal cooldown in one decision, so the gate is testable without a world.
    static bool ShouldProc(float ResolvedChance, float InternalCooldown, double Now, double LastFireTime, float Roll01);

    // Rolled chance for a clause, falling back to its authored Chance when the granting source rolled nothing.
    float ResolveChance(const FMythicTriggerSpec &Spec) const;

    // The actor a clause acts on, given the event that fired it.
    static AActor *ResolveTarget(const FMythicTriggerSpec &Spec, const FGameplayEventData *Payload, AActor *Owner);

protected:
    // Payload-bound args follow the delegate's own, so EventTag comes second.
    void HandleTriggerEvent(const FGameplayEventData *Payload, FGameplayTag EventTag);

private:
    TMap<FGameplayTag, FDelegateHandle> BoundEvents;

    // Keyed by index into Triggers, so two clauses on the same event keep separate cooldowns.
    TMap<int32, double> LastFireTimes;
};
