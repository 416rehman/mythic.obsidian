
#pragma once

#include "CoreMinimal.h"

namespace MythicCombat {
FORCEINLINE bool RollSucceeds(float Probability, float Roll) {
    return Probability > 0.0f && Roll <= Probability;
}

/**
 * Bounds a probability read from a captured attribute. That magnitude is the raw aggregator sum — the 0..1 clamp
 * on the attribute only binds its current value, so a stacked chance reaches a roll far above 1 and succeeds every
 * time while the character sheet still reads 100%. MaxChance is itself bounded to 1.
 */
FORCEINLINE float ClampProbability(float Probability, float MaxChance = 1.0f) {
    return FMath::Clamp(Probability, 0.0f, FMath::Clamp(MaxChance, 0.0f, 1.0f));
}
}
