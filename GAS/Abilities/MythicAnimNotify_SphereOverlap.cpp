
#include "MythicAnimNotify_SphereOverlap.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemGlobals.h"
#include "Abilities/GameplayAbilityTargetTypes.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"

#include "GAS/Executions/MythicDamageApplication.h"
#include "Settings/MythicDeveloperSettings.h"

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
    if (!Attacker || !World || !SendToEventWithTag.IsValid() || HitboxRadius <= 0.0f) {
        return;
    }

    const FVector Origin = Attacker->GetActorTransform().TransformPosition(HitboxLocationOffset);

    TArray<FHitResult> Hits;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(MythicMeleeSweep), false, Attacker);
    World->SweepMultiByChannel(Hits, Origin, Origin, FQuat::Identity, ECC_Pawn,
                               FCollisionShape::MakeSphere(HitboxRadius), Params);

    const APawn *AttackerPawn = Cast<APawn>(Attacker);
    const bool bSourceIsPlayer = AttackerPawn && AttackerPawn->IsPlayerControlled();
    const bool bFriendlyFire = GetDefault<UMythicDeveloperSettings>()->bFriendlyFireEnabled;

    TSet<const AActor *> Seen;
    TArray<FHitResult> Valid;
    for (const FHitResult &Hit : Hits) {
        const AActor *Victim = Hit.GetActor();
        if (!Victim || Victim == Attacker || Seen.Contains(Victim)) {
            continue;
        }
        if (!UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Victim)) {
            continue;
        }
        // The same rule the damage execution applies, so a swing cannot reach what a hit would be refused.
        const APawn *VictimPawn = Cast<const APawn>(Victim);
        const bool bTargetIsPlayer = VictimPawn && VictimPawn->IsPlayerControlled();
        if (UMythicDamageApplication::ShouldNegateFriendlyFire(bSourceIsPlayer, bTargetIsPlayer, false, bFriendlyFire)) {
            continue;
        }
        Seen.Add(Victim);
        Valid.Add(Hit);
    }

    if (Valid.IsEmpty()) {
        return;
    }
    OrderAndCapHits(Valid, Origin, MaxTargets);

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
    Payload.TargetData = TargetData;

    AActor *EventTarget = AttackerPawn && AttackerPawn->GetController() ? Cast<AActor>(AttackerPawn->GetController()) : Attacker;
    UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(EventTarget, SendToEventWithTag, Payload);
}
