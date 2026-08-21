
#pragma once

#include "CoreMinimal.h"

namespace MythicCombat {
FORCEINLINE bool RollSucceeds(float Probability, float Roll) {
    return Probability > 0.0f && Roll <= Probability;
}
}
