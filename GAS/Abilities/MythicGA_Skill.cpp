
#include "MythicGA_Skill.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/Tasks/AbilityTask_ApplyRootMotionMoveToForce.h"
#include "Abilities/Tasks/AbilityTask_PlayMontageAndWait.h"
#include "Abilities/Tasks/AbilityTask_WaitGameplayEvent.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/RootMotionSource.h"

#include "GAS/Abilities/MythicGA_Triggered.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Offense.h"
#include "GAS/Effects/MythicStatusRegistry.h"
#include "GAS/Executions/MythicCombatRoll.h"
#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/MythicTags_GAS.h"
#include "Settings/MythicDeveloperSettings.h"

float FMythicSkillTargeting::CosineHalfAngle(float AngleDegrees) {
    // A full turn takes everything, and nothing wider than a full turn exists.
    const float Width = FMath::Clamp(AngleDegrees, 0.0f, 360.0f);
    return FMath::Cos(FMath::DegreesToRadians(Width * 0.5f));
}

FVector FMythicSkillTargeting::ResolveOrigin(const FMythicSkillShape &Shape, const FVector &ActorLocation, const FVector &Forward) {
    FVector Direction = Forward;
    if (!Direction.Normalize()) {
        return ActorLocation;
    }
    return ActorLocation + Direction * Shape.ForwardOffset;
}

bool FMythicSkillTargeting::IsInside(const FMythicSkillShape &Shape, const FVector &Origin, const FVector &Forward, const FVector &Point) {
    if (Shape.Radius <= 0.0f) {
        return false;
    }

    const FVector ToPoint = Point - Origin;
    if (ToPoint.SizeSquared() > Shape.Radius * Shape.Radius) {
        return false;
    }
    if (Shape.Shape == EMythicSkillShape::Sphere || Shape.Shape == EMythicSkillShape::Single) {
        return true;
    }

    FVector Direction = ToPoint;
    FVector Facing = Forward;
    if (Shape.Shape == EMythicSkillShape::Arc) {
        Direction.Z = 0.0f;
        Facing.Z = 0.0f;
    }
    // A target standing on the caster, or a caster with no facing, has no angle to fail.
    if (!Direction.Normalize() || !Facing.Normalize()) {
        return true;
    }

    return FVector::DotProduct(Facing, Direction) >= CosineHalfAngle(Shape.AngleDegrees);
}

void FMythicSkillTargeting::SelectTargets(const FMythicSkillShape &Shape, const FVector &Origin, const FVector &Forward,
                                          const TArray<FVector> &Points, TArray<int32> &OutSelected) {
    OutSelected.Reset();

    for (int32 Index = 0; Index < Points.Num(); ++Index) {
        if (IsInside(Shape, Origin, Forward, Points[Index])) {
            OutSelected.Add(Index);
        }
    }

    OutSelected.Sort([&Points, &Origin](const int32 &A, const int32 &B) {
        return FVector::DistSquared(Points[A], Origin) < FVector::DistSquared(Points[B], Origin);
    });

    if (Shape.MaxTargets > 0 && OutSelected.Num() > Shape.MaxTargets) {
        OutSelected.SetNum(Shape.MaxTargets);
    }
}

float UMythicGA_Skill::ScaleRadius(float Authored, float Bonus) {
    return FMath::Max(0.0f, Authored + Bonus);
}

int32 UMythicGA_Skill::ScaleTargetCount(int32 Authored, float Bonus) {
    if (Authored <= 0) {
        return 0;
    }
    return FMath::Max(1, Authored + FMath::RoundToInt(Bonus));
}

float UMythicGA_Skill::ScaleDuration(float Authored, float Bonus) {
    // Floors at a tick rather than 0. A reduction big enough to zero the duration must still read as "very short";
    // returning 0 would mean "instant" to GAS and "leave it alone" to the caller, both of which are longer, not
    // shorter, than what the stat asked for.
    return Authored > 0.0f ? FMath::Max(MinScaledDuration, Authored + Bonus) : 0.0f;
}

float UMythicGA_Skill::GetSkillDurationBonus() const {
    const UMythicAttributeSet_Offense *Offense = GetOffenseSet();
    return Offense ? Offense->GetSkillDurationBonus() : 0.0f;
}

FVector UMythicGA_Skill::ComputeMovementDestination(const FVector &Start, const FVector &Forward, float Distance) {
    FVector Direction = Forward;
    if (!Direction.Normalize()) {
        return Start;
    }
    return Start + Direction * FMath::Max(0.0f, Distance);
}

const UMythicAttributeSet_Offense *UMythicGA_Skill::GetOffenseSet() const {
    const UAbilitySystemComponent *ASC = GetAbilitySystemComponentFromActorInfo();
    return ASC ? ASC->GetSet<UMythicAttributeSet_Offense>() : nullptr;
}

FMythicSkillShape UMythicGA_Skill::ResolveShape() const {
    FMythicSkillShape Resolved = Shape;

    const UMythicAttributeSet_Offense *Offense = GetOffenseSet();
    const float RadiusBonus = Offense ? Offense->GetSkillRadiusBonus() : 0.0f;
    const float CountBonus = Offense ? Offense->GetSkillTargetCountBonus() : 0.0f;

    Resolved.Radius = ScaleRadius(Shape.Radius, RadiusBonus);
    // A single-target skill is one target by definition, whatever the asset typed in MaxTargets. Count gear is
    // still allowed to add to that, which is what turns a thrust into a piercing thrust.
    Resolved.MaxTargets = ScaleTargetCount(Shape.Shape == EMythicSkillShape::Single ? 1 : Shape.MaxTargets, CountBonus);

    return Resolved;
}

float UMythicGA_Skill::ResolveSelfEffectDuration() const {
    return ScaleDuration(SelfEffectDuration, GetSkillDurationBonus());
}

void UMythicGA_Skill::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo *ActorInfo,
                                      const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData *TriggerEventData) {
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    if (!CommitAbility(Handle, ActorInfo, ActivationInfo)) {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    UAbilityTask_ApplyRootMotionMoveToForce *Dash = StartMovement();
    const bool bOutlivesActivation = (Montage != nullptr) || (Dash != nullptr);

    // An impact event with nothing to outlive the activation could never arrive, because the ability would have
    // ended first. Such a skill hits early rather than never.
    if (ImpactEventTag.IsValid() && bOutlivesActivation) {
        UAbilityTask_WaitGameplayEvent *Impact = UAbilityTask_WaitGameplayEvent::WaitGameplayEvent(this, ImpactEventTag, nullptr, false, true);
        Impact->EventReceived.AddDynamic(this, &UMythicGA_Skill::OnImpact);
        Impact->ReadyForActivation();
    }
    else {
        ExecutePayload();
    }

    if (Montage) {
        UAbilityTask_PlayMontageAndWait *Play = UAbilityTask_PlayMontageAndWait::CreatePlayMontageAndWaitProxy(
            this, NAME_None, Montage, GetClampedAttackSpeedPlayRate());
        // Not OnBlendOut: it fires as the blend starts, and ending there stops the montage mid-blend.
        Play->OnCompleted.AddDynamic(this, &UMythicGA_Skill::OnSkillFinished);
        Play->OnInterrupted.AddDynamic(this, &UMythicGA_Skill::OnSkillFinished);
        Play->OnCancelled.AddDynamic(this, &UMythicGA_Skill::OnSkillFinished);
        Play->ReadyForActivation();
        return;
    }

    // Ending the ability tears its tasks down, so a dash with no montage has to outlive the activation itself.
    if (Dash) {
        Dash->OnTimedOut.AddDynamic(this, &UMythicGA_Skill::OnSkillFinished);
        Dash->OnTimedOutAndDestinationReached.AddDynamic(this, &UMythicGA_Skill::OnSkillFinished);
        return;
    }

    EndAbility(Handle, ActorInfo, ActivationInfo, true, false);
}

void UMythicGA_Skill::OnImpact(FGameplayEventData Payload) {
    ExecutePayload();
}

void UMythicGA_Skill::OnSkillFinished() {
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}

UAbilityTask_ApplyRootMotionMoveToForce *UMythicGA_Skill::StartMovement() {
    ACharacter *Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character || Movement == EMythicSkillMovement::None || MovementDistance <= 0.0f) {
        return nullptr;
    }

    const FVector Destination = ComputeMovementDestination(Character->GetActorLocation(), Character->GetActorForwardVector(), MovementDistance);

    if (Movement == EMythicSkillMovement::Teleport) {
        // No prediction on a blink: a client that guesses a destination the server refuses snaps back through
        // whatever it blinked past. The dash below is predicted because the movement component replays it.
        if (HasAuthority(&CurrentActivationInfo)) {
            Character->TeleportTo(Destination, Character->GetActorRotation());
        }
        return nullptr;
    }

    UAbilityTask_ApplyRootMotionMoveToForce *Dash = UAbilityTask_ApplyRootMotionMoveToForce::ApplyRootMotionMoveToForce(
        this, FName("SkillDash"), Destination, FMath::Max(0.01f, MovementDuration), false, MOVE_Walking, false, nullptr,
        ERootMotionFinishVelocityMode::ClampVelocity, FVector::ZeroVector, 0.0f);
    if (Dash) {
        Dash->ReadyForActivation();
    }
    return Dash;
}

void UMythicGA_Skill::ExecutePayload() {
    // The server decides every outcome. It also has to: a payload run from a task callback is outside the
    // activation's prediction window, so a predicted copy of it would never be rolled back.
    if (!HasAuthority(&CurrentActivationInfo)) {
        return;
    }

    AActor *Owner = GetAvatarActorFromActorInfo();
    if (!Owner) {
        return;
    }

    ApplySelfEffect();

    const FMythicSkillShape Scaled = ResolveShape();
    const FVector Forward = Owner->GetActorForwardVector();
    const FVector Origin = FMythicSkillTargeting::ResolveOrigin(Scaled, Owner->GetActorLocation(), Forward);

    TArray<FHitResult> Hits;
    GatherTargets(Scaled, Origin, Forward, Hits);
    if (Hits.IsEmpty()) {
        return;
    }

    if (HasDamage()) {
        FMythicDamageContainerSpec Spec = MakeDamageContainerSpec(Damage);
        MarkSpecAsSkill(Spec);
        AddTargetsToDamageContainerSpec(Spec, Hits, TArray<AActor *>());
        ApplyDamageContainerSpec(Spec);
    }

    if (StatusToApply.IsValid()) {
        ApplyStatus(Hits, Owner);
    }
}

void UMythicGA_Skill::GatherTargets(const FMythicSkillShape &Scaled, const FVector &Origin, const FVector &Forward,
                                    TArray<FHitResult> &OutHits) const {
    AActor *Owner = GetAvatarActorFromActorInfo();
    const UWorld *World = Owner ? Owner->GetWorld() : nullptr;
    if (!World || Scaled.Radius <= 0.0f) {
        return;
    }

    TArray<FHitResult> Found;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicSkillShape), false, Owner);
    World->SweepMultiByChannel(Found, Origin, Origin, FQuat::Identity, ECC_Pawn,
                               FCollisionShape::MakeSphere(Scaled.Radius), Params);

    const APawn *OwnerPawn = Cast<APawn>(Owner);
    const bool bSourceIsPlayer = OwnerPawn && OwnerPawn->IsPlayerControlled();
    const bool bFriendlyFire = GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;

    TSet<const AActor *> Seen;
    TArray<FHitResult> Candidates;
    TArray<FVector> Points;
    for (const FHitResult &Hit : Found) {
        const AActor *Victim = Hit.GetActor();
        if (!Victim || Victim == Owner || Seen.Contains(Victim)) {
            continue;
        }
        if (!UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Victim)) {
            continue;
        }
        // The same rule the damage execution applies, so a skill cannot reach what a hit would be refused.
        const APawn *VictimPawn = Cast<const APawn>(Victim);
        const bool bTargetIsPlayer = VictimPawn && VictimPawn->IsPlayerControlled();
        if (UMythicDamageApplication::ShouldNegateFriendlyFire(bSourceIsPlayer, bTargetIsPlayer, false, bFriendlyFire)) {
            continue;
        }
        Seen.Add(Victim);
        Candidates.Add(Hit);
        Points.Add(Victim->GetActorLocation());
    }

    TArray<int32> Selected;
    FMythicSkillTargeting::SelectTargets(Scaled, Origin, Forward, Points, Selected);

    OutHits.Reserve(Selected.Num());
    for (const int32 Index : Selected) {
        OutHits.Add(Candidates[Index]);
    }
}

void UMythicGA_Skill::ApplySelfEffect() {
    if (!SelfEffect) {
        return;
    }

    const FGameplayEffectSpecHandle SpecHandle = MakeOutgoingGameplayEffectSpec(SelfEffect, GetAbilityLevel());
    if (!SpecHandle.IsValid() || !SpecHandle.Data.IsValid()) {
        return;
    }

    // An authored 0 means "whatever the effect says", so the base to add gear to is the spec's own duration, not
    // zero. Reading it back is what keeps a +2s bonus from replacing a 15s stance with a 2s one.
    const float Authored = SelfEffectDuration > 0.0f ? SelfEffectDuration : SpecHandle.Data->GetDuration();
    if (Authored > 0.0f) {
        SpecHandle.Data->SetDuration(ScaleDuration(Authored, GetSkillDurationBonus()), true);
    }

    K2_ApplyGameplayEffectSpecToOwner(SpecHandle);
}

void UMythicGA_Skill::ApplyStatus(const TArray<FHitResult> &Hits, AActor *Instigator) const {
    const float Chance = MythicCombat::ClampProbability(StatusChance);

    for (const FHitResult &Hit : Hits) {
        AActor *Target = Hit.GetActor();
        if (!Target) {
            continue;
        }
        if (!MythicCombat::RollSucceeds(Chance, FMath::FRand())) {
            continue;
        }
        // The resistance gate a weapon proc passes through, so a skill cannot ignore what a proc respects.
        const float Survives = UMythicGA_Triggered::SurviveChanceFromResistance(
            UMythicGA_Triggered::GetStatusResistance(Target, StatusToApply));
        if (!MythicCombat::RollSucceeds(Survives, FMath::FRand())) {
            continue;
        }
        UMythicStatusRegistry::ApplyStatusToActor(Target, StatusToApply, Instigator);
    }
}

void UMythicGA_Skill::MarkSpecAsSkill(FMythicDamageContainerSpec &Spec) {
    const FGameplayTagContainer SkillTag(GAS_ABILITY_TYPE_SKILL);

    if (Spec.DamageApplicationEffectSpec.IsValid() && Spec.DamageApplicationEffectSpec.Data.IsValid()) {
        Spec.DamageApplicationEffectSpec.Data->AppendDynamicAssetTags(SkillTag);
    }
    if (Spec.DamageCalculationEffectSpec.IsValid() && Spec.DamageCalculationEffectSpec.Data.IsValid()) {
        Spec.DamageCalculationEffectSpec.Data->AppendDynamicAssetTags(SkillTag);
    }
}
