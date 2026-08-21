
#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "ScheduleTransitionProcessor.generated.h"

class UMythicLivingWorldSettings;

UCLASS()
class MYTHIC_API UMythicScheduleTransitionProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicScheduleTransitionProcessor();

    static EMythicSchedulePhase GetPhaseForHour(float GameHour, const UMythicLivingWorldSettings *Settings);

    static float ComputeStaggeredHour(float GameHour, uint32 NameHash, float MaxStaggerHours);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    FMassEntityQuery ScheduleQuery;

    float TimeSinceLastTick = 0.0f;
};
