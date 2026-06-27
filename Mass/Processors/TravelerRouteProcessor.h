// Mythic Living World — Traveler Route Processor
// MASS processor that advances inter-settlement travelers one cell per step toward their destination, keeping the
// schedule WorkCell one step ahead (so the embodied cognitive brain's FollowSchedule desire walks the AIController to
// the next cell) and despawning them on arrival. The lifecycle counterpart to UMythicTravelerSpawnerProcessor.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "TravelerRouteProcessor.generated.h"

class UMythicLivingWorldSubsystem;
class UMythicTerritoryGrid;

/**
 * MASS processor that drives traveler movement + arrival.
 *
 * Each throttled tick (TravelerStepIntervalSeconds), for every traveler entity (FMythicTravelerTag +
 * FMythicTravelerFragment):
 *  - ARRIVED (Identity.Cell == DestinationCell) OR StepsRemaining == 0 → despawn: tear down any embodied actor first
 *    (FindEmbodiedActor->Destroy + UnregisterEmbodiedActor), then Defer().DestroyEntity. Verbatim the population/
 *    creature spawner despawn idiom — travelers are EXEMPT from the population spawner's far-despawn (TravelerTag), so
 *    THIS processor is their sole despawn authority.
 *  - EN ROUTE → compute the next greedy step (StepToward), set Schedule.WorkCell = next + Schedule.Phase = Work (so
 *    FollowSchedule (0.7) keeps routing the AIController forward), and decrement StepsRemaining. If the traveler is NOT
 *    embodied (no actor owns its position), dead-reckon Identity.Cell forward to the next step. If it IS embodied, the
 *    AIController::RefreshLiveCell move-loop is the SOLE Identity.Cell writer — this processor only nudges WorkCell, so
 *    there is never a double-writer in one frame.
 *
 * MUST ExecuteAfter UMythicScheduleTransitionProcessor: that processor stamps Phase from the time of day every tick, so
 * if it ran AFTER this one it would overwrite the traveler's Work phase with (e.g.) Rest at night and the traveler
 * would stop moving. Running after it, the route processor RE-STAMPS Phase = Work each step, keeping travelers moving
 * around the clock regardless of the town's daily routine.
 *
 * PrePhysics, Server|Standalone, game thread. Identity.Cell is read-only via copy for the despawn proximity check; the
 * fragment writes (WorkCell/Phase/StepsRemaining, and Identity.Cell only when NOT embodied) are ReadWrite.
 */
UCLASS()
class MYTHIC_API UMythicTravelerRouteProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicTravelerRouteProcessor();

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /**
     * Traveler route query: Identity (RW — dead-reckon Cell when not embodied), Schedule (RW — WorkCell/Phase),
     * Traveler (RW — StepsRemaining/TimeSinceStep), gated to FMythicTravelerTag (All).
     */
    FMassEntityQuery TravelerQuery;

    /** Timer accumulator — runs at TravelerStepIntervalSeconds, not every frame. */
    float TimeSinceLastTick = 0.0f;
};
