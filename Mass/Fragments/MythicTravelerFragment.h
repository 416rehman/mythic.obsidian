// Mythic Living World — MASS ECS Traveler Fragment
// Per-traveler route state for inter-settlement caravans/patrols. Present only on entities that also carry
// FMythicTravelerTag + FMythicNPCTag. Pure data — the route processor reads/advances it.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicTravelerFragment.generated.h"

/**
 * Route state for an inter-settlement traveler. The traveler walks a deterministic greedy path from OriginCell to
 * DestinationCell one cell per step. StepsRemaining is a hard route cap so a traveler can never loop forever; on
 * arrival (or cap reached) the route processor despawns it.
 */
USTRUCT()
struct MYTHIC_API FMythicTravelerFragment : public FMassFragment {
    GENERATED_BODY()

    /** Cell the traveler departed from (origin settlement center). */
    FMythicCellCoord OriginCell;

    /** Cell the traveler is heading to (destination settlement center). */
    FMythicCellCoord DestinationCell;

    /** Runtime id of the destination settlement (INDEX_NONE if not settlement-bound). */
    int32 DestinationSettlementId = INDEX_NONE;

    /** 0 = Caravan/merchant, 1 = Patrol/soldier escort. Selects the role tag + relationship preference at spawn. */
    uint8 Kind = 0;

    /** Accumulated time since the last cell step (throttles movement to TravelerStepIntervalSeconds). */
    float TimeSinceStepSeconds = 0.0f;

    /** Remaining steps before forced despawn = manhattan(Origin,Dest) + slack. Decremented per step. */
    uint16 StepsRemaining = 0;
};
