// Mythic Living World — MASS ECS Fragment Definitions
// Pure data structs for NPC/creature entities. No logic, no UObject overhead.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicMassFragments.generated.h"

// ─────────────────────────────────────────────────────────────
// Identity Fragment — Core identity data for every entity
// Present on all tiers (~32B)
// ─────────────────────────────────────────────────────────────

/**
 * Core identity of a MASS entity. Every NPC and creature has this.
 * Cheap enough to query en-masse for cell-based spatial operations.
 */
USTRUCT()
struct MYTHIC_API FMythicIdentityFragment : public FMassFragment {
    GENERATED_BODY()

    /** Which faction this entity belongs to */
    FMythicFactionId Faction;

    /**
     * True faction identity (for spies/dual-identity NPCs).
     * Defaults to same as Faction. When Faction != TrueFaction, this NPC is undercover.
     * Belief routing goes to TrueFaction handler. Discovery risk from Cunning/Paranoia NPCs.
     */
    FMythicFactionId TrueFaction;

    /** Cell coordinate where this entity currently resides */
    FMythicCellCoord Cell;

    /** Gameplay tag for the entity's role (e.g. "NPC.Role.Guard", "NPC.Role.Merchant") */
    FGameplayTag RoleTag;

    /** Deterministic hash of the entity's generated name (for stable visual/dialogue lookups) */
    uint32 NameHash = 0;

    /** Visual archetype index into the faction's appearance table (drives mesh/material selection) */
    uint8 VisualArchetype = 0;

    /** Packed age bracket (3 bits) + gender (1 bit) + reserved (4 bits) */
    uint8 DemographicFlags = 0;

    /**
     * Visibility group for line-of-sight substitution.
     * Entities in different visibility groups cannot see each other (byte compare, no traces).
     * 0 = default (everyone visible). Cover/stealth systems set this to isolate the player.
     * Night + weather stack perception multipliers but don't change groups.
     */
    uint8 VisibilityGroup = 0;

    /**
     * Action category for magic/ability classification.
     * 0 = Melee, 1 = Ranged, 2 = Magic_Damage, 3 = Magic_Healing, 4 = Magic_Forbidden, 5 = Environmental
     */
    uint8 ActionCategory = 0;

    /** Convenience: is this entity a spy (undercover in enemy faction)? */
    bool IsSpy() const { return Faction.Index != TrueFaction.Index && TrueFaction.IsValid(); }
};

// ─────────────────────────────────────────────────────────────
// Schedule Fragment — Daily routine state
// Present on all tiers (~16B)
// ─────────────────────────────────────────────────────────────

/** Schedule phases for NPC daily routines */
UENUM()
enum class EMythicSchedulePhase : uint8 {
    Work,
    Rest,
    Social,
    Travel,
    Idle
};

/**
 * Tracks the entity's daily schedule state.
 * Tier 0 entities just hold the phase for visual behavior (walking, standing).
 * Tier 1+ entities route to actual cell destinations.
 */
USTRUCT()
struct MYTHIC_API FMythicScheduleFragment : public FMassFragment {
    GENERATED_BODY()

    /** Current schedule phase */
    EMythicSchedulePhase Phase = EMythicSchedulePhase::Idle;

    /** Cell coordinates for specific schedule destinations */
    FMythicCellCoord WorkCell;
    FMythicCellCoord HomeCell;
};

// ─────────────────────────────────────────────────────────────
// Significance Fragment — Tier tracking and scoring
// Present on all tiers (~12B)
// ─────────────────────────────────────────────────────────────

/**
 * Tracks the entity's significance for tier promotion/demotion.
 * Score is recalculated by the significance processor based on proximity,
 * event density, and emotional intensity.
 */
USTRUCT()
struct MYTHIC_API FMythicSignificanceFragment : public FMassFragment {
    GENERATED_BODY()

    /** Cached significance score [0.0, 1.0] */
    float Score = 0.0f;

    /** Current significance tier — determines which fragments are hydrated */
    EMythicSignificanceTier Tier = EMythicSignificanceTier::Tier0_Ambient;

    /** Count of recent causal fabric events relevant to this entity (used in score calc) */
    uint16 RelevantEventCount = 0;

    /** Dirty flag — set when the entity's context changes and score needs recalculation */
    uint8 bDirty : 1;

    FMythicSignificanceFragment() : bDirty(true) {}
};

// ─────────────────────────────────────────────────────────────
// Psychodynamic Fragment — Pressure accumulation
// Only hydrated on Tier 1+ entities (~28B)
// ─────────────────────────────────────────────────────────────

/**
 * Emotional pressure system. Each channel accumulates stress from witnessed events.
 * Pressure drives venting behavior (Fight, Flee, Report, etc.) through personality.
 * Only allocated on Tier 1+ (hydrated) entities — Tier 0 doesn't feel.
 */
USTRUCT()
struct MYTHIC_API FMythicPsychodynamicFragment : public FMassFragment {
    GENERATED_BODY()

    /** Pressure accumulated per channel. Index via EMythicPressureChannel. */
    float Pressure[PressureChannelCount] = {};

    /** World time of the last event that affected this entity's pressure */
    double LastEventTime = 0.0;

    /**
     * Entity index of the current Fight target (for mob dynamics).
     * When N entities share the same FightTargetEntity → mob bonus applied.
     * INDEX_NONE = no target.
     */
    int32 FightTargetEntity = INDEX_NONE;

    /**
     * Despair flag. Set when total accumulated unvented pressure exceeds DespairThreshold.
     * Despaired NPCs abandon desires, become vulnerable to recruitment, and contribute
     * to faction collapse spirals.
     */
    bool bDespaired = false;
};

// ─────────────────────────────────────────────────────────────
// Personality Fragment — Behavioral routing
// Only hydrated on Tier 1+ entities (~32B)
// ─────────────────────────────────────────────────────────────

/**
 * Determines how pressure routes to venting behavior.
 * Each weight is a [0.0, 1.0] value — higher means this entity is more likely
 * to use that vent channel when under pressure.
 * Personality is generated deterministically from NameHash + faction template.
 */
USTRUCT()
struct MYTHIC_API FMythicPersonalityFragment : public FMassFragment {
    GENERATED_BODY()

    /** Venting weight per channel. Index via EMythicVentChannel. Higher = more likely to use. */
    float VentWeights[static_cast<int32>(EMythicVentChannel::COUNT)] = {};
};

static constexpr int32 VentChannelCount = static_cast<int32>(EMythicVentChannel::COUNT);

// ─────────────────────────────────────────────────────────────
// Creature Fragment — Creature-specific ecology data
// Only on creature entities (~24B)
// ─────────────────────────────────────────────────────────────

/**
 * Ecology data for non-humanoid MASS entities (wolves, birds, deer, etc.).
 * Drives pack behavior, territorial defense, and herd-flee contagion.
 */
USTRUCT()
struct MYTHIC_API FMythicCreatureFragment : public FMassFragment {
    GENERATED_BODY()

    /** Species identifier (indexes into a species data table) */
    uint8 SpeciesId = 0;

    /** Pack group ID — creatures with the same PackId share pressure state */
    uint16 PackId = 0;

    /** Base aggression level [0.0, 1.0]. Amplified near den by territorial boost. */
    float BaseAggression = 0.0f;

    /** Cell coordinate of this creature's den/spawning origin */
    FMythicCellCoord DenCell;

    /** Radius (in cells) within which the creature gets territorial aggression boost */
    uint8 TerritorialRadius = 2;
};

// ─────────────────────────────────────────────────────────────
// Social Fragment — Relationship edges
// Only hydrated on Tier 1+ entities (~68B)
// ─────────────────────────────────────────────────────────────

/** Maximum social edges per entity. Keeps the fragment at a fixed, cache-friendly size. */
static constexpr int32 MaxSocialEdges = 16;

/**
 * Tracks this entity's social connections to other entities.
 * Each edge is a lightweight reference (entity handle index) + quality score.
 * Only allocated on Tier 1+ hydrated entities.
 */
USTRUCT()
struct MYTHIC_API FMythicSocialFragment : public FMassFragment {
    GENERATED_BODY()

    /** Entity handle indices of connected entities. InvalidIndex = empty slot. */
    uint16 EdgeEntityIndices[MaxSocialEdges] = {};

    /** Relationship quality per edge. Positive = friendly, negative = hostile. [-127, 127] */
    int8 EdgeQuality[MaxSocialEdges] = {};

    /** Number of active edges (first N slots are valid) */
    uint8 EdgeCount = 0;

    /**
     * Has this NPC directly met/interacted with a player?
     * Required for player faction reactions (REQ-BEH-008): NPCs don't have
     * omniscient knowledge of player reputation — recognition requires either
     * prior contact or received belief propagation about the player.
     */
    bool bHasMetPlayer = false;

    /** Per-player loyalty values (for player property guards). Max 8 players. */
    float PlayerLoyalty[8] = {};

    static constexpr uint16 InvalidEdgeIndex = 0xFFFF;

    FMythicSocialFragment() {
        FMemory::Memset(EdgeEntityIndices, 0xFF, sizeof(EdgeEntityIndices));
        FMemory::Memzero(PlayerLoyalty, sizeof(PlayerLoyalty));
    }
};
