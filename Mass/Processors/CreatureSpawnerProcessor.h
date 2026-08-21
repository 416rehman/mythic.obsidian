
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "CreatureSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
struct FMythicCellCoord;

UCLASS()
class MYTHIC_API UMythicCreatureSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicCreatureSpawnerProcessor();

    static int32 ComputeCreatureTargetDensity(int32 MaxCreaturesPerBiomeCell, float DensityScale, int32 SystemCap);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery ExistingCreatureQuery;

    float TimeSinceLastTick = 0.0f;

    uint32 SpawnCounter = 0;

    uint16 NextPackId = 1;

    uint16 AllocatePackId();
};
