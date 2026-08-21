
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/Creatures/CreatureAggressionTypes.h"
#include "CreatureEcologyProcessor.generated.h"

UCLASS()
class MYTHIC_API UMythicCreatureEcologyProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicCreatureEcologyProcessor();

    static float ComputeTerritorialAggression(float BaseAggression, bool bNearDen, float TerritorialBoost);

    static void BuildAggressionMatrix(const class UDataTable *Table, FMythicCreatureAggressionMatrix &OutMatrix);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery CreatureQuery;

    FMassEntityQuery HydratedCreatureQuery;

    float TimeSinceLastTick = 0.0f;

    FMythicCreatureAggressionMatrix AggressionMatrix;

    bool bAggressionMatrixResolved = false;
};
