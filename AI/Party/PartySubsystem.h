// Mythic Living World — Party Subsystem
// Per-player companion management with loyalty, betrayal, and belief propagation.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/Party/MythicPartyTypes.h"
#include "PartySubsystem.generated.h"

class AMythicNPCCharacter;
class UMythicCausalFabric;
class UMythicSocialGraph;
class UMythicLivingWorldSettings;
class UMythicCognitiveBrainComponent;

/**
 * Party Subsystem — manages player companions (Tier 3 cognitive NPCs).
 *
 * Architecture:
 * - Per-player party slots (max 4, configurable)
 * - Loyalty tracking from shared event moral evaluation
 * - Betrayal pressure from forced compliance (order NPC to violate their morals)
 * - Rest-phase belief propagation between companions
 * - Companions depart or betray when thresholds are exceeded
 *
 * Performance:
 * - No per-frame tick — event-driven + rest-phase timer
 * - Max 4 companions × 16 beliefs each = 64 beliefs per party
 * - Loyalty/betrayal updates only on witnessed events (~O(1) per event)
 *
 * Party system is a WorldSubsystem (not per-player) because it manages all
 * player parties and needs world-context access (fabric, social graph).
 */
UCLASS()
class MYTHIC_API UMythicPartySubsystem : public UWorldSubsystem {
    GENERATED_BODY()

public:
    //~ Begin USubsystem Interface
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
    //~ End USubsystem Interface

    // ─── Party Management ─────────────────────────────────

    /**
     * Add a companion to the player's party.
     * @param PlayerId      Player controller ID
     * @param NPC           The NPC actor to add (must be Tier 2-3 cognitive)
     * @param SourceEntity  The MASS entity this NPC was promoted from
     * @return true if successfully added (false if party full or NPC invalid)
     */
    bool AddCompanion(int32 PlayerId, AMythicNPCCharacter* NPC, FMassEntityHandle SourceEntity);

    /**
     * Remove a companion from the player's party.
     * @param PlayerId  Player controller ID
     * @param NPC       The NPC actor to remove
     * @param bVoluntary Whether this is a voluntary departure (true) or forced removal (false)
     * @return true if the companion was found and removed
     */
    bool RemoveCompanion(int32 PlayerId, AMythicNPCCharacter* NPC, bool bVoluntary = false);

    /**
     * Get the party members for a specific player.
     * @param PlayerId  Player controller ID
     * @param OutMembers Output array of party members
     * @return Number of party members
     */
    int32 GetPartyMembers(int32 PlayerId, TArray<FMythicPartyMember>& OutMembers) const;

    /**
     * Get current party size for a player.
     */
    int32 GetPartySize(int32 PlayerId) const;

    /**
     * Check if an NPC is in any player's party.
     */
    bool IsInParty(const AMythicNPCCharacter* NPC) const;

    // ─── Event Handling ───────────────────────────────────

    /**
     * Notify the party system that the player performed an action.
     * All companions evaluate the action against their morals.
     * @param PlayerId  Player who performed the action
     * @param EventTag  What happened
     * @param MoralAxis Which moral dimension is affected
     * @param MoralValue Magnitude and direction [-1.0, 1.0]
     */
    void OnPlayerAction(int32 PlayerId, const FGameplayTag& EventTag, EMythicMoralAxis MoralAxis, float MoralValue);

    /**
     * Enter rest phase for a player's party. Triggers belief propagation.
     * @param PlayerId Player entering rest
     */
    void EnterRestPhase(int32 PlayerId);

    /**
     * Exit rest phase.
     */
    void ExitRestPhase(int32 PlayerId);

    // ─── Serialization ───────────────────────────────────

    /** Serialize party state for save/load. Saves companion loyalty, pressure, and party composition. */
    void Serialize(FArchive& Ar);

private:
    // ─── Internal Operations ──────────────────────────────

    /**
     * Propagate beliefs between party members during rest.
     * Each member shares their top beliefs with others, with confidence reduction per hop.
     */
    void PropagateBeliefs(int32 PlayerId);

    /**
     * Evaluate a companion's loyalty response to a player action.
     * @return Loyalty delta (positive = pleased, negative = disturbed)
     */
    float EvaluateLoyaltyImpact(const FMythicPartyMember& Member, EMythicMoralAxis MoralAxis, float MoralValue) const;

    /**
     * Check if a companion should leave or betray based on current state.
     */
    void CheckCompanionThresholds(int32 PlayerId, int32 MemberIndex);

    /**
     * Handle companion departure (voluntary or forced by loyalty threshold).
     */
    void HandleCompanionDeparture(int32 PlayerId, int32 MemberIndex);

    /**
     * Handle companion betrayal (pressure exceeded betrayal threshold).
     */
    void HandleCompanionBetrayal(int32 PlayerId, int32 MemberIndex);

    // ─── State ────────────────────────────────────────────

    /** Per-player party: PlayerId → array of party members */
    TMap<int32, TArray<FMythicPartyMember>> PlayerParties;

    /** Max party size (configurable) */
    int32 MaxPartySize = 4;

    /** Loyalty below which companion voluntarily leaves */
    float LoyaltyDepartureThreshold = 0.15f;

    /** Betrayal pressure above which companion acts against the player */
    float BetrayalThreshold = 5.0f;

    /** Confidence reduction per belief propagation hop */
    float BeliefPropagationDecay = 0.3f;

    // ─── Cached References ────────────────────────────────

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UMythicSocialGraph* SocialGraph = nullptr;
    const UMythicLivingWorldSettings* Settings = nullptr;
};
