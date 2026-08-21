
#pragma once

#include "CoreMinimal.h"

struct FMythicFactionData;


namespace MythicFactionColor {
    MYTHIC_API FColor DeterministicColorForId(uint8 FactionIndex);

    MYTHIC_API FColor GetFactionColor(const FMythicFactionData& Data, uint8 FactionIndex);
}
