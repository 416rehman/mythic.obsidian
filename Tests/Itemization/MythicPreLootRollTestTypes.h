#pragma once

#include "GameModes/GameState/MythicGameState.h"
#include "MythicPreLootRollTestTypes.generated.h"

/** Concrete stand-in for the abstract production GameState, which SpawnActor refuses. Test-only. */
UCLASS(NotBlueprintable, Hidden)
class AMythicPreLootRollTestGameState final : public AMythicGameState {
    GENERATED_BODY()
};
