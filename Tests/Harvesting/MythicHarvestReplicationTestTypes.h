#pragma once

#include "GameModes/GameState/MythicGameState.h"

#include "MythicHarvestReplicationTestTypes.generated.h"

/** Hidden concrete fixture for the production abstract GameState presentation coordinator. */
UCLASS(NotBlueprintable, Hidden)
class AMythicHarvestReplicationTestGameState final : public AMythicGameState {
    GENERATED_BODY()
};
