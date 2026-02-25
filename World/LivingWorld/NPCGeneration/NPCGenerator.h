// Mythic Living World — NPC Generation Utility
// Procedural NPC identity generation from faction culture data.
// Hash-seeded, deterministic — same seed produces same NPC across save/load.

#pragma once

#include "CoreMinimal.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "Mass/Fragments/MythicMassFragments.h"

/**
 * Static utility class for procedural NPC generation.
 * All generation is deterministic — seeded by NameHash so the same entity
 * produces the same identity across sessions and save/load cycles.
 *
 * Generation pipeline:
 * 1. Name — syllable-table concatenation from faction culture index + hash seed
 * 2. Demographics — age bracket + gender from faction population profile
 * 3. Personality — VentWeights biased by faction ideology + role modifier + random variance
 * 4. Visual — archetype index from faction culture range
 * 5. Schedule — home/work cell assignment from settlement data
 */
class MYTHIC_API FMythicNPCGenerator {
public:
    // ─── Name Generation ─────────────────────────────────

    /**
     * Generate a deterministic name hash from faction + cell + spawn index.
     * Used for Tier 0-1 entities (hash only, no string allocation).
     * @param FactionIndex    Index of the faction
     * @param Cell            Cell where the entity spawns
     * @param SpawnIndex      Index within the batch spawn
     * @return Deterministic 32-bit name hash
     */
    static uint32 GenerateNameHash(uint8 FactionIndex, const FMythicCellCoord& Cell, int32 SpawnIndex);

    /**
     * Reconstruct a full name string from a name hash + faction index.
     * Used for Tier 2+ promotion — lazy string reconstruction from seed.
     * Deterministic: same hash + faction always produces the same name.
     *
     * @param NameHash        The entity's stored name hash
     * @param FactionIndex    Faction index for culture-specific syllable tables
     * @return Generated name string (e.g., "Kael'dyn", "Morthak", "Lianna")
     */
    static FName ReconstructNameFromHash(uint32 NameHash, uint8 FactionIndex);

    // ─── Demographic Generation ──────────────────────────

    /**
     * Pack demographic flags from a name hash.
     * Deterministic: same hash always produces the same demographics.
     *
     * Layout: [3 bits: age bracket] [1 bit: gender] [4 bits: reserved]
     * Age brackets: 0=child, 1=young, 2=adult, 3=middle, 4=elder, 5-7=reserved
     * Gender: 0=male, 1=female
     *
     * @param NameHash        Seed for deterministic generation
     * @param bHasCivilians   Whether this faction has civilian demographics (affects age distribution)
     * @return Packed demographic flags byte
     */
    static uint8 GenerateDemographicFlags(uint32 NameHash, bool bHasCivilians);

    /**
     * Select a visual archetype index for the entity.
     * Deterministic from hash. Range [0, MaxArchetypes).
     * Different factions can have different archetype counts.
     *
     * @param NameHash        Seed
     * @param MaxArchetypes   Number of visual archetypes available for this faction (default 8)
     * @return Visual archetype index
     */
    static uint8 GenerateVisualArchetype(uint32 NameHash, uint8 MaxArchetypes = 8);

    // ─── Personality Generation ──────────────────────────

    /**
     * Generate personality VentWeights biased by faction ideology.
     * Each vent channel weight is derived from:
     * 1. Faction ideology baseline (e.g., high Authority → higher Enforce weight)
     * 2. Role modifier (guards have boosted Enforce, merchants have boosted Tend)
     * 3. Random variance seeded by NameHash (±0.15 per channel)
     * 4. Outlier chance (5% probability of extreme personality trait)
     *
     * All weights are clamped to [0.0, 1.0] and the distribution is normalized
     * so total VentWeights sum is approximately constant.
     *
     * @param NameHash        Seed for deterministic generation
     * @param Ideology        The faction's current ideology profile
     * @param RoleTag         Optional role tag for role-specific modifiers
     * @return Populated personality fragment
     */
    static FMythicPersonalityFragment GeneratePersonality(
        uint32 NameHash,
        const FMythicIdeologyProfile& Ideology,
        const FGameplayTag& RoleTag = FGameplayTag());

private:
    // ─── Syllable Tables ─────────────────────────────────
    // Faction culture index selects which syllable table to use.
    // Tables are organized by phonetic feel:
    // 0 = Imperial/Latin (hard consonants, formal)
    // 1 = Nordic/harsh (guttural, short)
    // 2 = Elven/flowing (long vowels, soft consonants)
    // 3 = Tribal/primal (short, percussive)
    // 4+ = Generic fantasy

    /** Get a syllable from the table for the given faction culture and position */
    static const TCHAR* GetSyllable(uint8 CultureIndex, int32 SyllableIndex, bool bIsFirst);

    /** Get the number of syllable options for the given culture and position */
    static int32 GetSyllableCount(uint8 CultureIndex, bool bIsFirst);

    /** Simple hash step for deterministic pseudo-random sequences */
    static uint32 HashStep(uint32 Seed);
};
