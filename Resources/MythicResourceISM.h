
#pragma once

#include "CoreMinimal.h"
#include "Destructible.h"
#include "GameplayTagContainer.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "MythicResourceISM.generated.h"

struct FTrackedDestructibleData;
class UMythicResourceManagerComponent;

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

UCLASS()
class MYTHIC_API UMythicResourceISM : public UInstancedStaticMeshComponent, public IDestructible {
public:
    virtual FRewardsToGive GetOnKillRewards(AActor *Killer = nullptr) override;

private:
    GENERATED_BODY()

    UPROPERTY()
    TSet<int32> DestroyedInstances;

public:
    // Check if an instance is already destroyed
    UFUNCTION(BlueprintCallable)
    bool IsInstanceDestroyed(int32 InstanceIndex) const {
        return DestroyedInstances.Contains(InstanceIndex);
    }

public:
    virtual void BeginPlay() override;

    // Health Configuration for this resource type
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Destructible|Health", meta = (ShowOnlyInnerProperties))
    FDestructibleHealthConfig HealthConfig = FDestructibleHealthConfig();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource")
    FRewardsToGive OnKillRewards;

    // This tag is used to bin all resources of this type
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource", meta = (Categories="Destructible"))
    FGameplayTag ResourceType;

    // GATHERING DEPTH (Part A) — required tool tag to gather this node. EMPTY (default) = any tool (or none) gathers it,
    // preserving today's behaviour. When set, the manager gates gathering via FMythicGatherRules::CanGather against the
    // interactor's equipped-tool type-probe (see World/Gathering/MythicGatherRules.h for the documented owner one-liner).
    // Inert data until the manager reads it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Gathering")
    FGameplayTag RequiredToolTag;

    // GATHERING DEPTH (Part A) — resource tier index (0 = default/common). Higher tiers drop more (TierYieldMultiplier)
    // and respawn slower (ScaledRespawnDelay) via FMythicGatherRules. Inert data until the manager reads it.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Resource|Gathering")
    int32 ResourceTier = 0;

    // Convert Instance Index to Instance Id
    UFUNCTION(BlueprintCallable)
    FPrimitiveInstanceId InstanceIndexToId(int32 InstanceIndex) {
        return this->PrimitiveInstanceDataManager.IndexToId(InstanceIndex);
    }

    int32 CalculateHealthFromTransform(const FTransform &Transform) const;

    UFUNCTION()
    void DestroyResource(int32 InstanceId);

    UFUNCTION()
    void RestoreResource(int32 InstanceIndex, FTransform OriginalTransform, bool MarkRenderStateDirty);
};
