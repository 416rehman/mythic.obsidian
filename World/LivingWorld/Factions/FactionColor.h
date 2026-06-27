// Mythic Living World — Faction Color
// Deterministic-from-id faction color with an authorable override. Kept in its own small header/cpp (NOT inlined into
// LivingWorldTypes.h) so the faction-color concern is local and a hue tweak does not recompile the whole world. The
// war-map, the NPC appearance resolver (FMythicAppearanceResolver), and any UI all consume the SAME accessor so a
// faction's color is identical everywhere it appears.

#pragma once

#include "CoreMinimal.h"

struct FMythicFactionData;

// ─────────────────────────────────────────────────────────────
// Faction color — deterministic golden-ratio hue, override-aware
// ─────────────────────────────────────────────────────────────

namespace MythicFactionColor {
    /**
     * Deterministic, hue-separated color for a faction index. Uses the golden-ratio conjugate to walk the hue wheel so
     * consecutive faction indices land far apart on the color wheel (maximally distinguishable at a glance on the war-map
     * / minimap). Pure + stable: the same index always produces the same FColor, on any thread, with no engine state.
     *
     * @param FactionIndex  The faction's database index (FMythicFactionId::Index).
     * @return Opaque, fully-saturated FColor for that index.
     */
    MYTHIC_API FColor DeterministicColorForId(uint8 FactionIndex);

    /**
     * Resolved display color for a faction: the authored override when bOverrideFactionColor is set, otherwise the
     * deterministic-from-id color. FactionIndex is passed explicitly because FMythicFactionData carries no index field —
     * the caller (which already holds the FMythicFactionId) supplies it.
     *
     * @param Data          Faction snapshot (read for the override flag + override color only).
     * @param FactionIndex  The faction's database index, used for the deterministic fallback.
     * @return The faction's resolved display color.
     */
    MYTHIC_API FColor GetFactionColor(const FMythicFactionData& Data, uint8 FactionIndex);
} // namespace MythicFactionColor
