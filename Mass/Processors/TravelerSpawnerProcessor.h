
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "TravelerSpawnerProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;
class UMythicFactionDatabase;

UCLASS()
class MYTHIC_API UMythicTravelerSpawnerProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicTravelerSpawnerProcessor();

    static FMythicCellCoord StepToward(FMythicCellCoord From, FMythicCellCoord To);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery ActiveTravelerQuery;

    float TimeSinceLastTick = 0.0f;
};
