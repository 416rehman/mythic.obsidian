#include "MythicDamageContainer.h"

#include "Components/PrimitiveComponent.h"
#include "Destructible.h"
#include "GameFramework/Actor.h"

FMythicDestructibleTargetIdentity
FMythicDestructibleTargetIdentity::ResolveActor(const AActor *Actor) {
    if (!::IsValid(Actor)
        || !Actor->GetClass()->ImplementsInterface(
            UDestructible::StaticClass())) {
        return {};
    }
    return {Actor, INDEX_NONE, false};
}

FMythicDestructibleTargetIdentity
FMythicDestructibleTargetIdentity::Resolve(const FHitResult &Hit) {
    const AActor *HitActor = Hit.GetActor();
    if (!::IsValid(HitActor)) {
        return {};
    }

    const UPrimitiveComponent *HitComponent = Hit.GetComponent();
    if (::IsValid(HitComponent)
        && HitComponent->GetClass()->ImplementsInterface(
            UDestructible::StaticClass())) {
        return {HitComponent, Hit.Item, true};
    }
    return ResolveActor(HitActor);
}

void FMythicDamageContainerSpec::AddTargets(
    const TArray<FHitResult> &HitResults,
    const TArray<AActor *> &TargetActors) {
    for (const FHitResult &HitResult : HitResults) {
        if (!IsValid(HitResult.GetActor())) {
            continue;
        }

        FGameplayAbilityTargetData_SingleTargetHit *TargetHit =
            new FGameplayAbilityTargetData_SingleTargetHit();
        TargetHit->HitResult = HitResult;

        if (FMythicDestructibleTargetIdentity::Resolve(HitResult).IsValid()) {
            DestructibleTargetsHandle.Add(TargetHit);
        }
        else {
            TargetsHandle.Add(TargetHit);
        }
    }

    if (TargetActors.Num() > 0) {
        FGameplayAbilityTargetData_ActorArray *DestructibleActors =
            new FGameplayAbilityTargetData_ActorArray();
        FGameplayAbilityTargetData_ActorArray *Actors =
            new FGameplayAbilityTargetData_ActorArray();

        for (AActor *Actor : TargetActors) {
            if (!IsValid(Actor)) {
                continue;
            }

            if (FMythicDestructibleTargetIdentity::ResolveActor(Actor).IsValid()) {
                DestructibleActors->TargetActorArray.Add(Actor);
            }
            else {
                Actors->TargetActorArray.Add(Actor);
            }
        }

        if (DestructibleActors->TargetActorArray.Num() > 0) {
            DestructibleTargetsHandle.Add(DestructibleActors);
        }
        else {
            delete DestructibleActors;
        }
        if (Actors->TargetActorArray.Num() > 0) {
            TargetsHandle.Add(Actors);
        }
        else {
            delete Actors;
        }
    }
}
