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
    UE_LOG(Mythic, Log, TEXT("FTrackedDestructibleData::PreReplicatedRemove: Removed %d items"), RemovedIndices.Num());

    if (RemovedIndices.Num() <= 0) {
        return;
    }

    // For each removed instance
    // auto itemsToSync = TArray<FTrackedDestructibleData>();
    // for (auto index : RemovedIndices) {
    //     if (Items.IsValidIndex(index)) {
    //         itemsToSync.Add(Items[index]);
    //     }
    // }
    //
    // UMythicResourceManagerComponent::HandleResourceRespawn(itemsToSync);
}

// Called on clients after a destroyed resource is added to the array on the server - cosmetic only
void FTrackedDestructibleDataArray::PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize) {
    UE_LOG(Mythic, Log, TEXT("FTrackedDestructibleData::PostReplicatedAdd: Added %d items"), AddedIndices.Num());

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

    auto Owner = this->GetOwnerComponent();
    if (!Owner) {
        UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::PostReplicatedAdd: OwningObject is null"));
        return;
    }

    auto World = Owner->GetWorld();
    if (!World) {
        UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::PostReplicatedAdd: World is null"));
        return;
    }

    UMythicResourceManagerComponent::HandleResourceDestruction(itemsToSync, World);
}

// Called on clients after the destroyed resources array has changed - Cosmetic only
void FTrackedDestructibleDataArray::PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize) {
    UE_LOG(Mythic, Log, TEXT("FTrackedDestructibleData::PostReplicatedChange: Changed %d items"), ChangedIndices.Num());

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

    auto Owner = this->GetOwnerComponent();
    if (!Owner) {
        UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::PostReplicatedChange: OwningObject is null"));
        return;
    }

    auto World = Owner->GetWorld();
    if (!World) {
        UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::PostReplicatedChange: World is null"));
        return;
    }

    UMythicResourceManagerComponent::HandleResourceDestruction(itemsToSync, World);
}


void UMythicResourceManagerComponent::ProcessBatchRespawn() {
    UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Checking for resources to respawn"));
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn called on non-authority"));
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

    UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Found %d resources to respawn"), indicesToRemove.Num());

    if (indicesToRemove.Num() <= 0) {
        return;
    }

    DestroyedResources.RemoveItems(indicesToRemove);

    UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Removed %d resources from destroyed list"), indicesToRemove.Num());

    // If we removed any items, mark the array dirty
    if (indicesToRemove.Num() > 0) {
        DestroyedResources.MarkArrayDirty();
        UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::ProcessBatchRespawn: Marked DestroyedResources array dirty"));
    }
}

void UMythicResourceManagerComponent::OnRep_DestroyedResources() {
    UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources"));

    // Synchronize state - On first connect, we need a delay to give time to the world to load
    if (UWorld *World = GetWorld()) {
        UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: Handling resource destruction after delay"));
        auto Items = DestroyedResources.GetItems();
        HandleResourceDestruction(*Items, World);
    }
    else {
        // Keep trying next frame
        UE_LOG(Mythic, Warning, TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: World is null, retrying next frame"));

        FTimerHandle DelayHandle;
        GetWorld()->GetTimerManager().SetTimer(DelayHandle, [this]() {
            UE_LOG(Mythic, Log,
                   TEXT("UMythicResourceManagerComponent::OnRep_DestroyedResources: Retrying OnRep_DestroyedResources because World should be valid now"));
            OnRep_DestroyedResources();
        }, 1, false);
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
void UMythicResourceManagerComponent::AddOrUpdateResource(FTransform Transform, TSubclassOf<UMythicResourceISM> ISMClass, int32 DamageAmount, APlayerController* PlayerController) {
    // Early return for non-authority
    if (!GetOwner()->HasAuthority()) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::AddOrUpdateResource called on non-authority"));
        return;
    }

    if (DamageAmount <= 0) {
        UE_LOG(Mythic, Warning, TEXT("UMythicResourceManagerComponent::AddOrUpdateResource: DamageAmount is <= 0, ignoring"));
        return;
    }

    // Try to find existing resource
    FTrackedDestructibleData *ExistingResource = TrackedResources.FindByPredicate([&](const FTrackedDestructibleData &TrackedResource) {
        return TrackedResource.ResourceISMCClass == ISMClass && TrackedResource.Transform.GetLocation().Equals(Transform.GetLocation(), 1.0f);
    });

    if (ExistingResource) {
        ApplyDamageToResource(*ExistingResource, DamageAmount, PlayerController);
    }
    else {
        AddNewResource(Transform, ISMClass, DamageAmount, PlayerController);
    }
}

void UMythicResourceManagerComponent::ApplyDamageToResource(FTrackedDestructibleData &Resource, int32 DamageAmount, APlayerController* PlayerController) {
    int32 PreviousHits = Resource.HitsTillDestruction;
    Resource.HitsTillDestruction = FMath::Max(0, Resource.HitsTillDestruction - DamageAmount);

    UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Applied %d damage, HitsTillDestruction: %d -> %d"),
           DamageAmount, PreviousHits, Resource.HitsTillDestruction);

    // Check if resource is now destroyed
    if (Resource.HitsTillDestruction <= 0 && PreviousHits > 0) {
        UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::ApplyDamageToResource: Resource destroyed!"));
        // Create destroyed resource with respawn time
        AddToDestroyedResources(Resource, PlayerController);

        // Remove from tracked resources
        TrackedResources.RemoveAll([&](const FTrackedDestructibleData &TrackedResource) {
            return TrackedResource.ResourceISMCClass == Resource.ResourceISMCClass && TrackedResource.Transform.GetLocation().Equals(
                Resource.Transform.GetLocation(), 1.0f);
        });
    }
}

void UMythicResourceManagerComponent::AddNewResource(FTransform Transform, TSubclassOf<UMythicResourceISM> ISMClass, int32 DamageAmount, APlayerController* PlayerController) {
    FTrackedDestructibleData NewResource = FTrackedDestructibleData();
    NewResource.ResourceISMCClass = ISMClass;
    NewResource.Transform = Transform;

    auto World = this->GetWorld();
    auto ISMComponent = NewResource.GetISMComponent(World);
    if (!ISMComponent) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Could not find ISM component for class %s"), *ISMClass->GetName());
        return;
    }

    // Get max health for this destructible type
    int32 MaxHealth = ISMComponent->CalculateHealthFromTransform(Transform);
    if (MaxHealth <= 0) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Calculated MaxHealth is <= 0 for ISM %s at location %s"),
               *ISMComponent->GetName(), *Transform.GetLocation().ToString());
        return;
    }

    auto Index = NewResource.GetInstanceIndex(World);
    if (Index < 0) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Could not find instance at location %s on ISM %s"),
               *Transform.GetLocation().ToString(),
               *ISMComponent->GetName());
        return;
    }

    auto InstanceTransform = ISMComponent->GetInstanceTransform(Index, NewResource.Transform, true);
    if (!InstanceTransform) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::AddNewResource: Could not get instance transform for index %d on ISM %s"), Index,
               *ISMComponent->GetName());
        return;
    }

    NewResource.InstanceId = ISMComponent->InstanceIndexToId(Index).Id;
    NewResource.HitsTillDestruction = FMath::Max(0, MaxHealth - DamageAmount);

    UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: New resource with MaxHealth %d, taking %d damage, HitsTillDestruction: %d"),
           MaxHealth, DamageAmount, NewResource.HitsTillDestruction);

    // Check if already destroyed
    auto AlreadyDestroyed = DestroyedResources.GetItems()->FindByPredicate([&NewResource](const FTrackedDestructibleData &DestroyedResource) {
        return DestroyedResource == NewResource;
    });
    if (AlreadyDestroyed) {
        UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: Resource already destroyed, ignoring"));
        return;
    }

    // If the health is <= 0, no need to add to tracked resources and we can just add to destroyed resources
    if (NewResource.HitsTillDestruction <= 0) {
        AddToDestroyedResources(NewResource, PlayerController);
    }
    else {
        TrackedResources.Add(NewResource);
        UE_LOG(Mythic, Log, TEXT("UMythicResourceManagerComponent::AddNewResource: Added to tracked resources"));
    }
}

// Authority only - Make sure the resource is removed from tracked resources
void UMythicResourceManagerComponent::AddToDestroyedResources(FTrackedDestructibleData DestroyedResource, APlayerController* PlayerController) {
    // Set respawn time
    DestroyedResource.RespawnTime = GetWorld()->GetTimeSeconds() + DefaultRespawnDelay;

    // Add to destroyed resources
    DestroyedResources.AddItem(DestroyedResource);

    UE_LOG(Mythic, Log,
           TEXT("UMythicResourceManagerComponent::AddToDestroyedResources: Resource %d added to destroyed resources, will respawn in %.1f seconds"),
           DestroyedResource.InstanceId, DefaultRespawnDelay);

    // Give loot
    auto CDO = DestroyedResource.ResourceISMCClass.GetDefaultObject();
    if (!CDO) {
        UE_LOG(Mythic, Error, TEXT("UMythicResourceManagerComponent::AddToDestroyedResources: Could not get default object for class %s"),
               *DestroyedResource.ResourceISMCClass->GetName());
        return;
    }

    CDO->OnKillRewards.Give(PlayerController);
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

        UE_LOG(Mythic, Log, TEXT("Batch respawn system started - checking every %.1f seconds"), BatchRespawnInterval);
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
void UMythicResourceManagerComponent::HandleResourceDestruction(const TArray<FTrackedDestructibleData> &DestroyedResources, UWorld *World) {
    UE_LOG(Mythic, Log, TEXT("HandleResourceDestruction: Syncing destruction of %d resources"), DestroyedResources.Num());

    auto length = DestroyedResources.Num();
    for (int i = 0; i < length; i++) {
        auto Resource = DestroyedResources[i];

        auto ResourceComponent = Resource.GetISMComponent(World);
        if (!ResourceComponent) {
            // TODO - Should we have a retry queue so when the ISM's owning actor is loaded we can retry?
            UE_LOG(Mythic, Error, TEXT("HandleResourceDestruction: ResourceISMC is null or not loaded"));
            continue;
        }

        UE_LOG(Mythic, Log, TEXT("HandleResourceDestruction: Syncing resource on ISM %s, InstanceId %d"), *ResourceComponent->GetName(),
               Resource.InstanceId);

        // Destroy / Hide resource
        bool ShouldUpdateRender = i == length - 1; // Only update on the last one to save performance
        ResourceComponent->DestroyResource(Resource.InstanceId, Resource.Transform, ShouldUpdateRender);
    }
}

// This is only called on PreReplicateRemove of the MythicResourceManagerComponent's DestroyedResources array to respawn resources as they are removed
void UMythicResourceManagerComponent::HandleResourceRespawn(const TArray<FTrackedDestructibleData> &RespawnedResources, UWorld *World) {
    UE_LOG(Mythic, Log, TEXT("HandleResourceRespawn: Syncing respawn of %d resources"), RespawnedResources.Num());

    auto length = RespawnedResources.Num();
    for (int i = 0; i < length; i++) {
        auto Resource = RespawnedResources[i];

        auto ResourceComponent = Resource.GetISMComponent(World);
        if (!ResourceComponent) {
            // TODO - Should we have a retry queue so when the ISM's owning actor is loaded we can retry?
            UE_LOG(Mythic, Error, TEXT("HandleResourceRespawn: ResourceISMC is null or not loaded"));
            continue;
        }

        UE_LOG(Mythic, Log, TEXT("HandleResourceRespawn: Syncing resource on ISM %s, InstanceId %d"), *ResourceComponent->GetName(),
               Resource.InstanceId);

        // Restore / Unhide resource
        bool ShouldUpdateRender = i == length - 1; // Only update on the last one to save performance
        ResourceComponent->RestoreResource(Resource.InstanceId, Resource.Transform, ShouldUpdateRender);
    }
}
