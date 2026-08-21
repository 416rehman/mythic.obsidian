#pragma once
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "GameplayTagContainer.h"
#include "MythicDamageContainer.generated.h"

class URPGAbilitySystemComponent;
class UGameplayEffect;
class URPGTargetType;


USTRUCT(BlueprintType)
struct FMythicStatusEffectMapping {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> BleedEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> BurnEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> PoisonEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> StunEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> SlowEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> FreezeEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> WeakenEffect;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Status Effects")
    TSubclassOf<UGameplayEffect> TerrifyEffect;
};

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

    // Optional per-status debuff effects applied to each target whose corresponding rolled flag survives its
    // resistance gate. All null by default (no debuff applied until a designer assigns one).
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = MythicDamageContainer)
    FMythicStatusEffectMapping StatusEffects;
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

    /** Per-status debuff effects to apply per target (threaded from the source FMythicDamageContainer). */
    UPROPERTY(BlueprintReadOnly, Category = GameplayEffectContainer)
    FMythicStatusEffectMapping StatusEffects;

    bool IsDestructible(AActor *Actor);

    void AddTargets(const TArray<FHitResult> &HitResults, const TArray<AActor *> &TargetActors);
};
