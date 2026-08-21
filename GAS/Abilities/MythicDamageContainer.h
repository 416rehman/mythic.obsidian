#pragma once
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTagContainer.h"
#include "MythicDamageContainer.generated.h"

class URPGAbilitySystemComponent;
class UGameplayEffect;
class URPGTargetType;


USTRUCT(BlueprintType, Blueprintable)
struct FMythicDamageContainer {
    GENERATED_BODY()

    FMythicDamageContainer() {}
    // The DamageCalculationEffect is applied to the player to calculate the damage. I.e setting the TotalDamage attribute
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MythicDamageContainer)
    TSubclassOf<UGameplayEffect> DamageCalculationEffect;

    // The DamageApplicationEffect is applied to the target to apply the damage. I.e applying the damage from the TotalDamage attribute to the target's health
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MythicDamageContainer)
    TSubclassOf<UGameplayEffect> DamageApplicationEffect;
};


USTRUCT(BlueprintType, Blueprintable)
struct FMythicDamageContainerSpec {
    GENERATED_BODY()

    FMythicDamageContainerSpec() {}

    UPROPERTY(BlueprintReadOnly, Category = GameplayEffectContainer)
    FGameplayEffectContextHandle EffectContextHandle;

    /** Computed target data */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
    FGameplayAbilityTargetDataHandle TargetsHandle;

    /** DestructibleTargetsHandle */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
    FGameplayAbilityTargetDataHandle DestructibleTargetsHandle;

    /** Damage Context Effect Spec */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
    FGameplayEffectSpecHandle DamageCalculationEffectSpec;

    /** Damage Application Effect Spec */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = GameplayEffectContainer)
    FGameplayEffectSpecHandle DamageApplicationEffectSpec;

    bool IsDestructible(AActor *Actor);

    void AddTargets(const TArray<FHitResult> &HitResults, const TArray<AActor *> &TargetActors);
};
