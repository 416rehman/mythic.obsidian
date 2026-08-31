#pragma once

#include "NativeGameplayTags.h"

/** Parent namespace for every executable contextual action identity. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_ROOT);

/** Reserved child namespace for viewer-safe unavailable explanation identities. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_ROOT);

/** Parent namespace for CommonUI action bindings used by contextual action definitions. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_INPUT_ROOT);

/** Generic safe rejection when the presentation embodiment no longer resolves. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_INVALID_TARGET);

/** Generic safe rejection when an owner-only action offer or its revision is no longer current. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_STALE);

/** Safe rejection when the viewer is outside the action definition's authoritative range. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_OUT_OF_RANGE);

/** Safe rejection when the authoritative interaction trace is obstructed. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_OBSTRUCTED);

/** Safe rejection when the action requires deliberate focus that the server cannot currently prove. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_NOT_FOCUSED);

/** Generic safe rejection used when the provider intentionally exposes no more specific explanation. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(CONTEXT_ACTION_REASON_UNAVAILABLE);
