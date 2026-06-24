// Mythic Living World — Schedule Transition Processor
// Timer-driven MASS processor: transitions NPC schedule phases based on game time of day.

#pragma once

#include "CoreMinimal.h"
#include "MassProcessor.h"
#include "MassEntityQuery.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "ScheduleTransitionProcessor.generated.h"

class UMythicLivingWorldSettings;

/**
 * MASS processor that drives NPC daily schedule phase transitions.
 *
 * Reads the current game time-of-day and transitions all NPC schedule fragments
 * to the appropriate phase (Work, Travel, Rest, Social, Idle).
 *
 * Schedule layout (24h game time cycle):
 *   06:00 - 08:00  Travel (home → work)
 *   08:00 - 14:00  Work
 *   14:00 - 15:00  Travel (work → social)
 *   15:00 - 18:00  Social
 *   18:00 - 19:00  Travel (social → home)
 *   19:00 - 06:00  Rest
 *
 * Timer-driven (default 2s) — not per-frame. Schedule transitions are a visual
 * concern; exact timing isn't critical.
 *
 * Event disruption: If an entity's significance is dirty (it witnessed an event),
 * the schedule transition is skipped — the entity stays in its current phase until
 * the event is resolved (pressure vented). This implements the requirement that
 * events disrupt daily schedules.
 *
 * Cost: O(N) over all entities, but timer-throttled and extremely cheap per entity
 * (one enum comparison + one enum write).
 */
UCLASS()
class MYTHIC_API UMythicScheduleTransitionProcessor : public UMassProcessor {
    GENERATED_BODY()

public:
    UMythicScheduleTransitionProcessor();

    /**
     * Determine the schedule phase for a given hour of the game day, using the designer-tunable hour boundaries
     * (UMythicLivingWorldSettings::Schedule*Hour). Static + Settings-parameterized so it stays unit-testable.
     * @param GameHour  Hour of the day [0.0, 24.0)
     * @param Settings  Living-world settings supplying the phase boundaries (null → Rest, a safe default)
     * @return Target schedule phase for this time of day
     */
    static EMythicSchedulePhase GetPhaseForHour(float GameHour, const UMythicLivingWorldSettings *Settings);

    /**
     * Apply a deterministic per-NPC schedule offset so the town doesn't transition every NPC's phase in lockstep.
     * Returns the effective hour = GameHour shifted by an offset in [-MaxStaggerHours, +MaxStaggerHours] derived from
     * the stable NameHash, wrapped into [0.0, 24.0). MaxStaggerHours <= 0 → returns GameHour unchanged (global sync).
     * Pure + static so the offset rule is unit-testable.
     */
    static float ComputeStaggeredHour(float GameHour, uint32 NameHash, float MaxStaggerHours);

protected:
    virtual void ConfigureQueries(const TSharedRef<FMassEntityManager> &EntityManager) override;
    virtual void Execute(FMassEntityManager &EntityManager, FMassExecutionContext &Context) override;

private:
    /** Query for all entities with schedule + identity + significance fragments */
    FMassEntityQuery ScheduleQuery;

    /** Timer accumulator — transitions run at configured interval */
    float TimeSinceLastTick = 0.0f;
};
