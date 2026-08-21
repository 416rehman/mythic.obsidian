
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TerritoryPatrolSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;
struct FMythicCellCoord;
enum class EMythicBiome : uint8;

UCLASS()
class MYTHIC_API UMythicTerritoryPatrolSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicTerritoryPatrolSpawnerProcessor();

    static int32 ComputeTerritoryPatrolDensity(float MilitaryStrength, float Influence, int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell);

    static float BiomeGarrisonModifier(EMythicBiome Biome);

    static int32 ApplyContestedBorderBoost(int32 BaseSoldierTarget, bool bContested, float ContestedMultiplier,
                                           int32 MaxSoldiersPerControlledCell, int32 MaxEntitiesPerCell);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery ExistingNPCQuery;

    float TimeSinceLastTick = 0.0f;
};
