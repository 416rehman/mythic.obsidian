
#pragma once

#include "CoreMinimal.h"

namespace MythicCombat {
/** Highest chance a diminished roll can reach. Below 1 so that no amount of stacking makes an outcome certain. */
static constexpr float MaxEffectiveProbability = 1.0f - KINDA_SMALL_NUMBER;

FORCEINLINE bool RollSucceeds(float Probability, float Roll) {
    return Probability > 0.0f && Roll <= Probability;
}

/**
 * Bounds a probability read from a captured attribute. That magnitude is the raw aggregator sum — the 0..1 clamp
 * on the attribute only binds its current value, so a stacked chance reaches a roll far above 1 and succeeds every
 * time while the character sheet still reads 100%. MaxChance is itself bounded to 1.
 */
/**
 * Bends a stacked chance so it approaches certainty without ever arriving.
 *
 * Below SoftCap nothing happens, so ordinary values stay exactly what they were authored as. Above it each further
 * point buys a little less than the last, and the curve tends to 1 as the raw chance tends to infinity. So stacking
 * always pays something and never pays everything: there is no amount of gear that removes the roll.
 */
FORCEINLINE float DiminishProbability(float Probability, float SoftCap) {
    const float Raw = FMath::Max(0.0f, Probability);
    const float Soft = FMath::Clamp(SoftCap, 0.0f, 1.0f);
    if (Raw <= Soft) {
        return Raw;
    }
    const float Headroom = 1.0f - Soft;
    if (Headroom <= KINDA_SMALL_NUMBER) {
        return Soft;
    }
    const float Curved = Soft + Headroom * (1.0f - FMath::Exp(-(Raw - Soft) / Headroom));

    // The curve only approaches 1, but a float cannot hold the difference for long: past roughly ten times the
    // headroom the exponential rounds to zero and the result becomes exactly 1, which would delete the roll. The
    // ceiling keeps a real chance of failure at any amount of stacking, which is the whole point of the curve.
    return FMath::Min(Curved, MaxEffectiveProbability);
}

FORCEINLINE float ClampProbability(float Probability, float MaxChance = 1.0f) {
    return FMath::Clamp(Probability, 0.0f, FMath::Clamp(MaxChance, 0.0f, 1.0f));
}
}
