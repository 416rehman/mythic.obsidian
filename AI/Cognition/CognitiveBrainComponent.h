// Mythic Living World — Cognitive Brain Component
// BDI (Beliefs-Desires-Intentions) brain for Tier 2-3 cognitive NPC actors.
// Attaches to AMythicNPCCharacter. Think tick is staggered across actors.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Mass/EntityHandle.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "AI/Cognition/CognitiveTypes.h"
#include "Tasks/Task.h"
#include "CognitiveBrainComponent.generated.h"

class UMythicCausalFabric;
class UMythicFactionDatabase;
class UMythicSocialGraph;
class UMythicLivingWorldSettings;
enum class EMythicSchedulePhase : uint8;

/**
 * BDI cognitive brain attached to Tier 2-3 NPC actors.
 *
 * Architecture:
 * - Think() runs on a staggered timer (0.5-2.0s, configurable)
 * - Belief layer queries the Causal Fabric with personality-weighted filtering
 * - Desire layer scores each desire type via utility functions
 * - Intention layer commits the highest-utility desire with hysteresis
 * - Execution communicates to the behavior system via gameplay tags
 *
 * Performance:
 * - NOT a tick component — uses FTimerHandle for staggered think
 * - Max 30 cognitive actors at once (budgeted by LivingWorldSettings::MaxCognitiveActors)
 * - Each Think() is O(beliefs × desires) = O(16 × 13) = ~200 operations
 * - Total per-frame cost distributed across ~15 actors per second at 0.5s interval
 */
UCLASS(ClassGroup=(LivingWorld), meta=(BlueprintSpawnableComponent))
class MYTHIC_API UMythicCognitiveBrainComponent : public UActorComponent {
    GENERATED_BODY()

public:
    UMythicCognitiveBrainComponent();

    /**
     * Resolves the best dialogue template for the NPC's current state.
     * Integrates faction, role, active intention, and emotional pressure.
     * bCompanionCommentary=true selects the COMMENTARY template family (a companion remarking on the player's moral
     * action), gated by PlayerActionMoralScore vs each template's CommentaryMoralThreshold; in that mode a no-match
     * returns EMPTY (caller skips the bark). Defaults reproduce the normal player-initiated bark exactly.
     */
    UFUNCTION(BlueprintCallable, Category = "Living World|Dialogue")
    FText SelectDialogue(AActor *InteractingPlayer = nullptr, bool bCompanionCommentary = false, float PlayerActionMoralScore = 0.0f) const;

    //~ Begin UActorComponent Interface
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    //~ End UActorComponent Interface

    // ─── Desire arbitration tuning ────────────────────────

    /** Ceiling for ROUTINE (non-acute) desire utilities. Routine desires (Patrol/Trade/Socialize/Exploit/Rally/Report)
     *  must NOT outscore the daily-schedule anchors (FollowSchedule=0.7, Rest=0.8) — only ACUTE threat desires
     *  (Survive/Flee/Defend/Avenge) are intentionally unbounded. Without this cap an unbounded routine scorer (a
     *  high-Injustice NPC's Rally/Report) wins arbitration and steers the NPC off its routine into a desire the
     *  controller has no movement for, so it idles in place. */
    static constexpr float RoutineDesireCeiling = 0.65f;

    /** Pure utility for the simple "Weight × Pressure × Multiplier, capped at RoutineDesireCeiling, floored at 0"
     *  routine scorers (Rally, Report). Static + pure so the cap is unit-testable without a live brain. */
    static float ScoreRoutineDesire(float Weight, float Pressure, float Multiplier);

    /** Pure per-step belief-confidence decay: Confidence × exp(-DecayRate × max(0, DeltaSeconds)). Applied over the
     *  delta since a belief's last decay so the product telescopes to exp(-rate × totalAge) (decaying in N steps equals
     *  one step over the summed time) — that property is the bug fix this guards. Static + pure so it is unit-testable. */
    static float DecayBeliefConfidence(float Confidence, float DecayRate, double DeltaSeconds);

    /** Intention-commit hysteresis: a new (highest-utility) desire replaces the current intention only if it beats it
     *  by at least Hysteresis — without this margin an NPC flickers between two near-tied desires every think. Static +
     *  pure so the threshold logic is unit-testable; the margin is data-driven (LivingWorldSettings::DesireHysteresis). */
    static bool ShouldOverrideIntention(float BestUtility, float CurrentUtility, float Hysteresis);

    // ─── Configuration ────────────────────────────────────

    /**
     * Initialize with the NPC's identity data (faction, personality, role).
     * Must be called after construction and before the first think tick.
     */
    void InitializeBrain(
        FMythicFactionId Faction,
        FMythicCellCoord HomeCell,
        const FMythicPersonalityFragment &Personality,
        FMassEntityHandle SourceEntity,
        FMythicFactionId TrueFaction = FMythicFactionId(),
        FGameplayTag Role = FGameplayTag());

    // ─── State Access ─────────────────────────────────────

    /** Get the current intention (what the NPC is trying to do) */
    const FMythicIntention &GetCurrentIntention() const { return CurrentIntention; }

    /** Get the NPC's current beliefs. ONLY safe on the think worker itself — from any OTHER thread (e.g. party belief
     *  propagation reading a foreign brain while it may be mid-think) use GetBeliefsCopy() instead. */
    const TArray<FMythicBelief> &GetBeliefs() const { return Beliefs; }

    /** Thread-safe snapshot of the NPC's beliefs (taken under BeliefsLock). Use from other threads. */
    TArray<FMythicBelief> GetBeliefsCopy() const;

    /** Get the NPC's most recent desire scores (from last think tick). RACY from any thread other than the think
     *  worker — LastDesires is written on the async think worker under BeliefsLock. From the game thread (e.g. the
     *  gameplay debugger) use GetLastDesiresCopy() instead. */
    const TArray<FMythicDesire> &GetLastDesires() const { return LastDesires; }

    /** Thread-safe snapshot of the NPC's last-think desire scores (taken under BeliefsLock — the same lock the think
     *  worker holds around ScoreDesires when it writes LastDesires). Use from other threads (the debugger). */
    TArray<FMythicDesire> GetLastDesiresCopy() const;

    // ─── Debug context (game-thread cached identity/schedule) ─────────────────
    // These expose write-once / game-thread-cached members for the gameplay debugger's Cognition pane. No lock: Role,
    // Faction, TrueFaction, HomeCell are write-once at InitializeBrain; CachedSchedulePhase/CachedWorkCell are written
    // on the GAME thread in Think() (same thread the debugger's CollectData runs on). Race-free reads.

    /** True if this NPC is a spy (its public Faction differs from its TrueFaction). */
    bool IsSpyBrain() const { return TrueFaction.IsValid() && TrueFaction.Index != Faction.Index; }

    /** The NPC's covert true faction (for spies). Invalid for non-spies. */
    FMythicFactionId GetTrueFaction() const { return TrueFaction; }

    /** The NPC's role tag (e.g. TAG_LivingWorld_Role_Spy). */
    FGameplayTag GetRole() const { return Role; }

    /** The brain's home cell (Rest anchor). */
    FMythicCellCoord GetHomeCell() const { return HomeCell; }

    /** Game-thread-cached schedule phase copied from the entity's FMythicScheduleFragment in Think(). */
    EMythicSchedulePhase GetCachedSchedulePhase() const { return CachedSchedulePhase; }

    /** Game-thread-cached work destination cell copied from the schedule fragment in Think(). */
    FMythicCellCoord GetCachedWorkCell() const { return CachedWorkCell; }

    /** Get the MASS entity this NPC was promoted from */
    FMassEntityHandle GetSourceEntity() const { return SourceEntity; }

    /** Get the NPC's faction */
    FMythicFactionId GetFaction() const { return Faction; }

    /** The NPC's deterministic generated display name (reconstructed from its identity-fragment NameHash + faction —
     *  the SAME source SelectDialogue's {npc_name} uses). Empty if the source entity/identity is unavailable. */
    FText GetDisplayName() const;

    /** Get the NPC's personality (VentWeights, etc.) */
    const FMythicPersonalityFragment &GetPersonality() const { return Personality; }

    // ─── External Events ──────────────────────────────────

    /**
     * Inject a belief from an external source (e.g., belief propagation from companion). THREAD-SAFE: takes BeliefsLock
     * (game-thread callers race the async think worker). Confidence is reduced per hop. Duplicates merge (max confidence).
     */
    void InjectBelief(const FMythicBelief &Belief);

    /**
     * Notify the brain that a significant event occurred nearby.
     * This forces an immediate re-think (bypasses the timer).
     */
    void OnSignificantEvent(const FGameplayTag &EventTag, FMythicCellCoord EventCell);

    /**
     * SERVER: halt the think loop immediately (used when the embodied NPC dies, before its corpse is destroyed).
     * Clears the think timer so no new ticks fire and flips bInitialized=false so any already-queued tick
     * early-returns, then JOINS the in-flight async think task (AsyncThinkTask.Wait()) so the background worker is
     * guaranteed to have finished dereferencing `this` before destruction (EndPlay does the same).
     */
    void StopThinking();

    /**
     * SERVER: (re-)arm the staggered think timer. The SINGLE timer-arm site — BeginPlay calls this on first life and
     * the actor pool's WakeFromPool calls it again on reuse (BeginPlay does NOT re-run for a recycled actor). Picks a
     * fresh ThinkInterval in [CognitiveThinkIntervalMin, Max] with a random initial delay so reused actors re-stagger
     * instead of all thinking on the same frame. No-op (with a warning at the BeginPlay call) if the living-world
     * references were never cached (CausalFabric/FactionDB/Settings null) — preserves BeginPlay's early-out.
     */
    void StartThinking();

    /**
     * SERVER: wipe all per-life BDI state so a pooled actor's brain starts clean on its next embodiment. Clears
     * Beliefs / LastDesires / CurrentIntention / SourceEntity, flips bInitialized=false, and resets CachedSchedulePhase
     * to Idle. MUST be called AFTER StopThinking() has joined the async think worker (the pool's SleepToPool ordering
     * guarantees this) — Beliefs/LastDesires are otherwise touched by that worker. Beliefs/LastDesires are cleared
     * under BeliefsLock (the same lock the worker holds), the rest are game-thread-only members. InitializeBrain does
     * NOT clear these, so without this a reused brain would inherit the previous occupant's beliefs/intention.
     */
    void ResetForReuse();

private:
    // ─── Core BDI Loop ────────────────────────────────────

    /** Main think function. Called on staggered timer. */
    void Think();

    /** Update beliefs by querying the causal fabric with personality bias */
    void UpdateBeliefs(double WorldTime);

    /** Lock-free belief insertion (dedup-merge / evict-weakest / add). The CALLER must hold BeliefsLock — the public
     *  InjectBelief() wraps this with the lock for game-thread callers; the think worker calls it under the coarse
     *  BeliefsLock it already holds around UpdateBeliefs/ScoreDesires. */
    void InjectBeliefInternal(const FMythicBelief &Belief);

    /** Score all desire types and populate the desires array */
    void ScoreDesires(double WorldTime);

    /** Commit the best desire as the active intention (with hysteresis) */
    void CommitIntention(double WorldTime);

    /** Check if the current intention should be abandoned (timeout, invalid target) */
    void ValidateIntention(double WorldTime);

    /** Called on the game thread when the async Think task completes. Runs Validate+Commit (which write
     *  CurrentIntention) here so that struct is only ever touched on the game thread. WorldTime is the Think-time
     *  stamp, threaded through so commit hysteresis/timeout match the scoring pass. */
    void OnAsyncThinkCompleted(double WorldTime);

    // ─── Utility Scoring Functions ────────────────────────

    float ScoreSurvive(double WorldTime) const;
    float ScoreDefend(double WorldTime) const;
    float ScoreAvenge(double WorldTime) const;
    float ScorePatrol(double WorldTime) const;
    float ScoreTrade(double WorldTime) const;
    float ScoreSocialize(double WorldTime) const;
    float ScoreJoinPlayer(double WorldTime) const;
    float ScoreFlee(double WorldTime) const;
    float ScoreRest(double WorldTime) const;
    float ScoreExploit(double WorldTime) const;
    float ScoreRally(double WorldTime) const;
    float ScoreReport(double WorldTime) const;
    float ScoreFollowSchedule(double WorldTime) const;

    // ─── Cached References ────────────────────────────────

    UPROPERTY()
    TObjectPtr<UMythicCausalFabric> CausalFabric;

    UPROPERTY()
    TObjectPtr<UMythicFactionDatabase> FactionDB;

    UMythicSocialGraph *SocialGraph = nullptr;
    const UMythicLivingWorldSettings *Settings = nullptr;

    // ─── NPC Identity ─────────────────────────────────────

    FMythicFactionId Faction;
    FMythicFactionId TrueFaction; // For spies
    FGameplayTag Role; // e.g. TAG_LivingWorld_Role_Spy
    FMythicCellCoord HomeCell;
    FMythicPersonalityFragment Personality;
    FMassEntityHandle SourceEntity;
    float PressureChannels[PressureChannelCount] = {};

    // Cached schedule phase, copied from the entity's FMythicScheduleFragment on the GAME thread in Think() (same
    // cross-thread-safe pattern as PressureChannels above) so the async scorers (ScoreRest/ScoreFollowSchedule) can
    // read it without racing the MASS ScheduleTransitionProcessor that writes it every sim tick. Defaults to the
    // first enumerator for non-embodied NPCs (no schedule); Think() overwrites it for embodied ones.
    EMythicSchedulePhase CachedSchedulePhase{};

    // Cached work destination, copied from the entity's FMythicScheduleFragment alongside the phase in Think(). The
    // FollowSchedule desire targets this during the Work phase (HomeCell — the brain's own member — covers Rest).
    FMythicCellCoord CachedWorkCell;

    // ─── BDI State ────────────────────────────────────────

    TArray<FMythicBelief> Beliefs;
    // Guards ALL Beliefs access. The async think worker decays/adds + iterates Beliefs (UpdateBeliefs/ScoreDesires),
    // while the GAME thread mutates it via InjectBelief (party belief propagation + betrayal) and reads it via
    // GetBeliefsCopy. Without this, a TArray realloc on one thread collides with an iterator on the other (crash).
    mutable FCriticalSection BeliefsLock;
    TArray<FMythicDesire> LastDesires;
    FMythicIntention CurrentIntention;

    /** Handle to the currently running async think task */
    UE::Tasks::FTask AsyncThinkTask;

    /** If true, an async think task is currently running */
    std::atomic<bool> bIsThinkingAsync{false};

    // ─── Timer ────────────────────────────────────────────

    FTimerHandle ThinkTimerHandle;
    float ThinkInterval = 1.0f;
    bool bInitialized = false;
};
