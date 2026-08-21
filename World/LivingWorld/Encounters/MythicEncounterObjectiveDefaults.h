
#pragma once

#include "CoreMinimal.h"

class UObjectiveDefinition;

namespace MythicEncounterObjectiveDefaults {
MYTHIC_API UObjectiveDefinition *BuildDefaultEncounterClearObjective(class UObject *Outer, int32 RequiredKills);
}
