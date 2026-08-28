#pragma once
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTagContainer.h"
#include "MythicDamageContainer.generated.h"

class URPGAbilitySystemComponent;
class UGameplayEffect;
class URPGTargetType;

/**
 * Exact typed identity for a destructible collision target.
 *
 * Component implementations own their FHitResult instance index; actor implementations own one actor-wide
 * identity. Unrelated sibling components are never inferred as the target.
 */
struct MYTHIC_API FMythicDestructibleTargetIdentity {
    const UObject *TargetObject = nullptr;
    int32 InstanceIndex = INDEX_NONE;
    bool bPerInstance = false;

    bool IsValid() const { return TargetObject != nullptr; }

    /** Resolves an exact hit component first, then an exact actor implementation, and otherwise fails closed. */
    static FMythicDestructibleTargetIdentity Resolve(const FHitResult &Hit);

    /** Resolves only an actor-level implementation for target data that carries no component geometry. */
    static FMythicDestructibleTargetIdentity ResolveActor(const AActor *Actor);
};


USTRUCT(BlueprintType, Blueprintable)
struct FMythicDamageContainer {
    GENERATED_BODY()

    FMythicDamageContainer() {}
    /**
     * Effect applied to the attacker to calculate the hit from live GAS attributes, such as writing TotalDamage.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MythicDamageContainer,
              meta = (ToolTip = "Effect applied to the attacker to calculate the hit from live GAS attributes."))
    TSubclassOf<UGameplayEffect> DamageCalculationEffect;

    /**
     * Effect applied once to each resolved target to consume the calculated damage, such as subtracting TotalDamage
     * from Health.
     */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MythicDamageContainer,
              meta = (ToolTip = "Effect applied once to each resolved target to consume the calculated damage."))
    TSubclassOf<UGameplayEffect> DamageApplicationEffect;
};


USTRUCT(BlueprintType, Blueprintable)
struct FMythicDamageContainerSpec {
    GENERATED_BODY()

    FMythicDamageContainerSpec() {}

    /** Effect context shared by the calculation and per-target application specs for this damage dispatch. */
    UPROPERTY(BlueprintReadOnly, Category = GameplayEffectContainer,
              meta = (ToolTip = "Effect context shared by every Gameplay Effect spec in this damage dispatch."))
    FGameplayEffectContextHandle EffectContextHandle;

    /** Canonical living-actor targets that receive the damage-application spec once each. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer,
              meta = (ToolTip = "Canonical living-actor targets that receive the damage-application spec once each."))
    FGameplayAbilityTargetDataHandle TargetsHandle;

    /** Exact actor or component-instance hit data routed to destructible-world handling. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer,
              meta = (ToolTip = "Exact destructible hits routed to world handling; component hits retain their component, instance Item, and geometry."))
    FGameplayAbilityTargetDataHandle DestructibleTargetsHandle;

    /** Source-side spec that calculates this hit from the attacker's captured GAS attributes. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer,
              meta = (ToolTip = "Source-side spec that calculates this hit from the attacker's captured GAS attributes."))
    FGameplayEffectSpecHandle DamageCalculationEffectSpec;

    /** Per-target spec that consumes the calculated damage and emits target-side hit feedback. */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer,
              meta = (ToolTip = "Per-target spec that consumes the calculated damage and emits target-side hit feedback."))
    FGameplayEffectSpecHandle DamageApplicationEffectSpec;

    /**
     * Adds exact hit data and actor-only data without inferring destructible sibling components. Component-backed
     * destructibles must arrive as FHitResult entries so their component, instance Item, and geometry survive.
     */
    void AddTargets(const TArray<FHitResult> &HitResults, const TArray<AActor *> &TargetActors);
};
