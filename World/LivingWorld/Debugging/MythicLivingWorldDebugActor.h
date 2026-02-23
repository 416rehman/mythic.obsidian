// Mythic Living World — Debug Visualization Actor
// Draws debug shapes for territory cells, factions, and settlements.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "MythicLivingWorldDebugActor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;
class UMythicSettlementRegistry;

UCLASS()
class MYTHIC_API AMythicLivingWorldDebugActor : public AActor {
    GENERATED_BODY()

public:
    AMythicLivingWorldDebugActor();

    virtual void Tick(float DeltaTime) override;
    virtual void BeginPlay() override;

private:
    /** Draw territory grid cells around the player */
    void DrawTerritoryGrid(const UMythicTerritoryGrid *Grid, const UMythicFactionDatabase *FactionDB, const FVector &ViewLocation);

    /** Draw settlement boundaries and info */
    void DrawSettlements(const UMythicSettlementRegistry *Registry, const UMythicFactionDatabase *FactionDB);

    /** Get a deterministic color for a faction ID */
    FColor GetFactionColor(uint8 FactionIndex) const;

    /** Radius around view location to draw cells (in cm) */
    float DrawRadius = 15000.0f; // 150m radius

    /** Cached subsystem reference */
    UPROPERTY()
    TWeakObjectPtr<UMythicLivingWorldSubsystem> LivingWorldSubsystem;

    /** Mesh to use for territory cells */
    UPROPERTY(EditAnywhere, Category = "Visualization")
    TObjectPtr<UStaticMesh> CellMesh;

    /** Base material for territory cells (must support Color parameter) */
    UPROPERTY(EditAnywhere, Category = "Visualization")
    TObjectPtr<UMaterialInterface> CellMaterial;

private:
    /** Pool of ISMs per faction index to handle coloring */
    UPROPERTY()
    TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> FactionISMs;

    /** Update or create ISM for a specific faction */
    UInstancedStaticMeshComponent *GetOrCreateFactionISM(int32 FactionIndex, const FColor &Color);
};
