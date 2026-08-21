
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TravelerRouteProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;

UCLASS()
class MYTHIC_API UMythicTravelerRouteProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicTravelerRouteProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery TravelerQuery;

    float TimeSinceLastTick = 0.0f;
};
