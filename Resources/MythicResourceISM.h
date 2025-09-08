// 

#pragma once

#include "CoreMinimal.h"
#include "Destructible.h"
#include "GameplayTagContainer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "MythicResourceISM.generated.h"

struct FTrackedDestructibleData;
class UMythicResourceManagerComponent;

// Health configuration structure for each destructible type
USTRUCT(BlueprintType)
struct FDestructibleHealthConfig {
    GENERATED_BODY()

    // Health points per unit of Z scale
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    float HealthPerZUnit = 1.0f;

    // Minimum health regardless of Z
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 MinHealth = 1;

    // Maximum health (0 = no cap)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Health")
    int32 MaxHealth = 0;

    FDestructibleHealthConfig() {
        HealthPerZUnit = 6.0f;
        MinHealth = 1;
        MaxHealth = 6;
    }
};

/**
 * Class to be used to represent a resource that can be mined. I.e stone, tree, ore, etc.
 * The PCG should have the collision set to destructible on the Mesh Spawner, and the "Use Default Collision" should be unchecked.
 */
UCLASS()
class MYTHIC_API UMythicResourceISM : public UInstancedStaticMeshComponent, public IDestructible {
    // IDestructible Interface
public:
    virtual FRewardsToGive GetOnKillRewards(AActor *Killer = nullptr) override;
    // End IDestructible Interface

private:
    GENERATED_BODY()

    // Track destroyed instances to prevent double destruction
    UPROPERTY()
    TSet<int32> DestroyedInstances;
    
public:
    // Check if an instance is already destroyed
    UFUNCTION(BlueprintCallable)
    bool IsInstanceDestroyed(int32 InstanceIndex) const {
        return DestroyedInstances.Contains(InstanceIndex);
    }

public:
    // BeginPlay
    virtual void BeginPlay() override;
    
    // Health Configuration for this resource type
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Health", meta = (ShowOnlyInnerProperties))
    FDestructibleHealthConfig HealthConfig = FDestructibleHealthConfig();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
    FRewardsToGive OnKillRewards;

    // This tag is used to bin all resources of this type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Categories="Destructible"))
    FGameplayTag ResourceType;

    // Convert Instance Index to Instance Id
    UFUNCTION(BlueprintCallable)
    FPrimitiveInstanceId InstanceIndexToId(int32 InstanceIndex) {
        return this->PrimitiveInstanceDataManager.IndexToId(InstanceIndex);
    }

    // Get maximum health based on transform
    int32 CalculateHealthFromTransform(const FTransform &Transform) const;

    // Destroy Instance - Hides and disables collision
    UFUNCTION()
    void DestroyResource(int32 InstanceId);

    UFUNCTION()
    void RestoreResource(int32 InstanceIndex, FTransform OriginalTransform, bool MarkRenderStateDirty);
};
