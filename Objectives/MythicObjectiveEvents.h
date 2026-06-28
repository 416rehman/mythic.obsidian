// Mythic — shared pure gate for emitting objective-advancing GAS events (reach-location, talk-to-NPC, …).
// The actual emit (resolve ASC, build FGameplayEventData, HandleGameplayEvent) lives at each call site; this is just the
// common decision so it is unit-testable in one place.

#pragma once

#include "CoreMinimal.h"

namespace MythicObjectiveEvents {
    // Emit an objective-advancing GAS event only when this is the server-authoritative context for the player AND there
    // is a valid payload tag to identify the objective (an invalid/empty tag means "this thing isn't an objective
    // trigger"). Header-only inline — trivial, but the single documented contract for every objective-event emitter.
    FORCEINLINE bool ShouldEmitObjectiveEvent(bool bServerAuthoritativeForPlayer, bool bValidPayloadTag) {
        return bServerAuthoritativeForPlayer && bValidPayloadTag;
    }
}
