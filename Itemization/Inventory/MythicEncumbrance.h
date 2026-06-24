#pragma once

#include "CoreMinimal.h"
#include "MythicEncumbrance.generated.h"

/**
 * Encumbrance tier from total carried weight vs the carry capacity. A classic open-world-RPG carry-weight verb:
 * carrying more than your comfortable limit slows you; carrying past the absolute limit nearly immobilizes you.
 */
UENUM(BlueprintType)
enum class EMythicEncumbranceTier : uint8 {
    Unencumbered, // total weight <= soft capacity: no movement penalty
    Heavy,        // soft < weight <= hard: a movement penalty
    Overloaded,   // weight > hard: a heavier penalty (near-immobile)
};

/**
 * Pure encumbrance decisions (no engine state) so the policy is unit-testable headlessly. The aggregation (summing
 * item Weight × stack across an inventory) and the move-speed application (folding the multiplier into the
 * UMythicLifeComponent SpeedScale recompute) are the wiring layers built on top of these.
 */
namespace MythicEncumbrance {
    // The tier for TotalWeight against the soft + hard capacities. A non-positive cap DISABLES that threshold (so
    // unset/zero capacities → always Unencumbered, the default-off behaviour). weight>hard → Overloaded; weight>soft →
    // Heavy; otherwise Unencumbered.
    MYTHIC_API EMythicEncumbranceTier ComputeTier(float TotalWeight, float SoftCapacity, float HardCapacity);

    // The move-speed MULTIPLIER for a tier: Unencumbered → 1; Heavy → HeavyMult; Overloaded → OverloadedMult. Both
    // multipliers are clamped to [0,1] — encumbrance can only slow you, never speed you up.
    MYTHIC_API float SpeedMultiplierForTier(EMythicEncumbranceTier Tier, float HeavyMult, float OverloadedMult);
}
