#include "MythicDamageContainer.h"

#include "Destructible.h"

/// Determines if an actor or any of its components implement the Destructible interface.
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

/// Adds hit results and target actors to the damage container, separating destructible and non-destructible targets.
void FMythicDamageContainerSpec::AddTargets(const TArray<FHitResult> &HitResults, const TArray<AActor *> &TargetActors) {
    for (size_t i = 0; i < HitResults.Num(); i++) {
        FHitResult HitResult = HitResults[i];

        // If actor is Destructible, add it to DestructionTargets

        auto Actor = HitResult.GetActor();
        if (!Actor) {
            continue;
        }

        FGameplayAbilityTargetData_SingleTargetHit *TargetHit = new FGameplayAbilityTargetData_SingleTargetHit();
        TargetHit->HitResult = HitResult;

        // If the actor is Destructible, or any of its components implement the Destructible interface,
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

            // Cast to Destructible, if it implements it, add to DestructibleTargetsHandle, else add to TargetsHandle
            if (IsDestructible(Actor)) {
                DestructibleActors->TargetActorArray.Add(Actor);
            }
            else {
                Actors->TargetActorArray.Add(Actor);
            }
        }

        // Hand ownership to the handle (which wraps the raw ptr in a TSharedPtr) ONLY for a non-empty category, and
        // delete the other allocation — otherwise the empty category's `new` leaks on every hit. This is the common
        // case on the melee/AoE chokepoint: a swing hitting only non-destructibles leaves DestructibleActors empty.
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
