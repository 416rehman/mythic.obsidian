
#pragma once

#include "CoreMinimal.h"
#include "MythicResourceISM.h"
#include "MythicGatheringConfig.h"
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
    UPROPERTY(NotReplicated)
    TWeakObjectPtr<UMythicResourceManagerComponent> OwnerComponent;

    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FTrackedDestructibleData, FTrackedDestructibleDataArray>(Items, DeltaParms, *this);
    }

    void PreReplicatedRemove(const TArrayView<int32> &RemovedIndices, int32 FinalSize);
    void PostReplicatedAdd(const TArrayView<int32> &AddedIndices, int32 FinalSize);
    void PostReplicatedChange(const TArrayView<int32> &ChangedIndices, int32 FinalSize);

    UMythicResourceManagerComponent *GetOwnerComponent() const {
        return OwnerComponent.IsValid() ? OwnerComponent.Get() : nullptr;
    }

    const TArray<FTrackedDestructibleData> *GetItems() const {
        return &this->Items;
    }

    void AddItem(const FTrackedDestructibleData &NewItem) {
        Items.Add(NewItem);
        MarkItemDirty(Items.Last());

        TArray<int32> AddedIndices;
        AddedIndices.Add(Items.Num() - 1);
        PostReplicatedAdd(AddedIndices, Items.Num());
    }

    void RemoveItems(const TArrayView<int32> &RemovedIndices) {
        PreReplicatedRemove(RemovedIndices, Items.Num() - RemovedIndices.Num());

        TArray<int32> SortedIndices(RemovedIndices.GetData(), RemovedIndices.Num());
        SortedIndices.Sort([](int32 A, int32 B) { return A > B; });
        for (int32 Index : SortedIndices) {
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

    UPROPERTY()
    TArray<FTrackedDestructibleData> TrackedResources = TArray<FTrackedDestructibleData>();

    UPROPERTY(ReplicatedUsing=OnRep_DestroyedResources)
    FTrackedDestructibleDataArray DestroyedResources = FTrackedDestructibleDataArray();

    UPROPERTY()
    FTimerHandle BatchRespawnTimerHandle;

    // proficiency-based gathering bonuses (damage scaling + double yield chance)
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Gathering", meta = (AllowPrivateAccess = "true"))
    FGatheringProficiencyConfig GatheringConfig;

    // Settings
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (AllowPrivateAccess = "true"))
    float BatchRespawnInterval = 600.0f; // 10 minutes

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (AllowPrivateAccess = "true"))
    float PlayerCheckRadius = 5000.0f; // 50 meters

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Respawn", meta = (AllowPrivateAccess = "true"))
    float DefaultRespawnDelay = 300.0f; // 5 minutes
protected:
    UFUNCTION()
    void ProcessBatchRespawn();

    UFUNCTION()
    void OnRep_DestroyedResources();

public:
    UMythicResourceManagerComponent();

    // Add a resource to the tracked list
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void AddOrUpdateResource(FTransform Transform, int32 DamageAmount, APlayerController *PlayerController, UMythicResourceISM *ResourceISM, int32 index);

    // Load destroyed resources from save data.
    // RemainingSeconds = seconds-until-respawn (clock-independent), NOT an absolute world-time deadline.
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void LoadDestroyedResource(UMythicResourceISM *ResourceISM, int32 InstanceId, FTransform Transform, double RemainingSeconds);

private:
    int32 ApplyDamageToResource(FTrackedDestructibleData &Resource, int32 DamageAmount, APlayerController *PlayerController);
    int32 AddNewResource(FTransform Transform, int32 DamageAmount, APlayerController *PlayerController, UMythicResourceISM *ResourceISM, int32
                         Index);
    void AddToDestroyedResources(FTrackedDestructibleData DestroyedResource, APlayerController *PlayerController);

    int32 GetGathererProficiencyLevel(APlayerController *PlayerController, const FGameplayTag &ResourceType) const;

protected:
    virtual void BeginPlay() override;

public:
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    TArray<FTrackedDestructibleData> GetTrackedDestructibles() const;

    static bool ShouldRespawnDestructible(int32 HitsTillDestruction, float RespawnTime, float CurrentTime);

    const TArray<FTrackedDestructibleData> &GetDestroyedItems() const { return *DestroyedResources.GetItems(); }

    static void HandleResourceDestruction(const TArray<FTrackedDestructibleData> &DestroyedResources);
    static void HandleResourceRespawn(const TArray<FTrackedDestructibleData> &RespawnedResources);
};
