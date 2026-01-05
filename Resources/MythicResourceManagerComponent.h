// 

#pragma once

#include "CoreMinimal.h"
#include "MythicResourceISM.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicResourceManagerComponent.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FTrackedDestructibleData : public FFastArraySerializerItem {
    GENERATED_BODY()

    // Instance Id (not index)
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    int32 InstanceId = -1;

    // The original transform of the resource
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    FTransform Transform = FTransform::Identity;

    // Remaining hits required to mine this resource
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    int32 HitsTillDestruction = 1;

    // The absolute time after which the resource can respawn
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    double RespawnTime = 0.0;

    // The ISM this resource belongs to
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    UMythicResourceISM *ResourceISM = nullptr;

    bool operator==(const FTrackedDestructibleData &other) const {
        return other.ResourceISM == this->ResourceISM && other.InstanceId == this->InstanceId;
    }
};

USTRUCT(BlueprintType)
struct FTrackedDestructibleDataArray : public FFastArraySerializer {
    GENERATED_BODY()

private:
    UPROPERTY()
    TArray<FTrackedDestructibleData> Items = TArray<FTrackedDestructibleData>();

public:
    // Store reference to the owning component
    UPROPERTY(NotReplicated)
    TWeakObjectPtr<UMythicResourceManagerComponent> OwnerComponent;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FTrackedDestructibleData, FTrackedDestructibleDataArray>(Items, DeltaParms, *this);
    }

    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);

    // Helper function to get the owner safely
    UMythicResourceManagerComponent *GetOwnerComponent() const {
        return OwnerComponent.IsValid() ? OwnerComponent.Get() : nullptr;
    }

    const TArray<FTrackedDestructibleData> *GetItems() const {
        return &this->Items;
    }

    // Add an item and mark the array dirty and call PreReplicatedAdd manually (because server doesn't call it automatically)
    void AddItem(const FTrackedDestructibleData &NewItem) {
        Items.Add(NewItem);
        MarkItemDirty(Items.Last());

        // Manually call PostReplicatedAdd on server as it won't be called automatically
        TArray<int32> AddedIndices;
        AddedIndices.Add(Items.Num() - 1);
        PostReplicatedAdd(AddedIndices, Items.Num());
    }

    // Batch remove
    void RemoveItems(const TArrayView<int32> &RemovedIndices) {
        PreReplicatedRemove(RemovedIndices, Items.Num() - RemovedIndices.Num());

        for (int32 Index : RemovedIndices) {
            if (Items.IsValidIndex(Index)) {
                Items.RemoveAt(Index);
            }
        }

        MarkArrayDirty();
    }
};

template <>
struct TStructOpsTypeTraits<FTrackedDestructibleDataArray> : TStructOpsTypeTraitsBase2<FTrackedDestructibleDataArray> {
    enum {
        WithNetDeltaSerializer = true,
    };
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicResourceManagerComponent : public UActorComponent {
    GENERATED_BODY()

    // Tracked Resources - This is not replicated from the server
    UPROPERTY()
    TArray<FTrackedDestructibleData> TrackedResources = TArray<FTrackedDestructibleData>();

    // Fast Array Serializer - Destroyed Resources
    UPROPERTY(ReplicatedUsing=OnRep_DestroyedResources)
    FTrackedDestructibleDataArray DestroyedResources = FTrackedDestructibleDataArray();

    ///////////// RESPAWNING SYSTEM /////////////
    // Single timer handle for the 10-minute check
    UPROPERTY()
    FTimerHandle BatchRespawnTimerHandle;

    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (AllowPrivateAccess = "true"))
    float BatchRespawnInterval = 600.0f; // 10 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (AllowPrivateAccess = "true"))
    float PlayerCheckRadius = 5000.0f; // 50 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (AllowPrivateAccess = "true"))
    float DefaultRespawnDelay = 300.0f; // 5 minutes
protected:
    // The main respawn function - called every 10 minutes
    UFUNCTION()
    void ProcessBatchRespawn();
    ///////////// END RESPAWNING SYSTEM /////////////

    // OnRep_DestroyedResources
    UFUNCTION()
    void OnRep_DestroyedResources();

public:
    // Sets default values for this component's properties
    UMythicResourceManagerComponent();

    // Add a resource to the tracked list
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void AddOrUpdateResource(FTransform Transform, int32 DamageAmount, APlayerController *PlayerController, UMythicResourceISM *ResourceISM, int32 index);

    // Load destroyed resources from save data
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void LoadDestroyedResource(UMythicResourceISM *ResourceISM, int32 InstanceId, FTransform Transform, double RespawnTime);

private:
    // Helper functions
    void ApplyDamageToResource(FTrackedDestructibleData &Resource, int32 DamageAmount, APlayerController *PlayerController);
    void AddNewResource(FTransform Transform, int32 DamageAmount, APlayerController *PlayerController, UMythicResourceISM *ResourceISM, int32
                        Index);
    void AddToDestroyedResources(FTrackedDestructibleData DestroyedResource, APlayerController *PlayerController);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Lifetime replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    TArray<FTrackedDestructibleData> GetTrackedDestructibles() const;

    const TArray<FTrackedDestructibleData> &GetDestroyedItems() const { return *DestroyedResources.GetItems(); }

    // Used for handling destruction of resources after they are added to the destroyed resources array
    static void HandleResourceDestruction(const TArray<FTrackedDestructibleData> &DestroyedResources);
    static void HandleResourceRespawn(const TArray<FTrackedDestructibleData> &RespawnedResources);
};
