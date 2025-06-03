// 

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Net/Serialization/FastArraySerializer.h"
#include "MythicResourceManagerComponent.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FTrackedDestructibleData : public FFastArraySerializerItem {
    GENERATED_BODY()

    // The actor owning the component with the ResourceType tag
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    AActor *Actor;

    // The transform of the resource
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    FTransform Transform;

    // Hits till destruction
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    int32 HitsTillDestruction;

    // Tags
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    FGameplayTag DestructibleType;

    // Instance Id
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    int32 InstanceId;

    void PreReplicatedRemove(const struct FTrackedDestructibleDataArray &InArraySerializer);
    void PostReplicatedAdd(const struct FTrackedDestructibleDataArray &InArraySerializer);
    void PostReplicatedChange(const struct FTrackedDestructibleDataArray &InArraySerializer);
};

USTRUCT(BlueprintType)
struct FTrackedDestructibleDataArray : public FFastArraySerializer {
    GENERATED_BODY()
    UPROPERTY()
    TArray<FTrackedDestructibleData> Items;

    /** Step 4: Copy this, replace example with your names */
    bool NetDeltaSerialize(FNetDeltaSerializeInfo &DeltaParms) {
        return FastArrayDeltaSerialize<FTrackedDestructibleData, FTrackedDestructibleDataArray>(Items, DeltaParms, *this);
    }
};

template <>
struct TStructOpsTypeTraits<FTrackedDestructibleDataArray> : TStructOpsTypeTraitsBase2<FTrackedDestructibleDataArray> {
    enum {
        WithNetDeltaSerializer = true,
    };
};


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicDestructiblesManagerComponent : public UActorComponent {
    GENERATED_BODY()

    // Fast Array Serializer
    UPROPERTY(ReplicatedUsing=OnRep_TrackedResources)
    FTrackedDestructibleDataArray TrackedResources;

    // OnRep_TrackedResources
    UFUNCTION()
    void OnRep_TrackedResources() const;

public:
    // Sets default values for this component's properties
    UMythicDestructiblesManagerComponent();

    // Add a resource to the tracked list
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void AddOrUpdateResource(AActor *Actor, FTransform Transform, int32 HitsTillDestruction, FGameplayTag DestructibleTag, int32 InstanceId);

    // Remove a resource from the tracked list
    UFUNCTION(BlueprintCallable, Category = "Resource", BlueprintAuthorityOnly)
    void RemoveResource(AActor *Actor, FTransform Transform, int32 HitsTillDestruction);

protected:
    // Called when the game starts
    virtual void BeginPlay() override;

public:
    // Lifetime replication
    virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const override;

    TArray<FTrackedDestructibleData> GetTrackedDestructibles() const;
};
