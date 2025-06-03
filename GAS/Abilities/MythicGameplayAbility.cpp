// 


#include "MythicGameplayAbility.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Mythic.h"
#include "MythicDamageContainer.h"
#include "GAS/MythicTags_GAS.h"
#include "Itemization/Inventory/Fragments/Passive/TalentFragment.h"

class UMythicTargetType;

void UMythicGameplayAbility::OnAvatarSet(const FGameplayAbilityActorInfo *ActorInfo, const FGameplayAbilitySpec &Spec) {
    Super::OnAvatarSet(ActorInfo, Spec);

    if (bIsPassive) {
        // Give the ability to the actor
        ActorInfo->AbilitySystemComponent.Get()->TryActivateAbility(Spec.Handle, true);
    }
}


FMythicDamageContainerSpec UMythicGameplayAbility::MakeDamageContainerSpec(const FMythicDamageContainer &Container, int32 OverrideGameplayLevel) {
    // First figure out our actor info
    FMythicDamageContainerSpec ReturnSpec;

    // If we don't have an override level, use the default on the ability itself
    if (OverrideGameplayLevel == INDEX_NONE) {
        OverrideGameplayLevel = OverrideGameplayLevel = this->GetAbilityLevel(); //OwningASC->GetDefaultAbilityLevel();
    }
    auto OwningASC = GetAbilitySystemComponentFromActorInfo();
    ReturnSpec.EffectContextHandle = OwningASC->MakeEffectContext();

    // Add the damage calculation effect
    if (Container.DamageCalculationEffect) {
        FGameplayEffectSpecHandle DamageCalculationSpecHandle = OwningASC->MakeOutgoingSpec(Container.DamageCalculationEffect, OverrideGameplayLevel,
                                                                                            ReturnSpec.EffectContextHandle);
        ReturnSpec.DamageCalculationEffectSpec = DamageCalculationSpecHandle;
    }
    else {
        UE_LOG(Mythic, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: Container.DamageCalculationEffect is null"));
    }

    // Add the damage application effect
    if (Container.DamageApplicationEffect) {
        FGameplayEffectSpecHandle DamageApplicationSpecHandle = OwningASC->MakeOutgoingSpec(Container.DamageApplicationEffect, OverrideGameplayLevel,
                                                                                            ReturnSpec.EffectContextHandle);
        ReturnSpec.DamageApplicationEffectSpec = DamageApplicationSpecHandle;
    }
    else {
        UE_LOG(Mythic, Error, TEXT("UMythicGameplayAbility::MakeDamageContainerSpec: Container.DamageApplicationEffect is null"));
    }

    return ReturnSpec;
}

void UMythicGameplayAbility::SendEvent(FGameplayAbilityTargetDataHandle TargetData, FGameplayEffectContextHandle EffectContextHandle, FGameplayTag EventTag) {
    ;
    auto SourceASC = GetAbilitySystemComponentFromActorInfo();
    FGameplayEventData EventData;
    EventData.Instigator = SourceASC->GetAvatarActor();
    EventData.TargetData = TargetData;
    EventData.ContextHandle = EffectContextHandle;
    auto activations = SourceASC->HandleGameplayEvent(EventTag, &EventData);
    UE_LOG(Mythic, Warning, TEXT("BeforeDamage event sent to source %d abilities"), activations);
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainerSpec(const FMythicDamageContainerSpec &ContainerSpec) {
    UAbilitySystemComponent* const AbilitySystemComponent = CurrentActorInfo->AbilitySystemComponent.Get();
    FScopedPredictionWindow NewScopedWindow(AbilitySystemComponent, true);
    
    TArray<FActiveGameplayEffectHandle> AllEffects;

    // 1. CREATE DAMAGE CONTEXT - This stage only sets the context like isCritical, isBlocked, etc. No damage is calculated yet.
    if (!ContainerSpec.DamageCalculationEffectSpec.IsValid()) {
        UE_LOG(Mythic, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageCalculationEffectSpec is null"));
    }
    auto CalculationEffectHandle = K2_ApplyGameplayEffectSpecToOwner(ContainerSpec.DamageCalculationEffectSpec);
    AllEffects.Push(CalculationEffectHandle);

    // 2. SEND DAMAGE PRE EVENT
    if (ContainerSpec.TargetsHandle.Num() > 0) {
        SendEvent(ContainerSpec.TargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_PRE);

        // 3. APPLY DAMAGE - This stage calculates the damage and applies it to the targets
        if (!ContainerSpec.DamageApplicationEffectSpec.IsValid()) {
            UE_LOG(Mythic, Error, TEXT("UMythicGameplayAbility::ApplyDamageContainerSpec: ContainerSpec.DamageApplicationEffectSpec is null"));
        }
        auto ApplicationEffectHandle = K2_ApplyGameplayEffectSpecToTarget(ContainerSpec.DamageApplicationEffectSpec, ContainerSpec.TargetsHandle);
        AllEffects.Append(ApplicationEffectHandle);
    }

    // 4. Handle destructibles
    if (ContainerSpec.DestructibleTargetsHandle.Num() > 0) {
        // 4.1. SEND DAMAGE PRE EVENT - No damage is calculated as part of this
        SendEvent(ContainerSpec.DestructibleTargetsHandle, ContainerSpec.EffectContextHandle, GAS_EVENT_DMG_DESTRUCTIBLE);
    }

    return AllEffects;
}

TArray<FActiveGameplayEffectHandle> UMythicGameplayAbility::ApplyDamageContainer(const FMythicDamageContainer &Container, const TArray<FHitResult> &HitResults,
                                                                                 const TArray<AActor *> &TargetActors,
                                                                                 int32 OverrideGameplayLevel) {
    // If no HitResults or TargetActors, gtfo
    if (TargetActors.IsEmpty() || HitResults.IsEmpty()) {
        UE_LOG(Mythic, Warning, TEXT("No Targets"));
    }

    FMythicDamageContainerSpec Spec = MakeDamageContainerSpec(Container, OverrideGameplayLevel);
    AddTargetsToDamageContainerSpec(Spec, HitResults, TargetActors);
    return ApplyDamageContainerSpec(Spec);
}

void UMythicGameplayAbility::AddTargetsToDamageContainerSpec(FMythicDamageContainerSpec &ContainerSpec, const TArray<FHitResult> &HitResults,
                                                             const TArray<AActor *> &TargetActors) {
    ContainerSpec.AddTargets(HitResults, TargetActors);
}
