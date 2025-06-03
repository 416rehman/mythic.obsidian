// 

#pragma once

#include "CoreMinimal.h"
#include "Destructible.h"
#include "GameplayTagContainer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "MythicResourceStaticMeshComponent.generated.h"

struct FTrackedDestructibleData;
class UMythicDestructiblesManagerComponent;

USTRUCT(BlueprintType)
struct FResourceInstanceData {
    GENERATED_BODY()

    // Remaining hits required to mine this resource
    UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Resource")
    int32 HitsTillDestruction = 1;
};

/**
 * Class to be used to represent a resource that can be mined. I.e stone, tree, ore, etc.
 * The PCG should have the collision set to destructible on the Mesh Spawner, and the "Use Default Collision" should be unchecked.
 */
UCLASS()
class MYTHIC_API UMythicResourceStaticMeshComponent : public UInstancedStaticMeshComponent, public IDestructible {
    GENERATED_BODY()

public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
    FRewardsToGive OnKillRewards;

    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Resource")
    int InitializeHitsTillDestruction(float Z);
    int InitializeHitsTillDestruction_Implementation(float Z);

    void SetHitsTillDestruction(int32 InstanceIndex, int32 NewHitsTillDestruction, FTransform Transform);
    const FResourceInstanceData *GetResourceInstanceData(int32 InstanceIndex, FTransform Transform);

    UFUNCTION(BlueprintCallable)
    static FHitResult SynthesizeHitResult(FVector ImpactPoint, FVector TraceEnd, UWorld *World, AActor *DamageCauser);

    // This tag is used to bin all resources of this type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Categories="Destructible"))
    FGameplayTag ResourceType;

    // Convert Instance Index to Instance Id
    UFUNCTION(BlueprintCallable)
    int32 InstanceIndexToId(int32 InstanceIndex) {
        return this->PrimitiveInstanceDataManager.IndexToId(InstanceIndex).Id;
    }

private:
    virtual FRewardsToGive GetOnKillRewards(AActor *Killer = nullptr) override;
    virtual int CalculateHitsLeft(int32 InstanceIndex, AActor *DamageCauser, float Damage) override;
    virtual FGameplayTag GetDestructibleType() override;

    // Keeps track of the data for each instance of the resource
    TMap<int32, FResourceInstanceData> ResourceData;
};


/// Returns failed to sync destructible data so it can be retried later if needed
void SyncResourcesState(const TArray<FTrackedDestructibleData> &TrackedResources);
