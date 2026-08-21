
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "BeliefPropagationProcessor.generated.h"

UCLASS()
class MYTHIC_API UMythicBeliefPropagationProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicBeliefPropagationProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery HydratedSocialQuery;

    float TimeSinceLastTick = 0.0f;
};
