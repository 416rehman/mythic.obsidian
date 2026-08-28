
#include "MythicAnimNotify_SphereOverlap.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Animation/ActiveMontageInstanceScope.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#include "GAS/Executions/MythicDamageApplication.h"
#include "GAS/Abilities/MythicWeaponAttackAbility.h"
#include "Resources/MythicResourceISM.h"
#include "Settings/MythicDeveloperSettings.h"

bool UMythicAnimNotify_SphereOverlap::IsRuntimeQueryConfigurationValid(
    const float Radius, const FVector &LocationOffset,
    const int32 TargetCap) {
    return FMath::IsFinite(Radius) && Radius > 0.0f
        && !LocationOffset.ContainsNaN() && TargetCap >= 0;
}

FCollisionObjectQueryParams
UMythicAnimNotify_SphereOverlap::BuildRuntimeObjectQueryParams() {
    FCollisionObjectQueryParams ObjectTypes;
    ObjectTypes.AddObjectTypesToQuery(ECC_Pawn);
    ObjectTypes.AddObjectTypesToQuery(ECC_WorldStatic);
    ObjectTypes.AddObjectTypesToQuery(ECC_WorldDynamic);
    // Resource ISMs intentionally use the Destructible object channel. Omitting it prevents the physics query from
    // ever producing a typed resource contact, so neither combat filtering nor harvesting authorization can run.
    ObjectTypes.AddObjectTypesToQuery(ECC_Destructible);
    return ObjectTypes;
}

void UMythicAnimNotify_SphereOverlap::OrderAndCapHits(TArray<FHitResult> &Hits, const FVector &Origin, int32 MaxTargets) {
    Hits.Sort([&Origin](const FHitResult &A, const FHitResult &B) {
        return FVector::DistSquared(A.ImpactPoint, Origin) < FVector::DistSquared(B.ImpactPoint, Origin);
    });
    if (MaxTargets > 0 && Hits.Num() > MaxTargets) {
        Hits.SetNum(MaxTargets);
    }
}

void UMythicAnimNotify_SphereOverlap::Notify(USkeletalMeshComponent *MeshComp, UAnimSequenceBase *Animation,
                                             const FAnimNotifyEventReference &EventReference) {
    Super::Notify(MeshComp, Animation, EventReference);

    AActor *Attacker = MeshComp ? MeshComp->GetOwner() : nullptr;
    UWorld *World = Attacker ? Attacker->GetWorld() : nullptr;
    if (!Attacker || !Attacker->HasAuthority() || !World
        || !SendToEventWithTag.IsValid()
        || !IsRuntimeQueryConfigurationValid(
            HitboxRadius, HitboxLocationOffset, MaxTargets)) {
        return;
    }

    const UE::Anim::FAnimNotifyMontageInstanceContext *MontageContext =
        EventReference.GetContextData<
            UE::Anim::FAnimNotifyMontageInstanceContext>();
    UMythicWeaponAttackAbility *ActiveAttack = Cast<UMythicWeaponAttackAbility>(
        MontageContext
            ? UMythicWeaponAttackAbility::ResolveMontageActivationToken(
                  MeshComp, MontageContext->MontageInstanceID)
            : nullptr);
    const EMythicAttackSourceDomain SourceDomain = ActiveAttack
        ? ActiveAttack->GetActiveSourceDomain()
        : EMythicAttackSourceDomain::Invalid;
    if (!ActiveAttack
        || SourceDomain == EMythicAttackSourceDomain::Invalid) {
        return;
    }

    const FVector Origin = Attacker->GetActorTransform().TransformPosition(HitboxLocationOffset);

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicMeleeSweep), false, Attacker);
    const FCollisionObjectQueryParams ObjectTypes =
        BuildRuntimeObjectQueryParams();
    World->SweepMultiByObjectType(Hits, Origin, Origin, FQuat::Identity,
                                  ObjectTypes,
                                  FCollisionShape::MakeSphere(HitboxRadius), Params);

    const APawn *AttackerPawn = Cast<APawn>(Attacker);
    const bool bSourceIsPlayer = AttackerPawn && AttackerPawn->IsPlayerControlled();
    const bool bFriendlyFire = GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;

    TArray<FHitResult> Valid = MoveTemp(Hits);
    UMythicWeaponAttackAbility::FilterTargetHitsForSourceDomain(
        Valid, SourceDomain, Attacker);
    if (SourceDomain == EMythicAttackSourceDomain::Weapon) {
        Valid.RemoveAll(
            [bSourceIsPlayer, bFriendlyFire](const FHitResult &Hit) {
                const APawn *VictimPawn = Cast<const APawn>(Hit.GetActor());
                const bool bTargetIsPlayer =
                    VictimPawn && VictimPawn->IsPlayerControlled();
                return UMythicDamageApplication::ShouldNegateFriendlyFire(
                    bSourceIsPlayer, bTargetIsPlayer, false, bFriendlyFire);
            });
    }
    UMythicWeaponAttackAbility::NormalizeUniqueTargetHits(
        Valid, Attacker, SourceDomain);

    if (Valid.IsEmpty()) {
        return;
    }
    // Combat and harvest contacts have independent budgets. Otherwise a nearby tree can consume MaxTargets before
    // a living target, or a clustered enemy pack can prevent the exact resource instance from reaching validation.
    TArray<FHitResult> ResourceHits;
    TArray<FHitResult> CombatOrDestructibleHits;
    for (const FHitResult &Hit : Valid) {
        (Cast<UMythicResourceISM>(Hit.GetComponent())
             ? ResourceHits
             : CombatOrDestructibleHits)
            .Add(Hit);
    }
    OrderAndCapHits(CombatOrDestructibleHits, Origin, MaxTargets);
    OrderAndCapHits(ResourceHits, Origin, MaxTargets);
    Valid = MoveTemp(CombatOrDestructibleHits);
    Valid.Append(ResourceHits);

    // One event carrying every target: the ability already iterates target data and de-duplicates per swing, so a
    // cleave costs it nothing beyond the entries it was always written to walk.
    FGameplayAbilityTargetDataHandle TargetData;
    for (const FHitResult &Hit : Valid) {
        FGameplayAbilityTargetData_SingleTargetHit *Entry = new FGameplayAbilityTargetData_SingleTargetHit();
        Entry->HitResult = Hit;
        TargetData.Add(Entry);
    }

    FGameplayEventData Payload;
    Payload.EventTag = SendToEventWithTag;
    Payload.Instigator = Attacker;
    // OptionalObject identifies the exact authored temporal sample. OptionalObject2 binds that sample to the active
    // montage instance, so concurrent attacks reusing the same animation asset cannot consume one another's event.
    Payload.OptionalObject = this;
    Payload.OptionalObject2 = ActiveAttack;
    Payload.TargetData = TargetData;

    AActor *EventTarget = AttackerPawn && AttackerPawn->GetController() ? Cast<AActor>(AttackerPawn->GetController()) : Attacker;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(EventTarget, SendToEventWithTag, Payload);
}
