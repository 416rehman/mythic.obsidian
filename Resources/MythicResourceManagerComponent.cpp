// 


#include "MythicResourceManagerComponent.h"

#include "Mythic.h"
#include "MythicResourceStaticMeshComponent.h"
#include "Net/UnrealNetwork.h"


void FTrackedDestructibleData::PreReplicatedRemove(const struct FTrackedDestructibleDataArray &InArraySerializer) {
    UE_LOG(Mythic, Log, TEXT("FTrackedDestructibleData::PreReplicatedRemove"));
    // Spawn the resource back up
    if (this->HitsTillDestruction <= 0) {
        return;
    }
}

void FTrackedDestructibleData::PostReplicatedAdd(const struct FTrackedDestructibleDataArray &InArraySerializer) {
    UE_LOG(Mythic, Log, TEXT("FTrackedDestructibleData::PostReplicatedAdd"));

    // Update the resource
    if (this->HitsTillDestruction <= 0) {
        return;
    }
}

void FTrackedDestructibleData::PostReplicatedChange(const struct FTrackedDestructibleDataArray &InArraySerializer) {
    UE_LOG(Mythic, Log, TEXT("FTrackedDestructibleData::PostReplicatedChange"));

    // Update the resource
    if (this->HitsTillDestruction <= 0) {
        return;
    }
}


void UMythicDestructiblesManagerComponent::OnRep_TrackedResources() const {
    UE_LOG(Mythic, Log, TEXT("UMythicDestructiblesManagerComponent::OnRep_TrackedResources"));

    // Synchronize state
    SyncResourcesState(TrackedResources.Items);
}

// Sets default values for this component's properties
UMythicDestructiblesManagerComponent::UMythicDestructiblesManagerComponent() {
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    SetIsReplicatedByDefault(true);
}

// Authority Only - Transform is only used for restoring purposes
void UMythicDestructiblesManagerComponent::AddOrUpdateResource(AActor *Actor, FTransform Transform, int32 HitsTillDestruction, FGameplayTag DestructibleTag,
                                                               int32 InstanceId) {
    if (!this->GetOwner()->HasAuthority()) {
        return;
    }
    
    auto found_item = this->TrackedResources.Items.FindByPredicate([&](FTrackedDestructibleData &TrackedResource) {
        // return TrackedResource.Transform.GetLocation() == Transform.GetLocation();
        return TrackedResource.Actor == Actor && TrackedResource.InstanceId == InstanceId && TrackedResource.DestructibleType == DestructibleTag;
    });

    if (found_item) {
        if (found_item->HitsTillDestruction != HitsTillDestruction) {
            found_item->HitsTillDestruction = HitsTillDestruction;
            this->TrackedResources.MarkItemDirty(*found_item);
        }
    }
    else {
        FTrackedDestructibleData TrackedResource;
        TrackedResource.Actor = Actor;
        TrackedResource.Transform = Transform;
        TrackedResource.HitsTillDestruction = HitsTillDestruction;
        TrackedResource.DestructibleType = DestructibleTag;
        TrackedResource.InstanceId = InstanceId;
        this->TrackedResources.Items.Add(TrackedResource);

        this->TrackedResources.MarkItemDirty(TrackedResource);
    }
}

void UMythicDestructiblesManagerComponent::RemoveResource(AActor *Actor, FTransform Transform, int32 InstanceIdentifier) {
    // Authority only
    if (!this->GetOwner()->HasAuthority()) {
        return;
    }

    // Remove the resource from the tracked list
    this->TrackedResources.Items.RemoveAll([&](FTrackedDestructibleData &TrackedResource) {
        return TrackedResource.Transform.GetLocation() == Transform.GetLocation();
    });

    this->TrackedResources.MarkArrayDirty();
}

// Called when the game starts
void UMythicDestructiblesManagerComponent::BeginPlay() {
    Super::BeginPlay();
}

void UMythicDestructiblesManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicDestructiblesManagerComponent, TrackedResources);
}

TArray<FTrackedDestructibleData> UMythicDestructiblesManagerComponent::GetTrackedDestructibles() const {
    return this->TrackedResources.Items;
}
