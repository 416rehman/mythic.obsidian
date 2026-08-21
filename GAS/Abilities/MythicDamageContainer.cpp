#include "MythicDamageContainer.h"

#include "Destructible.h"

bool FMythicDamageContainerSpec::IsDestructible(AActor *Actor) {
    bool bIsDestructible = Actor->GetClass()->ImplementsInterface(UDestructible::StaticClass());
    if (!bIsDestructible) {
        for (UActorComponent *Component : Actor->GetComponents()) {
            if (Component->GetClass()->ImplementsInterface(UDestructible::StaticClass())) {
                bIsDestructible = true;
                break;
            }
        }
    }

    return bIsDestructible;
}

void FMythicDamageContainerSpec::AddTargets(const TArray<FHitResult> &HitResults, const TArray<AActor *> &TargetActors) {
    for (size_t i = 0; i < HitResults.Num(); i++) {
        FHitResult HitResult = HitResults[i];


        auto Actor = HitResult.GetActor();
        if (!Actor) {
            continue;
        }

        FGameplayAbilityTargetData_SingleTargetHit *TargetHit = new FGameplayAbilityTargetData_SingleTargetHit();
        TargetHit->HitResult = HitResult;

        if (IsDestructible(Actor)) {
            DestructibleTargetsHandle.Add(TargetHit);
        }
        else {
            TargetsHandle.Add(TargetHit);
        }
    }

    if (TargetActors.Num() > 0) {
        FGameplayAbilityTargetData_ActorArray *DestructibleActors = new FGameplayAbilityTargetData_ActorArray();
        FGameplayAbilityTargetData_ActorArray *Actors = new FGameplayAbilityTargetData_ActorArray();

        for (AActor *Actor : TargetActors) {
            if (!Actor) {
                continue;
            }

            if (IsDestructible(Actor)) {
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
