// Mythic Living World — Cognitive Brain Types
// Core data types for the BDI (Beliefs-Desires-Intentions) cognitive architecture.
// Used by Tier 2-3 NPC actors with UMythicCognitiveBrainComponent.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassEntityHandle.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "CognitiveTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythCognition, Log, All);

// ─────────────────────────────────────────────────────────────
// Desire Types — What an NPC might want
// ─────────────────────────────────────────────────────────────

/**
 * The categories of desires a cognitive NPC can generate.
 * Each desire type has a dedicated utility scoring function in CognitiveBrainComponent.
 * Desires map to behavioral intentions via personality-weighted selection.
 */
UENUM(BlueprintType)
enum class EMythicDesireType : uint8 {
    /** Stay alive — triggered by high Threat pressure */
    Survive UMETA(DisplayName = "Survive"),

    /** Defend home/faction — triggered by faction under attack near home cell */
    Defend UMETA(DisplayName = "Defend"),

    /** Avenge a social connection — triggered by Grief→Wrath crystallization */
    Avenge UMETA(DisplayName = "Avenge"),

    /** Patrol territory — baseline for guard-role NPCs */
    Patrol UMETA(DisplayName = "Patrol"),

    /** Engage in trade — baseline for merchant-role NPCs */
    Trade UMETA(DisplayName = "Trade"),

    /** Seek social interaction — driven by isolation and personality */
    Socialize UMETA(DisplayName = "Socialize"),

    /** Join the player's party — emergent from relationship + moral alignment */
    JoinPlayer UMETA(DisplayName = "JoinPlayer"),

    /** Flee from danger — driven by high Threat + low Fight personality */
    Flee UMETA(DisplayName = "Flee"),

    /** Return home for rest — driven by schedule and exhaustion */
    Rest UMETA(DisplayName = "Rest"),

    /** Exploit a situation — driven by high Exploit personality + opportunity */
    Exploit UMETA(DisplayName = "Exploit"),

    /** Rally others — driven by leadership personality + group threat */
    Rally UMETA(DisplayName = "Rally"),

    /** Report a crime or event — driven by authority compliance personality */
    Report UMETA(DisplayName = "Report"),

    /** Follow daily schedule — default low-priority desire */
    FollowSchedule UMETA(DisplayName = "FollowSchedule"),

    COUNT UMETA(Hidden)
};

static constexpr int32 DesireTypeCount = static_cast<int32>(EMythicDesireType::COUNT);

// ─────────────────────────────────────────────────────────────
// Belief — What an NPC knows/perceives
// ─────────────────────────────────────────────────────────────

/**
 * A single belief held by a cognitive NPC.
 * Beliefs are formed by querying the Causal Fabric with personality bias.
 * They decay and can be incorrect (bias distortion).
 *
 * ~40 bytes per belief.
 */
USTRUCT()
struct FMythicBelief {
    GENERATED_BODY()

    /** Gameplay tag describing what this belief is about */
    FGameplayTag EventTag;

    /** Cell where the believed event occurred */
    FMythicCellCoord Cell;

    /** Faction primarily involved */
    FMythicFactionId InvolvedFaction;

    /** Confidence in this belief [0.0, 1.0] — decays over time, reduced per belief propagation hop */
    float Confidence = 1.0f;

    /** World time when this belief was formed */
    double FormationTime = 0.0;

    /** How many hops this belief has propagated through (0 = witnessed directly) */
    uint8 PropagationHops = 0;

    /** The event ID in the causal fabric this belief references (0 = no specific event) */
    uint32 SourceEventId = 0;
};

/** Max beliefs a cognitive NPC can hold. Oldest/weakest evicted on overflow. */
static constexpr int32 MaxBeliefsPerNPC = 16;

// ─────────────────────────────────────────────────────────────
// Desire — What an NPC wants to do
// ─────────────────────────────────────────────────────────────

/**
 * A scored desire with traceable causal origin.
 * Generated each think tick from beliefs + pressure + personality.
 * The highest-utility desire becomes the active intention.
 *
 * ~32 bytes per desire.
 */
USTRUCT()
struct FMythicDesire {
    GENERATED_BODY()

    /** Type of desire */
    EMythicDesireType Type = EMythicDesireType::FollowSchedule;

    /** Utility score [0.0, ∞) — higher = more urgent. Computed by utility functions. */
    float Utility = 0.0f;

    /** Target entity (for Avenge, Defend, Socialize, etc.) — may be invalid */
    FMassEntityHandle TargetEntity;

    /** Target cell (for Patrol, Defend, Flee — the destination) */
    FMythicCellCoord TargetCell;

    /** The belief that generated this desire (for causal tracing) */
    uint32 SourceEventId = 0;
};

// ─────────────────────────────────────────────────────────────
// Intention — Committed desire being executed
// ─────────────────────────────────────────────────────────────

/**
 * The currently committed intention. The brain holds exactly one active intention.
 * Intention hysteresis: a new desire must score significantly higher to override.
 */
USTRUCT()
struct FMythicIntention {
    GENERATED_BODY()

    /** The committed desire */
    FMythicDesire Desire;

    /** World time when this intention was committed */
    double CommitTime = 0.0;

    /** Timeout — if exceeded without completion, intention is abandoned */
    float TimeoutSeconds = 30.0f;

    /** Whether this intention has been started (communicated to behavior system) */
    bool bStarted = false;

    /** Whether this intention is valid (has been committed) */
    bool bValid = false;

    void Reset() {
        bValid = false;
        bStarted = false;
        Desire = FMythicDesire();
    }
};
