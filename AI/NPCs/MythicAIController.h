// 

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "DetourCrowdAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "World/LivingWorld/LivingWorldTypes.h" // FMythicCellCoord (patrol ring helper)
#include "MythicAIController.generated.h"

class UMythicAttributeSet_NPCCombat;
class UMythicAbilitySystemComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;
class UMythicActivityCatalog;
struct FMythicActivityDef;

/**
 * Read-only snapshot of an AI controller's non-combat runtime state, for the Living World gameplay debugger
 * (Companions/Players pane). AI controllers are server-only, single-threaded actors, so a plain by-value copy is
 * race-free. Carries the leash anchor + idle/patrol/flee/companion-follow state that is otherwise protected.
 */
struct FMythicAIDebugState {
    FVector EngageAnchorLocation = FVector::ZeroVector;
    float LeashRange = 0.0f;
    int32 PatrolLegIndex = 0;
    bool bFleeingMove = false;
    bool bCompanionFollowActive = false;
    FString CompanionLeaderKey;
    bool bHasHostileTarget = false;
};

/// Every NPC will have a name, and an Ability System Component with LifeAttributeSet
/// AIController's are not replicated, so values needed on the clients should go in the NPCCharacter
UCLASS(Blueprintable, BlueprintType, Abstract)
class MYTHIC_API AMythicAIController : public ADetourCrowdAIController, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    AMythicAIController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
    // SERVER: apply the possessed NPC's per-type sight perception (from its NPCData) over the constructor defaults;
    // also (re)subscribe the threat accrual to the possessed pawn's received-hit event.
    virtual void OnPossess(APawn *InPawn) override;
    virtual void OnUnPossess() override;

    // Pure sanitiser for a sight config: clamps sight/lose-sight to >= 0, forces LoseSightRadius >= SightRadius (a
    // smaller lose-radius makes a target sensed-then-instantly-lost — a real authoring footgun), and clamps the
    // peripheral angle to [0,180]. Static + no engine state so the invariant is unit-testable.
    static void SanitizePerception(float &SightRadius, float &LoseSightRadius, float &PeripheralAngleDegrees);

    // Pure: index of the nearest hostile by squared distance (the default acquisition policy — engage the CLOSEST
    // perceived threat, not whichever was sensed first). INDEX_NONE for an empty set; ties resolve to the first.
    // Static so the selection is unit-testable; a threat-weighted policy remains the logged aggro-model design call.
    static int32 SelectClosestHostileIndex(TConstArrayView<float> DistancesSq);

    // Pure: index of the HIGHEST-threat candidate (parallel to a candidates array), for the aggro/threat-table
    // targeting policy (a tank holding aggro off the squishy players). INDEX_NONE if empty OR no candidate has
    // positive threat — so the caller falls back to SelectClosestHostileIndex when no threat has been accrued. Ties
    // resolve to the first (lowest-index) max, matching the closest-policy. Static so the selection is unit-testable.
    static int32 SelectHighestThreatIndex(TConstArrayView<float> Threats);

    // Pure: the threat a single combat action adds to this NPC's threat table = Damage × ThreatPerDamage + a flat
    // BonusThreat (e.g. a taunt forcing aggro). All inputs clamped non-negative so an action only ever RAISES threat.
    static float ComputeThreatDelta(float Damage, float ThreatPerDamage, float BonusThreat);

    // Pure: should the NPC drop its target because it was pulled too far from its engage anchor? LeashRangeSq <= 0
    // disables the leash (infinite pursuit — the default). Static so the leash rule is unit-testable.
    static bool ShouldReleaseLeash(float DistSqFromAnchor, float LeashRangeSq);

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;

    //~ Begin IGenericTeamAgentInterface Interface
    virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor &Other) const override;
    // Returns a constant valid team id so the engine's perception affiliation filter actually routes through
    // GetTeamAttitudeTowards (which is faction + player-reputation aware). Without this, that logic is never
    // invoked - per-actor hostility is decided entirely by GetTeamAttitudeTowards, not by this id.
    virtual FGenericTeamId GetGenericTeamId() const override;
    //~ End IGenericTeamAgentInterface Interface

    // The actor this NPC currently considers a hostile target (server-only). Null when none perceived.
    UFUNCTION(BlueprintCallable, Category = "Mythic AI|Combat")
    AActor *GetCurrentHostileTarget() const { return CurrentHostileTarget; }

    // Copy-out of the per-attacker aggro table (read-only debug accessor). AI controllers are server-only,
    // single-threaded actors, so a plain by-value copy is race-free. Returns the number of entries copied.
    // Added for the Living World gameplay debugger (Party/Companions pane) so it can surface real threat numbers.
    int32 CopyThreatTable(TArray<TPair<TWeakObjectPtr<AActor>, float>> &OutThreats) const;

    // Copy-out of the non-combat runtime state (leash anchor, idle/patrol/flee/companion-follow) for the gameplay
    // debugger. Server-only single-threaded actor — a plain by-value copy is race-free. Mirrors CopyThreatTable.
    void CopyAIDebugState(FMythicAIDebugState &Out) const;

    // SERVER: commit this NPC to engaging Target as a hostile (anchor the leash, pursue + face, start the attack
    // loop, fire OnEngageHostileTarget). Refactored out of OnTargetPerceptionUpdated so non-perception callers — a
    // social verb that angers the NPC (AMythicNPCCharacter::ApplySocialReaction), a guard-alert, future scripting —
    // can force a fight through the SAME path. Guarded by HasAuthority + IsValid(Target). The COMMIT guard
    // (only-acquire-when-no-valid-target) stays in the perception caller; this method assumes Target was already
    // chosen. No-op off-authority or with an invalid target.
    void ForceEngageTarget(AActor *Target);

    // SERVER: toggle companion-follow for this NPC. Called by the party subsystem on recruit/remove. When active, a
    // dedicated FAST timer paces the NPC to its leader's pawn — resolved by the leader's CANONICAL key via the player
    // registry, so a co-op companion follows the player who recruited it (not always the host). TickIdleBehavior
    // early-outs while active (no schedule, no per-tick party scan). When inactive, the timer stops and the key is unused.
    void SetCompanionFollow(bool bActive, const FString &LeaderKey);

    // The absolute patrol cell for a given leg: Anchor + the leg's cardinal-ring offset (E, N, W, S, cycling). The leg
    // index wraps (incl. defensively for a negative index). Pure + static so the patrol-ring geometry is unit-testable;
    // TickIdleBehavior bounds-checks the result against the grid and skips off-grid legs so an edge-anchored guard
    // doesn't stall its whole patrol on one unreachable leg.
    static FMythicCellCoord GetPatrolCell(FMythicCellCoord Anchor, int32 LegIndex);

    // Pure: is this game hour daytime? [6,20) = day. Static + no engine state so the day/night boundary is unit-testable
    // (the activity catalog's day/night gate keys off it). Mirrors the env clock's day/night sense at a coarse grain.
    static bool IsDayHour(float Hour);

protected:
    // Sight perception that detects only enemies (affiliation filtered via GetTeamAttitudeTowards).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic AI|Perception")
    TObjectPtr<UAIPerceptionComponent> AIPerception;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    // Current hostile target selected from perception (server-only; AI controllers don't replicate).
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    TObjectPtr<AActor> CurrentHostileTarget;

    // Per-attacker threat (aggro table): how much each actor has provoked THIS NPC by damaging it. Server-only, bounded
    // by the handful of distinct attackers; pruned of stale/dead/zero entries. Used by the highest-threat target policy
    // (bThreatTargetingEnabled); empty/untouched when threat targeting is off → closest-target behaviour unchanged.
    TMap<TWeakObjectPtr<AActor>, float> ThreatTable;

    // The ASC this controller subscribed HandleThreatFromHit to (the possessed pawn's). Cached so OnUnPossess/EndPlay
    // can unbind even if GetPawn() is already null (pooled-NPC re-possession + teardown safety).
    TWeakObjectPtr<UAbilitySystemComponent> ThreatBoundASC;

    // SERVER: received-hit event handler — accrues threat[Instigator] += ComputeThreatDelta(damage,...) when threat
    // targeting is on. Bound to the pawn ASC's GAS_EVENT_DMG_RECEIVED in OnPossess. (Not a UFUNCTION — the generic
    // gameplay-event delegate is a plain multicast, bound via AddUObject, mirroring UMythicLifeComponent::HandleReceivedHit.)
    void HandleThreatFromHit(const struct FGameplayEventData *Payload);

    // Drop stale (destroyed), self, and non-positive entries so the table stays small + correct.
    void PruneThreatTable();

    // Unsubscribe HandleThreatFromHit from the cached pawn ASC + clear the table (OnUnPossess / EndPlay).
    void UnbindThreatEvent();

    // World location where the NPC ACQUIRED its current target — the leash anchor. Captured at acquisition (server-only).
    FVector EngageAnchorLocation = FVector::ZeroVector;

    // Distance (cm) within which the NPC attempts its melee attack on the current target.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float MeleeAttackRange = 180.0f;

    // Leash: if the NPC is pulled farther than this (cm) from where it engaged (EngageAnchorLocation), it gives up the
    // target and resets — prevents infinite cross-map pursuit / dragging enemy trains. 0 (default) = NO leash (infinite
    // pursuit, the prior behaviour — zero regression); a designer sets a positive range per NPC class to enable it.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat", meta = (ClampMin = "0.0"))
    float LeashRange = 0.0f;

    // How often (seconds) to attempt an attack while engaged. The ability's Cooldown GE gates the real rate.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float AttackAttemptInterval = 0.5f;

    // Distance (cm) the NPC retreats toward when its cognitive brain commits a Flee intention instead of attacking.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float FleeDistance = 800.0f;

    // AcceptanceRadius (cm) for CLOSING on a target (engage pursuit + Avenge re-pursue). MUST stay well below
    // MeleeAttackRange minus the DetourCrowd reach padding (~agent-radius*1.1 ≈ 37cm) — otherwise the crowd reach
    // test halts the agent just OUTSIDE MeleeAttackRange and it never reaches swing range. 120 leaves a safe margin.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float PursueAcceptanceRadius = 120.0f;

    // Repeating timer that drives attack attempts while a hostile target is engaged (interim driver; a proper
    // combat Behavior Tree is deferred).
    FTimerHandle AttackTimerHandle;

    // True when the in-flight move is a Flee retreat (moving AWAY). Lets a Flee↔Avenge desire flip abort the stale,
    // opposite-direction path immediately instead of waiting ~seconds for the 800cm retreat to finish.
    bool bFleeingMove = false;

    // ── Idle (out-of-combat) dispatch ──
    // How often (seconds) to re-evaluate the committed non-combat intention and steer the NPC toward a home-anchored
    // desire's cell. Combat (a valid CurrentHostileTarget) always preempts this.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float IdleDispatchInterval = 2.0f;

    // AcceptanceRadius (cm) for idle home moves; also the "already home" distance so the move isn't re-issued every
    // tick once the NPC has arrived.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float IdleMoveAcceptanceRadius = 150.0f;

    // Current leg of the patrol ring (index into the controller-local neighbour-offset ring) for a Patrol intention;
    // advances each time the NPC reaches the current patrol cell. Server-only state (the AI controller is server-side).
    int32 PatrolLegIndex = 0;

    // Stop-band (cm) a recruited companion holds from its party leader. Default is a shippable value; the tuned
    // follow distance / re-path cadence is a balance pass.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float FollowAcceptanceRadius = 250.0f;

    // Repeating timer driving out-of-combat intention dispatch (interim driver; a proper behavior tree is deferred).
    FTimerHandle IdleTimerHandle;

    // Timer callback: when NOT engaged, steer the NPC toward its committed home-anchored desire's (Defend/Rest) cell.
    void TickIdleBehavior();

    // ─── Context-driven activity dispatch (Step 3) ───
    // SERVER: additive idle branch. For a ROUTINE/neutral committed desire only, build the activity context (role/biome/
    // time/phase/nearby-merchant/NameHash from the brain + grid + env clock), weighted-pick an eligible activity from the
    // (cached) catalog, steer the NPC to the activity's resolved target, and ServerSetActivity(tag) on arrival/change. A
    // null/empty catalog or no eligible activity returns false → TickIdleBehavior falls through to its EXISTING grounded/
    // Socialize/Patrol logic verbatim (strictly additive, zero regression). Returns true if it handled steering this tick.
    bool TickActivityBehavior(class UMythicCognitiveBrainComponent *Brain, class UMythicLivingWorldSubsystem *LW,
                              const class UMythicTerritoryGrid *Grid, FMythicCellCoord LiveCell);

    // SERVER: bounded scan for the nearest MERCHANT NPC within Radius (cm) of this pawn. bOutFound is set true when one is
    // found. Capped at 12 examined NPCs (a rare idle-cadence event, not a tick path) — uses TActorIterator + an early cap,
    // NOT an overlap query. Returns the merchant actor or nullptr. Only called when a merchant-gated activity is in play.
    AActor *ScanNearbyMerchant(float Radius, bool &bOutFound) const;

    // SERVER: resolve the current game hour [0,24) from the SAME single-source env clock the ScheduleTransitionProcessor
    // uses (UMythicEnvironmentSubsystem → controller GetTimespan), falling back to world-time % DayLength when no
    // environment controller is placed. Game-thread only.
    float ResolveGameHour() const;

    // Radius (cm) within which a context activity's nearby-merchant gate searches. Designer-tunable.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle", meta = (ClampMin = "0.0"))
    float MerchantScanRadius = 1500.0f;

    // Lazily-resolved authored activity catalog (Settings->ActivityCatalog.LoadSynchronous, hoisted once). Weak so a GC of
    // the data asset is detected; DefaultActivities is the code-default fallback used when the catalog is unset.
    TWeakObjectPtr<UMythicActivityCatalog> CachedActivityCatalog;

    // Code-default activity defs, built once (MythicActivityDefaults::BuildDefaultActivities) on first activity dispatch
    // when no authored catalog is assigned. Server-only state.
    TArray<FMythicActivityDef> DefaultActivities;

    // Latches the one-time catalog/defaults resolution so we don't re-LoadSynchronous / rebuild every idle tick.
    bool bActivitySourceResolved = false;

    // ── Companion follow (dedicated fast timer, set via SetCompanionFollow) ──
    // Cadence (seconds) of the follow re-anchor. Fast so the companion paces the leader without the 2s idle-tick lag.
    float CompanionFollowInterval = 0.3f;
    // True while this NPC is a recruited companion that should follow its leader (cached — no per-tick party scan).
    bool bCompanionFollowActive = false;
    // The CANONICAL key (AMythicPlayerState::GetCanonicalPlayerKey) of the leader this companion follows; resolved to
    // the live leader pawn each tick via the player registry. Empty = no leader (companion stands put).
    FString CompanionLeaderKey;
    // Repeating fast timer driving the follow re-anchor while bCompanionFollowActive.
    FTimerHandle FollowTimerHandle;
    // Timer callback: re-anchor the move to the live leader pawn when out of the stop-band (combat/dead preempt).
    void TickCompanionFollow();

    // FROZEN-CELL #34-r2: refresh THIS embodied NPC's live coarse Identity.Cell so the game-thread spatial readers
    // (Significance proximity, WitnessPerception crime-hearing, Pressure propagation) see where the actor ACTUALLY is,
    // not its frozen spawn origin. Change-gated to a cell-boundary crossing. Game-thread only (called from the move-loop
    // timer callbacks); the sole off-thread Identity.Cell reader (CreatureEcologyProcessor) is FMythicCreatureTag-gated +
    // every embodied actor is FMythicNPCTag — disjoint archetypes, no Mass race. The home anchor is NOT this live cell
    // (InitializeFromMassEntity snapshots Schedule.HomeCell). Shared by TickCompanionFollow/TickIdleBehavior/TryAttackCurrentTarget.
    void RefreshLiveCell();

    UFUNCTION()
    void OnTargetPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus);

    // Timer callback: if the engaged target is valid, alive, and in melee range, activate the NPC's attack.
    void TryAttackCurrentTarget();

    // Tear down the current engagement (clear target + focus + attack timer + stop movement, fire the lost
    // event). Shared by the perception-loss path and the timer's self-dead / target-dead / invalid checks.
    void ReleaseHostileTarget();

    // Content hooks (Blueprint/BT) to complete the loop: actually attack / move to the target. C++ has no NPC
    // attack ability to invoke (deferred - see Docs/BACKLOG.md), so execution is left to an authored ability.
    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic AI|Combat")
    void OnEngageHostileTarget(AActor *Target);

    UFUNCTION(BlueprintImplementableEvent, Category = "Mythic AI|Combat")
    void OnHostileTargetLost(AActor *PreviousTarget);
};
