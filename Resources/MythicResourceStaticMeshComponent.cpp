// 


#include "MythicResourceStaticMeshComponent.h"

#include "CollisionDebugDrawingPublic.h"
#include "Mythic.h"
#include "MythicResourceManagerComponent.h"

FHitResult UMythicResourceStaticMeshComponent::SynthesizeHitResult(FVector ImpactPoint, FVector TraceEnd, UWorld *World, AActor *DamageCauser) {
    // We need to perform a new trace to get the correct instance index on both server and client
    FHitResult NewHitResult;

    // Get the direction vector (normalized)
    FVector Direction = (ImpactPoint - TraceEnd).GetSafeNormal();

    // Use the original TraceEnd as your new TraceStart
    FVector NewTraceStart = TraceEnd;

    // Add a constant distance (e.g., 100 units) in the same direction
    float ConstantDistance = 100.0f;
    FVector NewTraceEnd = ImpactPoint + (Direction * ConstantDistance);

    FCollisionQueryParams QueryParams;
    QueryParams.bReturnFaceIndex = true;
    QueryParams.bTraceComplex = true;
    QueryParams.bDebugQuery = true;
    QueryParams.AddIgnoredActor(DamageCauser);

    FCollisionObjectQueryParams CollisionParams;
    CollisionParams.AddObjectTypesToQuery(ECC_Destructible);

    // Use LineTraceSingleByChannel to hit the physical material
    float SphereRadius = 20.0;
    bool bHit = World->SweepSingleByObjectType(NewHitResult, NewTraceStart, NewTraceEnd, FQuat::Identity, CollisionParams,
                                               FCollisionShape::MakeSphere(SphereRadius));

    if (!bHit) {
        // If we couldn't find a valid instance, draw debug trace to visualize the issue
        // Draw the sphere trace in red to indicate failure
        DrawDebugSphere(World, NewTraceStart, SphereRadius, 12, FColor::Green, true, 5.0f);
        DrawDebugSphere(World, NewTraceEnd, SphereRadius, 12, FColor::Green, true, 5.0f);
        DrawDebugLine(World, NewTraceStart, NewTraceEnd, FColor::Blue, true, 5.0f, 0, 2.0f);

        // If we hit something else, draw what we actually hit
        UE_LOG(LogTemp, Warning, TEXT("CalculateHitsLeft didn't hit anything along the trace path"));
    }
    // Check if we hit this component
    if (bHit) {
        if (auto comp = NewHitResult.Component.Get()) {
            // Make sure comp is of type MythicResourceStaticMeshComponent
            if (Cast<UMythicResourceStaticMeshComponent>(comp)) {
                // Get the instance index from the new hit result
                return NewHitResult;
            }
        }
        // Get the instance index from the new hit result
        UE_LOG(LogTemp, Warning, TEXT("Hit a component that is not a MythicResourceStaticMeshComponent"));
    }

    // If we couldn't find a valid instance, return an error code
    UE_LOG(LogTemp, Warning, TEXT("CalculateHitsLeft failed to find valid instanced mesh at hit location"));
    return FHitResult();
}

int UMythicResourceStaticMeshComponent::CalculateHitsLeft(int32 InstanceIndex, AActor *DamageCauser, float Damage = 1) {
    FTransform Transform;
    GetInstanceTransform(InstanceIndex, Transform, true);

    auto ResourceInstance = GetResourceInstanceData(InstanceIndex, Transform);
    auto NewHitsTillDestruction = ResourceInstance->HitsTillDestruction - Damage;

    SetHitsTillDestruction(InstanceIndex, NewHitsTillDestruction, Transform);

    return NewHitsTillDestruction;
}

FGameplayTag UMythicResourceStaticMeshComponent::GetDestructibleType() {
    return this->ResourceType;
}

int UMythicResourceStaticMeshComponent::InitializeHitsTillDestruction_Implementation(float Z) {
    if (Z >= 0.0 && Z < 0.6) {
        return 3;
    }
    if (Z >= 0.6 && Z < 0.8) {
        return 4;
    }
    if (Z >= 0.8 && Z < 1) {
        return 5;
    }
    return 6;
}

void UMythicResourceStaticMeshComponent::SetHitsTillDestruction(int32 InstanceIndex, int32 NewHitsTillDestruction, FTransform Transform) {
    UE_LOG(Mythic, Log, TEXT("SetHitsTillDestruction: InstanceIndex=%d, NewHitsTillDestruction=%d"), InstanceIndex, NewHitsTillDestruction);
    if (NewHitsTillDestruction < 0) {
        return;
    }

    // If the instance is not in the map, calculate the hits till destruction and add it to the map
    if (!ResourceData.Contains(InstanceIndex)) {
        auto NewHits = InitializeHitsTillDestruction(Transform.GetScale3D().Z);
        UE_LOG(Mythic, Log, TEXT("SetHitsTillDestruction: Initializing new tracked resource instance with %d hits"), NewHits);
        ResourceData.Add(InstanceIndex, FResourceInstanceData{NewHits});
    }

    auto ResourceInstance = ResourceData.Find(InstanceIndex);
    if (!ResourceInstance) {
        UE_LOG(Mythic, Warning, TEXT("SetHitsTillDestruction: Failed to find tracked resource instance for index %d"), InstanceIndex);
        return;
    }

    if (ResourceInstance->HitsTillDestruction == NewHitsTillDestruction) {
        UE_LOG(Mythic, Log, TEXT("SetHitsTillDestruction: No change in hits till destruction for instance index %d"), InstanceIndex);
        return;
    }

    // Update the hits till destruction
    ResourceInstance->HitsTillDestruction = NewHitsTillDestruction;
    UE_LOG(Mythic, Log, TEXT("SetHitsTillDestruction: Updated hits till destruction for instance index %d to %d"), InstanceIndex,
           ResourceInstance->HitsTillDestruction);

    // Check if the resource is destroyed
    bool IsDestroyed = ResourceInstance->HitsTillDestruction <= 0;

    // If the resource is destroyed, remove it from the map
    if (IsDestroyed && InstanceIndex >= 0) {
        UE_LOG(Mythic, Log, TEXT("SetHitsTillDestruction: Destroying resource instance index %d because hits till destruction is %d"), InstanceIndex,
               ResourceInstance->HitsTillDestruction);
        ResourceData.Remove(InstanceIndex);

        // Draw Debug sphere at this instance index location
        auto World = this->GetWorld();
        GetInstanceTransform(InstanceIndex, Transform, true);
        DrawDebugSphere(World, Transform.GetTranslation(), 4, 12, FColor::Red, true);

        // Remove the instance from the component
        RemoveInstance(InstanceIndex);
    }
    else {
        UE_LOG(Mythic, Log, TEXT("SetHitsTillDestruction: Resource instance index %d has %d hits remaining"), InstanceIndex,
               ResourceInstance->HitsTillDestruction);
    }
}

const FResourceInstanceData *UMythicResourceStaticMeshComponent::GetResourceInstanceData(int32 InstanceIndex, FTransform Transform) {
    // If the instance is not in the map, calculate the hits till destruction and add it to the map
    if (!ResourceData.Contains(InstanceIndex)) {
        ResourceData.Add(InstanceIndex, FResourceInstanceData{
                             InitializeHitsTillDestruction(Transform.GetScale3D().Z)
                         });
    }

    return ResourceData.Find(InstanceIndex);
}

FRewardsToGive UMythicResourceStaticMeshComponent::GetOnKillRewards(AActor *Killer) {
    return OnKillRewards;
}

void SyncResourcesState(const TArray<FTrackedDestructibleData> &TrackedResources) {
    UE_LOG(Mythic, Log, TEXT("SyncResourcesState: Syncing %d resources"), TrackedResources.Num());
    // For each of these data items, do a trace and remove
    for (auto Resource : TrackedResources) {
        // Check if actor is same as hit result
        if (!Resource.Actor) {
            continue;
        }

        auto comps = Resource.Actor->GetComponents();
        for (auto Comp : comps) {
            UE_LOG(Mythic, Log, TEXT("SyncResourcesState: Checking component %s"), *Comp->GetName());

            // Cast to UMythicResourceStaticMeshComponent
            auto ResourceComponent = Cast<UMythicResourceStaticMeshComponent>(Comp);
            if (!ResourceComponent) {
                UE_LOG(Mythic, Warning, TEXT("SyncResourcesState: Component %s is not a UMythicResourceStaticMeshComponent"), *Comp->GetName());
                continue;
            }

            if (ResourceComponent->ResourceType != Resource.DestructibleType) {
                UE_LOG(Mythic, Warning, TEXT("SyncResourcesState: Resource type mismatch: %s != %s"), *ResourceComponent->ResourceType.ToString(),
                       *Resource.DestructibleType.ToString());
            }

            auto InstanceIndex = ResourceComponent->GetInstanceIndexForId(FPrimitiveInstanceId(Resource.InstanceId));
            UE_LOG(Mythic, Log, TEXT("SyncResourcesState: Instance index %d"), InstanceIndex);

            if (InstanceIndex < 0) {
                UE_LOG(Mythic, Warning, TEXT("SyncResourcesState: Instance index not found for id %d"), Resource.InstanceId);
                continue;
            }

            ResourceComponent->SetHitsTillDestruction(InstanceIndex, Resource.HitsTillDestruction, Resource.Transform);
        }
    }
}
