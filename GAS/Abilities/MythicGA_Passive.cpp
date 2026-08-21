
#include "GAS/Abilities/MythicGA_Passive.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Actor.h"
#include "Mythic.h"

UMythicGA_Passive::UMythicGA_Passive(const FObjectInitializer &ObjectInitializer) : Super(ObjectInitializer) {
    ActivationPolicy = EMythicAbilityActivationPolicy::OnSpawn;
    // A standing bonus decides loot-affecting outcomes, so it resolves on the server only and is never predicted.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UMythicGA_Passive::HasPayload(const FMythicPassiveClause &Clause) {
    return Clause.EffectToApply != nullptr;
}

void UMythicGA_Passive::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                        const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    AActor *Owner = GetAvatarActorFromActorInfo();
    UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    if (!Owner || !Owner->HasAuthority() || !ASC) {
        return;
    }

    for (const FMythicPassiveClause &Clause : Passives) {
        if (!HasPayload(Clause)) {
            UE_LOG(Myth, Warning, TEXT("%s: a passive clause has no effect and does nothing"), *GetName());
            continue;
        }

        FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
        Context.AddInstigator(Owner, Owner);

        const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(Clause.EffectToApply, 1.0f, Context);
        if (!Spec.IsValid()) {
            continue;
        }
        if (Clause.MagnitudeParameter.IsValid()) {
            Spec.Data->SetSetByCallerMagnitude(Clause.MagnitudeParameter, ResolveRolledValue(Clause.MagnitudeParameter, Clause.Magnitude));
        }

        const FActiveGameplayEffectHandle Applied = ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
        if (Applied.IsValid()) {
            AppliedEffects.Add(Applied);
        }
    }
}

void UMythicGA_Passive::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                   const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) {
    if (UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo()) {
        for (const FActiveGameplayEffectHandle &Applied : AppliedEffects) {
            ASC->RemoveActiveGameplayEffect(Applied);
        }
    }
    AppliedEffects.Reset();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}
