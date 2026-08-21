
#include "MythicGA_Triggered.h"

#include "AbilitySystemComponent.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

#include "GAS/Abilities/MythicAbilityRollSource.h"
#include "AbilitySystemGlobals.h"
#include "Engine/GameInstance.h"

#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"

#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GAS/Effects/MythicStatusEffectDefinition.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "Settings/MythicDeveloperSettings.h"
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

bool UMythicGA_Triggered::HasPayload(const FMythicTriggerSpec &Spec) {
    return Spec.StatusToApply.IsValid() || Spec.EffectToApply != nullptr;
}

bool UMythicGA_Triggered::PassesCondition(const FMythicTriggerCondition &Condition, const FGameplayTagContainer &WorldTags,
                                         const FGameplayTagContainer &EventTags, const FGameplayTagContainer &SourceTags,
                                         const FGameplayTagContainer &TargetTags, float SourceHealthFraction,
                                         float TargetHealthFraction) {
    if (Condition.RequiredWorldTag.IsValid() && !WorldTags.HasTag(Condition.RequiredWorldTag)) {
        return false;
    }
    if (Condition.RequiredEventTag.IsValid() && !EventTags.HasTag(Condition.RequiredEventTag)) {
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

float UMythicGA_Triggered::SurviveChanceFromResistance(float Resistance) {
    return MythicCombat::ClampProbability(1.0f - Resistance);
}

float UMythicGA_Triggered::GetStatusResistance(const AActor *Target, const FGameplayTag &StatusType) {
    const UAbilitySystemComponent *ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    const UWorld *World = Target ? Target->GetWorld() : nullptr;
    const UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    const UMythicStatusRegistry *Registry = GI ? GI->GetSubsystem<UMythicStatusRegistry>() : nullptr;
    if (!ASC || !Registry) {
        return 0.0f;
    }
    const UMythicStatusEffectDefinition *Definition = Registry->FindStatus(StatusType);
    if (!Definition || !Definition->ResistanceAttribute.IsValid()) {
        return 0.0f;
    }
    bool bFound = false;
    return ASC->GetGameplayAttributeValue(Definition->ResistanceAttribute, bFound);
}

void UMythicGA_Triggered::LimitTargets(TArray<AActor *> &Targets, int32 MaxTargets) {
    if (MaxTargets > 0 && Targets.Num() > MaxTargets) {
        Targets.SetNum(MaxTargets);
    }
}

void UMythicGA_Triggered::GatherClauseTargets(const FMythicTriggerSpec &Spec, AActor *Origin, AActor *Owner, TArray<AActor *> &Out) const {
    const float Radius = ResolveRolledValue(Spec.RadiusParameter, Spec.Radius);
    if (Radius <= 0.0f) {
        Out.Add(Origin);
        return;
    }

    const UWorld *World = Origin ? Origin->GetWorld() : nullptr;
    if (!World) {
        return;
    }

    TArray<FOverlapResult> Overlaps;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicTriggerSweep), false, Owner);
    World->OverlapMultiByChannel(Overlaps, Origin->GetActorLocation(), FQuat::Identity, ECC_Pawn,
                                 FCollisionShape::MakeSphere(Radius), Params);

    const APawn *OwnerPawn = Cast<APawn>(Owner);
    const bool bSourceIsPlayer = OwnerPawn && OwnerPawn->IsPlayerControlled();
    const bool bFriendlyFire = GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;

    for (const FOverlapResult &Overlap : Overlaps) {
        AActor *Candidate = Overlap.GetActor();
        if (!Candidate || Candidate == Owner || Out.Contains(Candidate)) {
            continue;
        }
        if (!UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Candidate)) {
            continue;
        }
        // The same rule the damage execution applies, so a sweep cannot hit what a swing would not.
        const APawn *CandidatePawn = Cast<APawn>(Candidate);
        const bool bTargetIsPlayer = CandidatePawn && CandidatePawn->IsPlayerControlled();
        if (UMythicDamageApplication::ShouldNegateFriendlyFire(bSourceIsPlayer, bTargetIsPlayer, false, bFriendlyFire)) {
            continue;
        }
        Out.Add(Candidate);
    }

    const FVector Centre = Origin->GetActorLocation();
    Out.Sort([Centre](const AActor &A, const AActor &B) {
        return FVector::DistSquared(A.GetActorLocation(), Centre) < FVector::DistSquared(B.GetActorLocation(), Centre);
    });
    LimitTargets(Out, Spec.MaxTargets);
}

EMythicTriggerFacing UMythicGA_Triggered::ResolveFacing(const FVector &OtherForward, const FVector &OtherToOwner, float Threshold) {
    FVector Forward = OtherForward;
    FVector ToOwner = OtherToOwner;
    // Two actors on the same spot, or an actor with no orientation, have no arc to speak of.
    if (!Forward.Normalize() || !ToOwner.Normalize()) {
        return EMythicTriggerFacing::Any;
    }

    const float Dot = static_cast<float>(FVector::DotProduct(Forward, ToOwner));
    const float Bound = FMath::Clamp(Threshold, 0.0f, 1.0f);
    if (Dot >= Bound) {
        return EMythicTriggerFacing::Front;
    }
    if (Dot <= -Bound) {
        return EMythicTriggerFacing::Behind;
    }
    return EMythicTriggerFacing::Flank;
}

bool UMythicGA_Triggered::PassesFacing(EMythicTriggerFacing Required, EMythicTriggerFacing Actual) {
    if (Required == EMythicTriggerFacing::Any) {
        return true;
    }
    // An unknown arc fails a clause that asked for one, rather than passing by accident.
    return Actual == Required;
}

bool UMythicGA_Triggered::IsNthEvent(int32 EveryNth, int32 Count) {
    if (EveryNth <= 1) {
        return true;
    }
    return Count > 0 && (Count % EveryNth) == 0;
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
    EventCounts.Empty();

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

bool UMythicGA_Triggered::ApplyClauseEffect(const FMythicTriggerSpec &Spec, AActor *Target, AActor *Owner) const {
    UAbilitySystemComponent *OwnerASC = GetAbilitySystemComponentFromActorInfo();
    UAbilitySystemComponent *TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Target);
    if (!OwnerASC || !TargetASC || !Spec.EffectToApply) {
        return false;
    }

    FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
    Context.AddInstigator(Owner, Owner);

    const FGameplayEffectSpecHandle EffectSpec = OwnerASC->MakeOutgoingSpec(Spec.EffectToApply, 1.0f, Context);
    if (!EffectSpec.IsValid()) {
        return false;
    }
    if (Spec.MagnitudeParameter.IsValid()) {
        EffectSpec.Data->SetSetByCallerMagnitude(Spec.MagnitudeParameter,
                                                 ResolveRolledValue(Spec.MagnitudeParameter, Spec.Magnitude));
    }
    if (Spec.DurationParameter.IsValid()) {
        EffectSpec.Data->SetSetByCallerMagnitude(Spec.DurationParameter,
                                                 ResolveRolledValue(Spec.DurationParameter, Spec.Duration));
    }

    return OwnerASC->ApplyGameplayEffectSpecToTarget(*EffectSpec.Data.Get(), TargetASC).WasSuccessfullyApplied();
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
        if (Spec.TriggerEvent != EventTag || !HasPayload(Spec)) {
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
        if (Spec.Condition.RequiredFacing != EMythicTriggerFacing::Any) {
            const AActor *Other = Payload ? Payload->Target.Get() : nullptr;
            const EMythicTriggerFacing Actual = Other
                ? ResolveFacing(Other->GetActorForwardVector(), Owner->GetActorLocation() - Other->GetActorLocation(),
                                Spec.Condition.FacingThreshold)
                : EMythicTriggerFacing::Any;
            if (!PassesFacing(Spec.Condition.RequiredFacing, Actual)) {
                continue;
            }
        }

        const FGameplayTagContainer &EventTags = Payload ? Payload->InstigatorTags : FGameplayTagContainer::EmptyContainer;
        if (!PassesCondition(Spec.Condition, WorldTags, EventTags, SourceTags, TargetTags, SourceHealth,
                             GetHealthFraction(Target))) {
            continue;
        }

        // Counted after the gate, so a clause gated on snow counts strikes in snow rather than every swing.
        int32 &Count = EventCounts.FindOrAdd(Index);
        ++Count;
        if (!IsNthEvent(Spec.EveryNthEvent, Count)) {
            continue;
        }

        const double *LastFire = LastFireTimes.Find(Index);
        if (!ShouldProc(ResolveRolledValue(Spec.ChanceParameter, Spec.Chance), Spec.InternalCooldown, Now,
                        LastFire ? *LastFire : 0.0, FMath::FRand())) {
            continue;
        }

        TArray<AActor *> Recipients;
        GatherClauseTargets(Spec, Target, Owner, Recipients);

        bool bLanded = false;
        for (AActor *Recipient : Recipients) {
            if (Spec.StatusToApply.IsValid()) {
                const float Survives = SurviveChanceFromResistance(GetStatusResistance(Recipient, Spec.StatusToApply));
                if (MythicCombat::RollSucceeds(Survives, FMath::FRand())) {
                    bLanded |= UMythicStatusRegistry::ApplyStatusToActor(Recipient, Spec.StatusToApply, Owner);
                }
            }
            if (Spec.EffectToApply) {
                bLanded |= ApplyClauseEffect(Spec, Recipient, Owner);
            }
        }
        if (bLanded) {
            LastFireTimes.Add(Index, Now);
        }
    }
}
