// Mythic Living World — Party Types
// Data types for the party/companion system.

#pragma once

#include "CoreMinimal.h"
#include "Mass/EntityHandle.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h"
#include "MythicPartyTypes.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogMythParty, Log, All);

class AMythicNPCCharacter;

// Tri-state guard for the post-load companion rebuild (Slice-7). A loaded member walks NotCreated -> EntityCreated
// (entity re-minted + Tier2 + spawn-request tag set, actor not yet embodied) -> Bound (actor embodied + slot rebound).
// Live (recruited-this-session) members are Bound from the start.
UENUM()
enum class EMythicCompanionRebuildState : uint8 {
    Bound UMETA(DisplayName = "Bound"), // actor live + slot bound (default for live recruits)
    NotCreated UMETA(DisplayName = "NotCreated"), // loaded, entity not yet re-minted
    EntityCreated UMETA(DisplayName = "EntityCreated") // entity re-minted + spawn-request tagged, awaiting embodiment
};

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

    // ─── Slice-7: persisted MASS identity for entity re-creation on load ───
    // The Mass world is transient (the EntityManager is never serialized) and the spawner only mints NEW monotonic
    // NameHashes (R18-M7), so SourceEntity is stale after load and re-resolve-by-scan finds nothing. These fields let
    // us explicitly RE-CREATE the companion entity with a caller-supplied NameHash on load. Captured in AddCompanion
    // from the live source entity's FMythicIdentityFragment (canonical — Rule 3). OriginalFaction holds the cover faction.
    uint32 PersistedNameHash = 0;
    FMythicFactionId PersistedTrueFaction;
    FMythicCellCoord PersistedSpawnCell;
    FGameplayTag PersistedRoleTag;

    // Post-load rebuild bookkeeping (NOT serialized — defaults to Bound for live recruits; set to NotCreated on load
    // for members carrying a persisted identity).
    EMythicCompanionRebuildState RebuildState = EMythicCompanionRebuildState::Bound;
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
