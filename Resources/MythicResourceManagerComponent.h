// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Mythic.h"
#include "MythicResourceISM.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicResourceManagerComponent.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FTrackedDestructibleData : public FFastArraySerializerItem {
    GENERATED_BODY()

    // Instance Id
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
    double RespawnTime;

    // The ISM class that owns this resource
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    TSubclassOf<UMythicResourceISM> ResourceISMCClass;

    // Non replicated - Cached Resource ISM component
    UPROPERTY(Transient)
    TWeakObjectPtr<UMythicResourceISM> ResourceISMC;

    // Non replicated - Cached hit result from trace to find ISM component
    UPROPERTY(Transient)
    FHitResult Hit = FHitResult();

    // Returns the ISM component, doing a trace to find it if not already cached
    UMythicResourceISM *GetISMComponent(UWorld *World) {
        if (ResourceISMC.IsValid()) {
            return ResourceISMC.Get();
        }

        // Do a trace to find the actor at this location, account for slight offsets
        if (!World) {
            UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::GetISMComponent: World is null"));
            return nullptr;
        }

        FVector Start = Transform.GetLocation() + FVector(0, 0, 500);
        FVector End = Transform.GetLocation() - FVector(0, 0, 500);
        DrawDebugLine(World, Start, End, FColor::Cyan, true, 10.0f,1, 5.0f);
        
        FCollisionQueryParams Params;
        Params.bReturnPhysicalMaterial = false;
        Params.bTraceComplex = false;
        Params.AddIgnoredActor(nullptr); // Ignore no actors
        this->Hit = FHitResult();
        bool bHit = World->LineTraceSingleByChannel(this->Hit, Start, End, ECC_Visibility, Params);
        if (!bHit || !this->Hit.GetActor()) {
            UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::GetISMComponent: Could not find actor at location %s"), *Transform.GetLocation().ToString());
            return nullptr;
        }
        auto actor = this->Hit.GetActor();
        if (!actor) {
            UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::GetISMComponent: HitResult actor is null"));
            return nullptr;
        }

        auto ISMComponent = actor->FindComponentByClass<UMythicResourceISM>();
        if (!ISMComponent) {
            UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::GetISMComponent: Could not find UMythicResourceISM component on actor %s"),
                   *actor->GetName());
            return nullptr;
        }

        this->ResourceISMC = ISMComponent;

        return ISMComponent;
    }

    int32 GetInstanceIndex(UWorld *World) {
        auto ISMComponent = GetISMComponent(World);
        if (!ISMComponent) {
            UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::GetInstanceIndex: ISMComponent is null"));
            return -1;
        }

        auto index = this->Hit.Item;
        if (index < 0) {
            UE_LOG(Mythic, Error, TEXT("FTrackedDestructibleData::GetInstanceIndex: HitResult.Item is invalid"));
            return -1;
        }

        return index;
    }
};

USTRUCT(BlueprintType)
struct FTrackedDestructibleDataArray : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTrackedDestructibleData> Items = TArray<FTrackedDestructibleData>();

    /** Step 4: Copy this, replace example with your names */
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FTrackedDestructibleData, FTrackedDestructibleDataArray>(Items, DeltaParms, *this);
    }

    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);
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
    void OnRep_DestroyedResources() const;

public:
    // Sets default values for this component's properties
    UMythicResourceManagerComponent();

    // Add a resource to the tracked list
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void AddOrUpdateResource(FTransform Transform, TSubclassOf<UMythicResourceISM> ISMClass, int32 DamageAmount);

private:
    // Helper functions
    void ApplyDamageToResource(FTrackedDestructibleData &Resource, int32 DamageAmount);
    void AddNewResource(FTransform Transform, TSubclassOf<UMythicResourceISM> ISMClass, int32 DamageAmount);
    void AddToDestroyedResources(FTrackedDestructibleData DestroyedResource);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Lifetime replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    TArray<FTrackedDestructibleData> GetTrackedDestructibles() const;

    // Used for handling destruction of resources after they are added to the destroyed resources array
    static void HandleResourceDestruction(const TArray<FTrackedDestructibleData>& DestroyedResources, UWorld* World);
    static void HandleResourceRespawn(const TArray<FTrackedDestructibleData>& RespawnedResources, UWorld* World);
};
