// Mythic GAS — shared combat probability roll
// Single source of truth for the chance-roll boundary rule used across the damage executions (crit/status procs in
// MythicDamageCalculation; dodge + per-status resistance survival in MythicDamageApplication). Pure + header-only so the
// decision is unit-testable without a live ASC; the random sample is taken at the call site and passed in.

#pragma once

#include "CoreMinimal.h"

namespace MythicCombat {
/**
 * True iff a probabilistic effect with the given Probability succeeds on the given [0,1] Roll sample.
 *
 * Two boundary invariants (both were real bugs before the guards existed):
 *  - A Probability <= 0 NEVER succeeds — even on an exact-0.0 roll. A bare `Roll <= Probability` let a 0% chance fire
 *    when FMath::FRand() returned exactly 0.0.
 *  - A Probability >= 1 ALWAYS succeeds — even on an exact-1.0 roll. FMath::FRand() can return 1.0 inclusive, so the
 *    comparison must be `<=`, not `<`, or a guaranteed proc would wrongly drop.
 *
 * Roll is the caller's sample (typically FMath::FRand()); kept out of here so this stays deterministic + testable.
 */
FORCEINLINE bool RollSucceeds(float Probability, float Roll) {
    return Probability > 0.0f && Roll <= Probability;
}
} // namespace MythicCombat
