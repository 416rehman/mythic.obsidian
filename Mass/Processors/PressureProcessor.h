
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "PressureProcessor.generated.h"

class UMythicActionEventSubsystem;

UCLASS()
class MYTHIC_API UMythicPressureProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicPressureProcessor();

    static bool ComputeDespairState(float TotalPressure, float DespairThreshold, bool bWasDespaired);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery HydratedEntityQuery;

    TWeakObjectPtr<UMythicActionEventSubsystem> CachedActionSubsystem;
};
