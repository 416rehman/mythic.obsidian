
#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicTravelerFragment.generated.h"

USTRUCT()
struct MYTHIC_API FMythicTravelerFragment : public FMassFragment {
    GENERATED_BODY()

    FMythicCellCoord OriginCell;

    FMythicCellCoord DestinationCell;

    int32 DestinationSettlementId = INDEX_NONE;

    uint8 Kind = 0;

    float TimeSinceStepSeconds = 0.0f;

    uint16 StepsRemaining = 0;
};
