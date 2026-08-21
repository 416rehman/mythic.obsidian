
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
 * Everything a clause needs to be true before it may roll. Every field is optional: an empty query and a full
 * 0..1 health window mean "no gate", so a clause with a default condition behaves exactly as one without.
 */
USTRUCT(BlueprintType)
struct MYTHIC_API FMythicTriggerCondition {
    GENERATED_BODY()

    /**
     * World state the clause needs, from the only three axes the environment publishes: Environment.Weather.*,
     * Environment.Time.* and Environment.Season.*. Matches the tag or any child, so Environment.Weather gates on
     * any weather at all. Same shape as FMythicWeatherDamageMod::WeatherTag. Invalid = no world gate.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition", meta = (Categories = "Environment"))
    FGameplayTag RequiredWorldTag;

    /**
      * Tag the event itself must carry. GAS.Event.Proficiency.Gained carries the proficiency's track tag, which is
      * how a talent keys off fishing rather than mining; Proficiency alone keys off any kind of work. Matches the
      * tag or any child. Invalid = any event of that type.
      */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition")
    FGameplayTag RequiredEventTag;

    // Matched against the tags the ability's owner currently holds.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition")
    FGameplayTagQuery SourceQuery;

    // Matched against the tags the other party currently holds.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition")
    FGameplayTagQuery TargetQuery;

    // Health fraction the owner must be inside. 0..1 is no gate; 0..0.5 is "below half".
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SourceHealthMin = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float SourceHealthMax = 1.0f;

    // Health fraction the other party must be inside. 0..0.2 is "already dying"; 1..1 is "unwounded".
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TargetHealthMin = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Condition", meta = (ClampMin = "0.0", ClampMax = "1.0"))
    float TargetHealthMax = 1.0f;
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

    // Status handed to the status registry when the clause lands. Optional if the clause applies an effect.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger", meta = (Categories = "Status.Type"))
    FGameplayTag StatusToApply;

    /**
     * Effect applied when the clause lands — a heal, a buff, anything a status cannot express. Optional if the
     * clause applies a status.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    TSubclassOf<UGameplayEffect> EffectToApply;

    /**
     * SetByCaller tag the effect reads its magnitude from, and the key the granting item rolled it under — one
     * tag for both, so an authored effect and its rolled value cannot drift apart.
     */
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    FGameplayTag MagnitudeParameter;

    // Magnitude used when MagnitudeParameter resolves no roll.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    float Magnitude = 0.0f;

    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    EMythicTriggerTarget Target = EMythicTriggerTarget::Other;

    // Seconds before this clause may fire again, so attack speed alone cannot carry a proc build. 0 = no limit.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger", meta = (ClampMin = "0.0"))
    float InternalCooldown = 0.0f;

    // What must be true before the clause rolls at all. Defaults to no gate.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Trigger")
    FMythicTriggerCondition Condition;
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

    // Value the granting source rolled under Parameter, or Fallback when it rolled nothing under that tag.
    float ResolveRolledValue(const FGameplayTag &Parameter, float Fallback) const;

    // True when the clause would do something. A clause with neither a status nor an effect is inert.
    static bool HasPayload(const FMythicTriggerSpec &Spec);

    // The actor a clause acts on, given the event that fired it.
    static AActor *ResolveTarget(const FMythicTriggerSpec &Spec, const FGameplayEventData *Payload, AActor *Owner);

    // Whether a clause's gate is open. Pure, so every world and health combination is testable without a world.
    static bool PassesCondition(const FMythicTriggerCondition &Condition, const FGameplayTagContainer &WorldTags,
                                const FGameplayTagContainer &EventTags, const FGameplayTagContainer &SourceTags,
                                const FGameplayTagContainer &TargetTags, float SourceHealthFraction,
                                float TargetHealthFraction);

    // Health as a 0..1 fraction. Returns 1 for anything with no health, so a "below half" gate stays shut on it.
    static float GetHealthFraction(const AActor *Actor);

protected:
    // Applies the clause's effect to the resolved target, with its rolled magnitude set by caller.
    bool ApplyClauseEffect(const FMythicTriggerSpec &Spec, AActor *Target, AActor *Owner) const;

    // Payload-bound args follow the delegate's own, so EventTag comes second.
    void HandleTriggerEvent(const FGameplayEventData *Payload, FGameplayTag EventTag);

private:
    TMap<FGameplayTag, FDelegateHandle> BoundEvents;

    // Keyed by index into Triggers, so two clauses on the same event keep separate cooldowns.
    TMap<int32, double> LastFireTimes;
};
