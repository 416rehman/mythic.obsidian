
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
    void DrawTerritoryGrid(const UMythicTerritoryGrid *Grid, const UMythicFactionDatabase *FactionDB, const FVector &ViewLocation);

    void DrawSettlements(const UMythicSettlementRegistry *Registry, const UMythicFactionDatabase *FactionDB);

    FColor GetFactionColor(uint8 FactionIndex) const;

    float DrawRadius = 15000.0f;

    UPROPERTY()
    TWeakObjectPtr<UMythicLivingWorldSubsystem> LivingWorldSubsystem;

    /** Mesh to use for territory cells */
    UPROPERTY(EditAnywhere, Category = "Visualization")
    TObjectPtr<UStaticMesh> CellMesh;

    /** Base material for territory cells (must support Color parameter) */
    UPROPERTY(EditAnywhere, Category = "Visualization")
    TObjectPtr<UMaterialInterface> CellMaterial;

private:
    UPROPERTY()
    TMap<int32, TObjectPtr<UInstancedStaticMeshComponent>> FactionISMs;

    UInstancedStaticMeshComponent *GetOrCreateFactionISM(int32 FactionIndex, const FColor &Color);
};
