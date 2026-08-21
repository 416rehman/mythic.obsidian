
#include "MythicGA_Triggered.h"

#include "AbilitySystemComponent.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "GAS/Abilities/MythicAbilityRollSource.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"

#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "Mythic.h"

UMythicGA_Triggered::UMythicGA_Triggered(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
    ActivationPolicy = EMythicAbilityActivationPolicy::OnSpawn;
    // Procs decide loot-affecting outcomes, so they resolve on the server only and are never predicted.
    NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::ServerOnly;
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

bool UMythicGA_Triggered::ShouldProc(float ResolvedChance, float InternalCooldown, double Now, double LastFireTime, float Roll01) {
    if (InternalCooldown > 0.0f && LastFireTime > 0.0 && (Now - LastFireTime) < InternalCooldown) {
        return false;
    }
    return MythicCombat::RollSucceeds(MythicCombat::ClampProbability(ResolvedChance), Roll01);
}

float UMythicGA_Triggered::ResolveChance(const FMythicTriggerSpec &Spec) const {
    if (Spec.ChanceParameter.IsValid()) {
        if (const UObject *Source = GetCurrentSourceObject()) {
            if (const IMythicAbilityRollSource *RollSource = Cast<IMythicAbilityRollSource>(Source)) {
                float Rolled = 0.0f;
                if (RollSource->GetRolledAbilityValue(GetCurrentAbilitySpecHandle(), Spec.ChanceParameter, Rolled)) {
                    return Rolled;
                }
            }
        }
    }
    return Spec.Chance;
}

bool UMythicGA_Triggered::PassesCondition(const FMythicTriggerCondition &Condition, const FGameplayTagContainer &WorldTags,
                                         const FGameplayTagContainer &SourceTags, const FGameplayTagContainer &TargetTags,
                                         float SourceHealthFraction, float TargetHealthFraction) {
    if (Condition.RequiredWorldTag.IsValid() && !WorldTags.HasTag(Condition.RequiredWorldTag)) {
        return false;
    }
    if (!Condition.SourceQuery.IsEmpty() && !Condition.SourceQuery.Matches(SourceTags)) {
        return false;
    }
    if (!Condition.TargetQuery.IsEmpty() && !Condition.TargetQuery.Matches(TargetTags)) {
        return false;
    }
    if (SourceHealthFraction < Condition.SourceHealthMin || SourceHealthFraction > Condition.SourceHealthMax) {
        return false;
    }
    if (TargetHealthFraction < Condition.TargetHealthMin || TargetHealthFraction > Condition.TargetHealthMax) {
        return false;
    }
    return true;
}

float UMythicGA_Triggered::GetHealthFraction(const AActor *Actor) {
    const UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Actor);
    const UMythicAttributeSet_Life *Life = ASC ? ASC->GetSet<UMythicAttributeSet_Life>() : nullptr;
    if (!Life) {
        return 1.0f;
    }
    const float Max = Life->GetMaxHealth();
    if (Max <= 0.0f) {
        return 1.0f;
    }
    return FMath::Clamp(Life->GetHealth() / Max, 0.0f, 1.0f);
}

AActor *UMythicGA_Triggered::ResolveTarget(const FMythicTriggerSpec &Spec, const FGameplayEventData *Payload, AActor *Owner) {
    if (Spec.Target == EMythicTriggerTarget::Self) {
        return Owner;
    }
    // Both damage events put the other party in Target: the victim when we dealt it, the attacker when we took it.
    return Payload ? const_cast<AActor *>(Payload->Target.Get()) : nullptr;
}

void UMythicGA_Triggered::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                          const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UAbilitySystemComponent *ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr;
    if (!ASC) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // One binding per distinct event, however many clauses share it.
    for (const FMythicTriggerSpec &Spec : Triggers) {
        if (!Spec.TriggerEvent.IsValid() || BoundEvents.Contains(Spec.TriggerEvent)) {
            continue;
        }
        const FGameplayTag EventTag = Spec.TriggerEvent;
        const FDelegateHandle Bound = ASC->GenericGameplayEventCallbacks.FindOrAdd(EventTag).AddUObject(
            this, &UMythicGA_Triggered::HandleTriggerEvent, EventTag);
        BoundEvents.Add(EventTag, Bound);
    }

    // No EndAbility: this stays active for as long as the talent is equipped.
}

void UMythicGA_Triggered::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                     const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled) {
    if (UAbilitySystemComponent *ASC = ActorInfo ? ActorInfo->AbilitySystemComponent.Get() : nullptr) {
        for (const TPair<FGameplayTag, FDelegateHandle> &Bound : BoundEvents) {
            if (FGameplayEventMulticastDelegate *Delegate = ASC->GenericGameplayEventCallbacks.Find(Bound.Key)) {
                Delegate->Remove(Bound.Value);
            }
        }
    }
    BoundEvents.Empty();
    LastFireTimes.Empty();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UMythicGA_Triggered::HandleTriggerEvent(const FGameplayEventData *Payload, FGameplayTag EventTag) {
    AActor *Owner = GetAvatarActorFromActorInfo();
    if (!Owner || !Owner->HasAuthority()) {
        return;
    }

    const UWorld *World = Owner->GetWorld();
    const double Now = World ? World->GetTimeSeconds() : 0.0;

    // Exactly the three axes the environment subsystem publishes as tags. Gathered once, not per clause.
    FGameplayTagContainer WorldTags;
    if (const UGameInstance *GI = World ? World->GetGameInstance() : nullptr) {
        if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
            WorldTags.AddTag(Env->GetWeather());
            WorldTags.AddTag(Env->GetDayTimeTag());
            WorldTags.AddTag(Env->GetSeasonTag());
        }
    }

    FGameplayTagContainer SourceTags;
    if (const UAbilitySystemComponent *OwnerASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Owner)) {
        OwnerASC->GetOwnedGameplayTags(SourceTags);
    }
    const float SourceHealth = GetHealthFraction(Owner);

    for (int32 Index = 0; Index < Triggers.Num(); ++Index) {
        const FMythicTriggerSpec &Spec = Triggers[Index];
        if (Spec.TriggerEvent != EventTag || !Spec.StatusToApply.IsValid()) {
            continue;
        }

        AActor *Target = ResolveTarget(Spec, Payload, Owner);
        if (!Target) {
            continue;
        }

        // Gated before the roll, so a clause whose condition is shut does not burn its chance.
        FGameplayTagContainer TargetTags;
        if (const UAbilitySystemComponent *TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target)) {
            TargetASC->GetOwnedGameplayTags(TargetTags);
        }
        if (!PassesCondition(Spec.Condition, WorldTags, SourceTags, TargetTags, SourceHealth, GetHealthFraction(Target))) {
            continue;
        }

        const double *LastFire = LastFireTimes.Find(Index);
        if (!ShouldProc(ResolveChance(Spec), Spec.InternalCooldown, Now, LastFire ? *LastFire : 0.0, FMath::FRand())) {
            continue;
        }

        if (UMythicStatusRegistry::ApplyStatusToActor(Target, Spec.StatusToApply, Owner)) {
            LastFireTimes.Add(Index, Now);
        }
    }
}
