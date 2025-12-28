// 


#include "MythicResourceManagerComponent.h"

#include "Mythic.h"
#include "MythicResourceISM.h"
#include "Net/UnrealNetwork.h"

// Called on clients before resources are removed on the server - cosmetic only
// To get here:
// 1. The server has a timer that checks for batch respawn (ProcessBatchRespawn)
// 2. Inside ProcessBatchRespawn, if any resources are due to respawn, they are removed from the DestroyedResources array
// 3. This triggers PreReplicatedRemove on clients, which calls HandleResourceRespawn
void FTrackedDestructibleDataArray::PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize) {
    UE_LOG(Myth, Log, TEXT("FTrackedDestructibleData::PreReplicatedRemove: Removed %d items"), RemovedIndices.Num());

    if (RemovedIndices.Num() <= 0) {
        return;
    }

    // For each removed instance
    auto itemsToSync = TArray<FTrackedDestructibleData>();
    for (auto index : RemovedIndices) {
        if (Items.IsValidIndex(index)) {
            itemsToSync.Add(Items[index]);
        }
    }

    UMythicResourceManagerComponent::HandleResourceRespawn(itemsToSync);
}

// Called on clients after a destroyed resource is added to the array on the server - cosmetic only
void FTrackedDestructibleDataArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    UE_LOG(Myth, Log, TEXT("FTrackedDestructibleData::PostReplicatedAdd: Added %d items"), AddedIndices.Num());

    if (AddedIndices.Num() <= 0) {
        return;
    }

    // For each removed instance 
    auto itemsToSync = TArray<FTrackedDestructibleData>();
    for (auto index : AddedIndices) {
        if (Items.IsValidIndex(index)) {
            itemsToSync.Add(Items[index]);
        }
    }

    UMythicResourceManagerComponent::HandleResourceDestruction(itemsToSync);
}

// Called on clients after the destroyed resources array has changed - Cosmetic only
void FTrackedDestructibleDataArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {
    UE_LOG(Myth, Log, TEXT("FTrackedDestructibleData::PostReplicatedChange: Changed %d items"), ChangedIndices.Num());

    if (ChangedIndices.Num() <= 0) {
        return;
    }

    // For each updated instance
    auto itemsToSync = TArray<FTrackedDestructibleData>();
    for (auto index : ChangedIndices) {
        if (Items.IsValidIndex(index)) {
            itemsToSync.Add(Items[index]);
        }
    }

    UMythicResourceManagerComponent::HandleResourceDestruction(itemsToSync);
}


void UMythicResourceManagerComponent::ProcessBatchRespawn() {
    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Checking for resources to respawn"));
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn called on non-authority"));
        return;
    }

    float CurrentTime = GetWorld()->GetTimeSeconds();
    auto indicesToRemove = TArray<int32>();
    auto DestroyedItems = *DestroyedResources.GetItems();
    for (int32 i = 0; i < DestroyedItems.Num(); i++) {
        auto &item = DestroyedItems[i];
        if (item.HitsTillDestruction <= 0 && item.RespawnTime > 0 && CurrentTime >= item.RespawnTime) {
            indicesToRemove.Add(i);
        }
    }

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Found %d resources to respawn"), indicesToRemove.Num());

    if (indicesToRemove.Num() <= 0) {
        return;
    }

    DestroyedResources.RemoveItems(indicesToRemove);

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Removed %d resources from destroyed list"), indicesToRemove.Num());
}

void UMythicResourceManagerComponent::OnRep_DestroyedResources() {
    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources"));

    // Synchronize state - On first connect, we need a delay to give time to the world to load
    if (GetWorld()) {
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: Handling resource destruction after delay"));
        auto Items = DestroyedResources.GetItems();
        HandleResourceDestruction(*Items);
    }
    else {
        // Keep trying next frame
        UE_LOG(Myth, Warning, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: World is null, retrying next frame"));

        GetWorld()->GetTimerManager().SetTimerForNextTick([this]() {
            UE_LOG(Myth, Log,
                   TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: Retrying OnRep_DestroyedResources because World should be valid now"));
            OnRep_DestroyedResources();
        });
    }
}

// Sets default values for this component's properties
UMythicResourceManagerComponent::UMythicResourceManagerComponent() {
    // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
    // off to improve performance if you don't need them.
    PrimaryComponentTick.bCanEverTick = true;

    SetIsReplicatedByDefault(true);
}

// Authority Only - Transform is only used for restoring purposes
void UMythicResourceManagerComponent::AddOrUpdateResource(FTransform Transform, int32 DamageAmount, APlayerController *PlayerController,
                                                          UMythicResourceISM *ResourceISM, int32 index) {
    // Early return for non-authority
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::AddOrUpdateResource called on non-authority"));
        return;
    }

    if (DamageAmount <= 0) {
        UE_LOG(Myth, Warning, TEXT("UMythicResourceManagerComponent::AddOrUpdateResource: DamageAmount is <= 0, ignoring"));
        return;
    }

    if (ResourceISM) {
        auto Trans = FTransform();
        ResourceISM->GetInstanceTransform(index, Trans, true);
        // Draw
        auto start = Trans.GetLocation();
        auto end = start + FVector3d(0, 0, 1000);
        const auto color = FColor::Black;
        constexpr auto thickness = 5.0f;
        DrawDebugLine(GetWorld(), start, end, color, false, 20, 1, thickness);
    }

    // Try to find existing resource
    FTrackedDestructibleData *ExistingResource = TrackedResources.FindByPredicate([&](const FTrackedDestructibleData &TrackedResource) {
        return TrackedResource.ResourceISM == ResourceISM && TrackedResource.InstanceId == ResourceISM->InstanceIndexToId(index).Id;
        // return TrackedResource.ResourceISMCClass == ISMClass && TrackedResource.Transform.GetLocation().Equals(Transform.GetLocation(), 1.0f);
    });

    if (ExistingResource) {
        ApplyDamageToResource(*ExistingResource, DamageAmount, PlayerController);
    }
    else {
        AddNewResource(Transform, DamageAmount, PlayerController, ResourceISM, index);
    }
}

void UMythicResourceManagerComponent::LoadDestroyedResource(UMythicResourceISM *ResourceISM, int32 InstanceId, FTransform Transform, double RespawnTime) {
    if (!GetOwner()->HasAuthority()) {
        return;
    }

    if (!ResourceISM) {
        return;
    }

    FTrackedDestructibleData NewDestructible;
    NewDestructible.ResourceISM = ResourceISM;
    NewDestructible.InstanceId = InstanceId;
    NewDestructible.Transform = Transform;
    NewDestructible.RespawnTime = RespawnTime;
    NewDestructible.HitsTillDestruction = 0; // It's destroyed

    // Check if already in list?
    bool bExists = DestroyedResources.GetItems()->ContainsByPredicate([&](const FTrackedDestructibleData &Existing) {
        return Existing.ResourceISM == ResourceISM && Existing.InstanceId == InstanceId;
    });

    if (!bExists) {
        DestroyedResources.AddItem(NewDestructible);

        // Also ensure it is visually destroyed immediately on Server
        ResourceISM->DestroyResource(InstanceId);
    }
}

void UMythicResourceManagerComponent::ApplyDamageToResource(FTrackedDestructibleData &Resource, int32 DamageAmount, APlayerController *PlayerController) {
    int32 PreviousHits = Resource.HitsTillDestruction;
    Resource.HitsTillDestruction = FMath::Max(0, Resource.HitsTillDestruction - DamageAmount);

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Applied %d damage, HitsTillDestruction: %d -> %d"),
           DamageAmount, PreviousHits, Resource.HitsTillDestruction);

    // Check if resource is now destroyed
    if (Resource.HitsTillDestruction <= 0 && PreviousHits > 0) {
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Resource destroyed!"));

        // Remove from tracked resources
        auto NumRemoved = TrackedResources.RemoveAll([&](const FTrackedDestructibleData &TrackedResource) {
            return TrackedResource == Resource;
        });

        if (NumRemoved <= 0) {
            UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Could not remove resource from tracked resources"));
            return;
        }

        // Create destroyed resource with respawn time
        AddToDestroyedResources(Resource, PlayerController);
    }
}

void UMythicResourceManagerComponent::AddNewResource(FTransform Transform, int32 DamageAmount,
                                                     APlayerController *PlayerController, UMythicResourceISM *ResourceISM, int32 Index) {
    FTrackedDestructibleData NewResource = FTrackedDestructibleData();
    NewResource.ResourceISM = ResourceISM;
    NewResource.Transform = Transform;

    // auto World = this->GetWorld();

    // auto ISMComponent = NewResource.GetISMComponent(World);
    auto ISMComponent = NewResource.ResourceISM;
    if (!ISMComponent) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Could not find ISM component for class %s"), *ISMComponent->GetName());
        return;
    }

    // Get max health for this destructible type
    int32 MaxHealth = ISMComponent->CalculateHealthFromTransform(Transform);
    if (MaxHealth <= 0) {
        UE_LOG(Myth, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Calculated MaxHealth is <= 0 for ISM %s at location %s"),
               *ISMComponent->GetName(), *Transform.GetLocation().ToString());
        return;
    }

    NewResource.InstanceId = ISMComponent->InstanceIndexToId(Index).Id;
    NewResource.HitsTillDestruction = FMath::Max(0, MaxHealth - DamageAmount);

    UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: New resource with MaxHealth %d, taking %d damage, HitsTillDestruction: %d"),
           MaxHealth, DamageAmount, NewResource.HitsTillDestruction);

    // Check if already destroyed
    auto AlreadyDestroyed = DestroyedResources.GetItems()->FindByPredicate([&NewResource](const FTrackedDestructibleData &DestroyedResource) {
        return DestroyedResource == NewResource;
    });
    if (AlreadyDestroyed) {
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: Resource already destroyed, ignoring"));
        return;
    }

    // If the health is <= 0, no need to add to tracked resources and we can just add to destroyed resources
    if (NewResource.HitsTillDestruction <= 0) {
        AddToDestroyedResources(NewResource, PlayerController);
    }
    else {
        TrackedResources.Add(NewResource);
        UE_LOG(Myth, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: Added to tracked resources"));
    }
}

// Authority only - Make sure the resource is removed from tracked resources
void UMythicResourceManagerComponent::AddToDestroyedResources(FTrackedDestructibleData DestroyedResource, APlayerController *PlayerController) {
    // Set respawn time
    DestroyedResource.RespawnTime = GetWorld()->GetTimeSeconds() + DefaultRespawnDelay;

    // Add to destroyed resources
    DestroyedResources.AddItem(DestroyedResource);

    UE_LOG(Myth, Log,
           TEXT("UMythicResourceManagerComponent::AddToDestroyedResources: Resource %d added to destroyed resources, will respawn in %.1f seconds"),
           DestroyedResource.InstanceId, DefaultRespawnDelay);

    // Give rewards
    DestroyedResource.ResourceISM->OnKillRewards.Give(PlayerController, false);
}

// Called when the game starts
void UMythicResourceManagerComponent::BeginPlay() {
    Super::BeginPlay();

    // Set owner of DestroyedResources for replication callbacks
    DestroyedResources.OwnerComponent = this;

    // Only server runs the batch respawn system
    if (GetOwner()->HasAuthority()) {
        GetWorld()->GetTimerManager().SetTimer(
            BatchRespawnTimerHandle,
            this,
            &UMythicResourceManagerComponent::ProcessBatchRespawn,
            BatchRespawnInterval,
            true // Repeat every 10 minutes
            );

        UE_LOG(Myth, Log, TEXT("Batch respawn system started - checking every %.1f seconds"), BatchRespawnInterval);
    }
}

void UMythicResourceManagerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UMythicResourceManagerComponent, DestroyedResources);
}

TArray<FTrackedDestructibleData> UMythicResourceManagerComponent::GetTrackedDestructibles() const {
    return this->TrackedResources;
}

// This is only called OnRep of the MythicResourceManagerComponent's DestroyedResources array to catch up state on clients
void UMythicResourceManagerComponent::HandleResourceDestruction(const TArray<FTrackedDestructibleData> &DestroyedResources) {
    UE_LOG(Myth, Log, TEXT("HandleResourceDestruction: Syncing destruction of %d resources"), DestroyedResources.Num());

    // All ISM Components that need to be dirtied once after processing
    TSet<UMythicResourceISM *> ISMsToDirty;

    auto length = DestroyedResources.Num();
    for (int i = 0; i < length; i++) {
        auto Resource = DestroyedResources[i];

        // auto ResourceComponent = Resource.GetISMComponent(World);
        auto ResourceComponent = Resource.ResourceISM;
        if (!ResourceComponent) {
            // TODO - Should we have a retry queue so when the ISM's owning actor is loaded we can retry?
            UE_LOG(Myth, Error, TEXT("HandleResourceDestruction: ResourceISMC is null or not loaded"));
            continue;
        }

        UE_LOG(Myth, Log, TEXT("HandleResourceDestruction: Syncing resource on ISM %s, InstanceId %d"), *ResourceComponent->GetName(),
               Resource.InstanceId);

        // Destroy / Hide resource
        ResourceComponent->DestroyResource(Resource.InstanceId);

        // Track ISM to dirty once after batch to reduce render updates
        ISMsToDirty.Add(ResourceComponent);
    }

    // Dirty render state once per ISM after processing all instances
    for (UMythicResourceISM *ISM : ISMsToDirty) {
        if (ISM) {
            ISM->MarkRenderStateDirty();
        }
    }
}

// This is only called on PreReplicateRemove of the MythicResourceManagerComponent's DestroyedResources array to respawn resources as they are removed
void UMythicResourceManagerComponent::HandleResourceRespawn(const TArray<FTrackedDestructibleData> &RespawnedResources) {
    UE_LOG(Myth, Log, TEXT("HandleResourceRespawn: Syncing respawn of %d resources"), RespawnedResources.Num());

    auto length = RespawnedResources.Num();
    for (int i = 0; i < length; i++) {
        auto Resource = RespawnedResources[i];

        auto ResourceComponent = Resource.ResourceISM;
        if (!ResourceComponent) {
            UE_LOG(Myth, Error, TEXT("HandleResourceRespawn: ResourceISMC is null or not loaded"));
            continue;
        }

        UE_LOG(Myth, Log, TEXT("HandleResourceRespawn: Syncing resource on ISM %s, InstanceId %d"), *ResourceComponent->GetName(),
               Resource.InstanceId);

        // Restore / Unhide resource
        bool ShouldUpdateRender = i == length - 1; // Only update on the last one to save performance
        ResourceComponent->RestoreResource(Resource.InstanceId, Resource.Transform, ShouldUpdateRender);
    }
}
