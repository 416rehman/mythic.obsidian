// Mythic Living World — Party Subsystem
// Per-player companion management with loyalty, betrayal, and belief propagation.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AI/Party/MythicPartyTypes.h"
#include "World/EnvironmentController/EnvironmentTypes.h" // EDayTime for the nightfall rest-phase handler
#include "Containers/Ticker.h" // FTSTicker::FDelegateHandle for the post-load companion-rebuild ticker (Slice-7)
#include "PartySubsystem.generated.h"

class AMythicNPCCharacter;
class UMythicCausalFabric;
class UMythicLivingWorldSubsystem;
class UMythicSocialGraph;
class UMythicLivingWorldSettings;
class UMythicCognitiveBrainComponent;
class APawn;
struct FMythicMoralAction; // full per-axis moral vector (Morality/MoralSignature.h) — passed by const-ref into loyalty eval
struct FMythicBelief;      // cognitive belief (AI/Cognition/CognitiveTypes.h) — passed by const-ref into ShouldShareBelief
class AMythicEnvironmentController; // the world clock — its DayTimeChangeDelegate drives the nightfall rest phase

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
    virtual void Initialize(FSubsystemCollectionBase &Collection) override;
    virtual void Deinitialize() override;
    virtual bool ShouldCreateSubsystem(UObject *Outer) const override;
    // Subscribe to the world clock here: the world + game instance are ready, though the controller actor may not be
    // spawned yet (handled by the register-delegate late-bind, mirroring UMythicEnvironmentHazardComponent).
    virtual void OnWorldBeginPlay(UWorld &InWorld) override;
    //~ End USubsystem Interface

    // ─── Party Management ─────────────────────────────────

    /**
     * Add a companion to the player's party.
     * @param PlayerKey     The recruiter's CANONICAL key (GetCanonicalPlayerKey). Both the party membership key AND the
     *                      follow key: the companion follows THIS player's pawn via the registry, so a co-op companion
     *                      follows whoever recruited it (not always the host), and the party survives save/load.
     * @param NPC           The NPC actor to add (must be Tier 2-3 cognitive)
     * @param SourceEntity  The MASS entity this NPC was promoted from
     * @return true if successfully added (false if party full or NPC invalid)
     */
    bool AddCompanion(const FString &PlayerKey, AMythicNPCCharacter *NPC, FMassEntityHandle SourceEntity);

    /**
     * True if ANY player's party already contains a member with this persisted NameHash. Used by AddCompanion's
     * cross-party duplicate guard so two players can't both recruit the same NPC (co-op). NameHash 0 (no captured
     * identity) never matches. Pure + static so the cross-party scan is unit-testable.
     */
    static bool AnyPartyContainsNameHash(const TMap<FString, TArray<FMythicPartyMember>> &AllParties, uint32 NameHash);

    /**
     * Migrate a pre-v4 save's int32 party key (always 0 in slice-1) to a well-formed canonical player key. Delegates to
     * AMythicPlayerState::ResolveCanonicalPlayerKey (single source), so the migrated key uses the session-fallback form.
     * Static + pure so the migration rule is unit-testable.
     */
    static FString MakeLegacyPartyKey(int32 LegacyPlayerId);

    /**
     * Remove a companion from the player's party.
     * @param PlayerId  Player controller ID
     * @param NPC       The NPC actor to remove
     * @param bVoluntary Whether this is a voluntary departure (true) or forced removal (false)
     * @return true if the companion was found and removed
     */
    bool RemoveCompanion(const FString &PlayerKey, AMythicNPCCharacter *NPC, bool bVoluntary = false);

    /**
     * Remove an NPC from WHATEVER player's party holds it (the caller doesn't know the PlayerId). Used on companion
     * DEATH — a dead companion MUST leave the party, otherwise its CompanionEntities despawn-exemption (slice 1b) keeps
     * it pinned Tier2 forever, blocking the perma-death dehydration + leaking a cognitive slot.
     * @return true if the NPC was found in any party and removed.
     */
    bool RemoveCompanionFromAnyParty(AMythicNPCCharacter *NPC, bool bVoluntary = false);

    /**
     * Get the party members for a specific player.
     * @param PlayerId  Player controller ID
     * @param OutMembers Output array of party members
     * @return Number of party members
     */
    int32 GetPartyMembers(const FString &PlayerKey, TArray<FMythicPartyMember> &OutMembers) const;

    /**
     * Get current party size for a player.
     */
    int32 GetPartySize(const FString &PlayerKey) const;

    /**
     * Check if an NPC is in any player's party.
     */
    bool IsInParty(const AMythicNPCCharacter *NPC) const;

    /**
     * Pure propagation-eligibility predicate for a belief during party gossip: a belief is shared only while it is
     * still confident enough to matter AND has not exceeded the designer's max hop distance (MaxBeliefPropagationHops).
     * Static + pure so it is unit-testable without live party actors, and the single source of truth for the gossip
     * loop's gate. PropagationHops is 0 for a directly-witnessed belief and incremented on each propagation hop.
     */
    static bool ShouldShareBelief(const FMythicBelief &Belief, int32 MaxHops);

    /**
     * Single source of truth for one belief's save/load byte layout (used by both branches of Serialize so the field
     * order can never desync between save and load). Reads/writes the original 3 fields (EventTag/Confidence/
     * FormationTime) for all versions and the full semantic state (Cell/InvolvedFaction/LastDecayTime/PropagationHops/
     * SourceEventId) at Version >= 3. Static so it is unit-testable without a live subsystem.
     */
    static void SerializeBelief(FArchive &Ar, FMythicBelief &Belief, int32 Version);

    /**
     * Single source of truth for a party's save/load KEY field (so save + load can't desync — same rationale as
     * SerializeBelief). v4+: the canonical player key (FString). v<4 (legacy): the field was an int32 PlayerId — read it
     * for byte-alignment then migrate via MakeLegacyPartyKey. Static so the round-trip is unit-testable with an FArchive.
     */
    static void SerializePartyKey(FArchive &Ar, FString &Key, int32 Version);

    /**
     * The pawn a companion of PlayerId should follow. Slice-1 single-player mapping: PlayerId 0 → the first local
     * player's pawn. The real PlayerId→pawn resolver is the MP slice (one shared source per Rule 3).
     */
    APawn *GetLeaderPawn(const FString &PlayerKey) const;

    /**
     * True if the MASS entity belongs to any player's party. Game-thread query used by the SignificanceProcessor
     * (itself game-thread) to EXEMPT companions from Tier2→Tier1 dehydration — otherwise a companion that follows the
     * player out of its significance zone is silently despawned. Mirrors the PersistentNPCRegistry::IsPermaDead gate.
     */
    bool IsCompanionEntity(FMassEntityHandle Entity) const;

    // ─── Event Handling ───────────────────────────────────

    /**
     * Notify the party system that the player performed an action.
     * All companions evaluate the action against their own faction morals.
     * @param PlayerId    Player who performed the action (0 = the single-player party key; AddCompanion(0,...) convention)
     * @param EventTag    What happened (the action tag)
     * @param MoralAction The FULL per-axis moral vector of the action ([-1,1] per axis). Taking the whole vector (not a
     *                    single axis) feeds companions the SAME data the world-action pipeline records — e.g. a kill
     *                    carries Violence AND Mercy — so loyalty severity is judged on the complete action, not a slice.
     */
    void OnPlayerAction(const FString &PlayerKey, const FGameplayTag &EventTag, const FMythicMoralAction &MoralAction);

    /**
     * Enter rest phase for a player's party. Triggers belief propagation.
     * @param PlayerId Player entering rest
     */
    void EnterRestPhase(const FString &PlayerKey);

    /**
     * Exit rest phase.
     */
    void ExitRestPhase(const FString &PlayerKey);

    // ─── Serialization ───────────────────────────────────

    /** Serialize party state for save/load. Saves companion loyalty, pressure, and party composition. */
    virtual void Serialize(FArchive &Ar) override;

private:
    // ─── Nightfall rest-phase wiring ──────────────────────
    // The world clock (AMythicEnvironmentController::DayTimeChangeDelegate) is the one real rest trigger that exists:
    // at nightfall every active party rests (loyalty recovery + betrayal-pressure decay + belief propagation), at dawn
    // it wakes. Bound server-side only (the delegate fires on server + clients; party state is authoritative).
    UFUNCTION()
    void OnEnvironmentControllerRegistered(AMythicEnvironmentController *Controller);
    UFUNCTION()
    void OnDaytimeChanged(EDayTime PrevDayTime, EDayTime NewDayTime);
    void BindEnvironmentController(AMythicEnvironmentController *Controller);
    TWeakObjectPtr<AMythicEnvironmentController> BoundEnvController;

    // ─── Internal Operations ──────────────────────────────

    /**
     * Propagate beliefs between party members during rest.
     * Each member shares their top beliefs with others, with confidence reduction per hop.
     */
    void PropagateBeliefs(const FString &PlayerKey);

    /**
     * Evaluate a companion's loyalty response to a player action's full moral vector (judged against the companion's
     * own faction ideology).
     * @return Loyalty delta (positive = pleased, negative = disturbed)
     */
    float EvaluateLoyaltyImpact(const FMythicPartyMember &Member, const FMythicMoralAction &MoralAction) const;

    /**
     * Check if a companion should leave or betray based on current state.
     */
    void CheckCompanionThresholds(const FString &PlayerKey, int32 MemberIndex);

    /**
     * Handle companion departure (voluntary or forced by loyalty threshold).
     */
    void HandleCompanionDeparture(const FString &PlayerKey, int32 MemberIndex);

    /**
     * Handle companion betrayal (pressure exceeded betrayal threshold).
     */
    void HandleCompanionBetrayal(const FString &PlayerKey, int32 MemberIndex);

    /**
     * The SINGLE removal sink for a party slot: drops the despawn-exemption signal + stops the companion's follow timer
     * BEFORE the RemoveAtSwap, so no exit path (RemoveCompanion / departure / betrayal / later death) can bypass either
     * cleanup. Captures the member by const-ref before the swap.
     */
    void RemoveMemberAt(const FString &PlayerKey, TArray<FMythicPartyMember> &Party, int32 Index);

    // SERVER → owning client: float a "<Name> has left your party" (grey) or "<Name> turns on you!" (red) callout over
    // the departing companion. Single source for the departure + betrayal loss feedback. Call BEFORE RemoveMemberAt
    // (while Member is still valid).
    void NotifyCompanionLost(const FString &PlayerKey, const FMythicPartyMember &Member, bool bBetrayed);

    // ─── Slice-7: post-load companion entity re-creation + rebind ───
    // The Mass world is transient on load (the EntityManager is never serialized) + the spawner only mints NEW
    // NameHashes (R18-M7), so a saved companion entity is never auto-recreated. These re-create each persisted
    // companion entity (caller-supplied NameHash), force-promote it to Tier2, request embodiment, and rebind the slot
    // once the actor exists. Game-thread only, idempotent. Driven by a self-rescheduling FTSTicker that survives the
    // two-tick ActorSpawnProcessor handoff.

    /** Armed at the end of a load-Serialize. Idempotent. Starts the rebuild ticker if any member needs rebuilding. */
    void RebindCompanionsAfterLoad();

    /** Self-rescheduling ticker body: creates entities on the first fire, then polls FindEmbodiedActor for the handoff,
     *  rebinds once embodied, stops when all members are Bound or a max-attempts timeout is hit. Returns true=keep. */
    bool TickCompanionRebuild(float DeltaTime);

    /** Re-create one persisted companion entity (Identity/Schedule/Significance + Tier2 hydrate + spawn-request tag).
     *  IsPermaDead-guarded. Returns the new entity handle (invalid on perma-dead / failure). The caller registers it in
     *  CompanionEntities SYNCHRONOUSLY (the despawn/demotion exemption is keyed on the handle). */
    FMassEntityHandle CreateLoadedCompanionEntity(const FMythicPartyMember &Member);

    /** Bind a loaded party slot to its freshly re-embodied actor + re-arm its follow timer, preserving the already-
     *  loaded loyalty/pressure/beliefs. */
    void RebindLoadedCompanion(const FString &PlayerKey, FMythicPartyMember &Member, AMythicNPCCharacter *Actor);

    /** Handle for the self-rescheduling rebuild ticker (invalid when not running). */
    FTSTicker::FDelegateHandle CompanionRebuildTickHandle;

    /** Tick count for the rebuild timeout guard (reset each RebindCompanionsAfterLoad). */
    int32 CompanionRebuildAttempts = 0;

    // ─── State ────────────────────────────────────────────

    /** Per-player party: canonical player key (GetCanonicalPlayerKey) → array of party members. Keyed by the persistent
     *  cross-session key (not the session int32 PlayerId) so co-op parties stay distinct + survive save/load. */
    TMap<FString, TArray<FMythicPartyMember>> PlayerParties;

    /** Flat set of all companion MASS entities (across all parties) for O(1) IsCompanionEntity — the despawn-exemption
     *  signal the SignificanceProcessor reads. Maintained in AddCompanion / RemoveCompanion alongside PlayerParties. */
    TSet<FMassEntityHandle> CompanionEntities;

    /** Max party size (configurable) */
    int32 MaxPartySize = 4;

    /** Loyalty below which companion voluntarily leaves */
    float LoyaltyDepartureThreshold = 0.15f;

    /** Betrayal pressure above which companion acts against the player */
    float BetrayalThreshold = 5.0f;

    /** A single player action whose |loyalty impact| on a STAYING companion meets/exceeds this makes that companion
     *  remark on it (moral commentary). Just above the betrayal-pressure trigger (0.1) so only notably-charged acts —
     *  heroic or villainous — draw a comment, not routine ones. */
    float CompanionCommentaryLoyaltyDelta = 0.15f;

    /** Confidence reduction per belief propagation hop */
    float BeliefPropagationDecay = 0.3f;

    /** Max hops a belief travels through party gossip before it stops spreading (designer cap; cached from
     *  LivingWorldSettings::MaxBeliefPropagationHops). Enforced via ShouldShareBelief so a rumor cannot reach the
     *  entire connected party graph — confidence-decay alone would otherwise let it spread ~5-6 hops. */
    int32 MaxBeliefPropagationHops = 3;

    // ─── Cached References ────────────────────────────────

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    // Owning subsystem. Game-thread world-event writes MUST route through its thread-safe SubmitWorldEvent queue — the
    // CausalFabric is a lock-free SINGLE-writer drained by the sim thread, so a direct game-thread AppendEvent races it.
    UPROPERTY()
    TObjectPtr<UMythicLivingWorldSubsystem> LivingWorld;

    UMythicSocialGraph *SocialGraph = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;
};
