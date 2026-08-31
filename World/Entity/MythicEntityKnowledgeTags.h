#pragma once

#include "NativeGameplayTags.h"

/** Root for semantic facts a player has actually learned about a canonical entity. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_ROOT);

/** Root for discovered, player-safe personality and behavioral traits. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_TRAIT_ROOT);

/** Root for discovered, player-safe history beats and witnessed deeds. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_HISTORY_ROOT);

/** Root for learned preferences; the root itself is not a displayable fact. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_PREFERENCE_ROOT);

/** Root for things the player has learned that an entity likes. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_LIKES_ROOT);

/** Root for things the player has learned that an entity dislikes. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_DISLIKES_ROOT);

/** Root for player-learned social ties and connections. */
MYTHIC_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(ENTITY_KNOWLEDGE_CONNECTION_ROOT);
