
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
 * Bends a stacked value so it approaches a ceiling without ever arriving.
 *
 * Below SoftCap nothing happens, so ordinary values stay exactly what they were authored as. Above it each further
 * point buys a little less than the last, and the curve tends to Ceiling as the raw value tends to infinity. So
 * stacking always pays something and never pays everything.
 *
 * This is the one diminishing curve in the game. Chance, damage, duration and status magnitude all pass through it;
 * only the soft cap and ceiling differ, and both are authored per stat rather than fixed here.
 */
FORCEINLINE float Diminish(float Raw, float SoftCap, float Ceiling) {
    const float Value = FMath::Max(0.0f, Raw);
    const float Top = FMath::Max(0.0f, Ceiling);
    const float Soft = FMath::Clamp(SoftCap, 0.0f, Top);
    if (Value <= Soft) {
        return Value;
    }
    const float Headroom = Top - Soft;
    if (Headroom <= KINDA_SMALL_NUMBER) {
        return Soft;
    }
    const float Curved = Soft + Headroom * (1.0f - FMath::Exp(-(Value - Soft) / Headroom));

    // The curve only approaches the ceiling, but a float cannot hold the difference for long: past roughly ten
    // times the headroom the exponential rounds to zero and the result becomes exactly the ceiling. For a chance
    // that would delete the roll, which is the one thing the curve exists to prevent, so it is held just below.
    return FMath::Min(Curved, Top * (1.0f - KINDA_SMALL_NUMBER));
}

/** The chance case of Diminish: a ceiling of 1, so no amount of stacking makes an outcome certain. */
FORCEINLINE float DiminishProbability(float Probability, float SoftCap) {
    return Diminish(Probability, SoftCap, 1.0f);
}

FORCEINLINE float ClampProbability(float Probability, float MaxChance = 1.0f) {
    return FMath::Clamp(Probability, 0.0f, FMath::Clamp(MaxChance, 0.0f, 1.0f));
}
}
