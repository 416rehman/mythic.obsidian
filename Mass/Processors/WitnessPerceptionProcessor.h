
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "World/LivingWorld/MythicCellSpatialIndex.h"
#include "WitnessPerceptionProcessor.generated.h"

class UMythicActionEventSubsystem;

UCLASS()
class MYTHIC_API UMythicWitnessPerceptionProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicWitnessPerceptionProcessor();

    static float ComputeStealthPerceptionRange(float BaseRange, float StealthScale);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery AllEntitiesQuery;

    TWeakObjectPtr<UMythicActionEventSubsystem> CachedActionSubsystem;

    FMythicCellSpatialIndex SpatialIndex;

    TArray<FMassEntityHandle> WitnessCandidates;
};
