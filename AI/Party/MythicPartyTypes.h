// Mythic Living World — Party Types
// Data types for the party/companion system.

#pragma once

#include "CoreMinimal.h"
#include "MassEntityHandle.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicPartyTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythParty, Log, All);

class AMythicNPCCharacter;

// ─────────────────────────────────────────────────────────────
// Party Member State
// ─────────────────────────────────────────────────────────────

/**
 * State of a single companion in a player's party.
 * Tracks the NPC actor, loyalty dynamics, and shared beliefs.
 */
USTRUCT()
struct FMythicPartyMember {
    GENERATED_BODY()

    /** The NPC actor in the world (Tier 3 cognitive NPC) */
    TWeakObjectPtr<AMythicNPCCharacter> NPCActor;

    /** The MASS entity handle this NPC was promoted from (for save/load identity) */
    FMassEntityHandle SourceEntity;

    /** Faction this NPC originally belonged to */
    FMythicFactionId OriginalFaction;

    /**
     * Loyalty to the player [0.0, 1.0].
     * Increases from shared positive events, decreases from moral violations.
     * Below LoyaltyDepartureThreshold → companion leaves.
     */
    float LoyaltyScore = 0.5f;

    /**
     * Accumulated betrayal pressure [0.0, ∞).
     * Increases when player forces actions above the NPC's Condemn threshold.
     * Above BetrayalThreshold → companion acts against the player.
     */
    float BetrayalPressure = 0.0f;

    /** Beliefs shared during rest phase (propagated between party members) */
    TArray<FMythicBelief> SharedBeliefs;

    /** World time when this companion joined the party */
    double JoinTime = 0.0;

    /** Whether this companion is currently in rest phase */
    bool bInRestPhase = false;

    /** Display name cached from the NPC's identity for quick access */
    FText CachedDisplayName;
};

// ─────────────────────────────────────────────────────────────
// Party Event Types — For tracking shared experiences
// ─────────────────────────────────────────────────────────────

UENUM(BlueprintType)
enum class EMythicPartyEventType : uint8 {
    /** Player performed an action the companion witnessed */
    PlayerActionWitnessed UMETA(DisplayName = "PlayerActionWitnessed"),

    /** Party entered rest phase — triggers belief propagation */
    RestPhaseStarted UMETA(DisplayName = "RestPhaseStarted"),

    /** A shared combat event (both player and companion involved) */
    SharedCombat UMETA(DisplayName = "SharedCombat"),

    /** Player gave a direct order that conflicts with companion morals */
    ForcedCompliance UMETA(DisplayName = "ForcedCompliance"),

    /** Companion's original faction suffered a significant event */
    FactionEventWitnessed UMETA(DisplayName = "FactionEventWitnessed")
};
