// 

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "AbilitySystemInterface.h"
#include "DetourCrowdAIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "MythicAIController.generated.h"

class UMythicAttributeSet_NPCCombat;
class UMythicAbilitySystemComponent;
class UAIPerceptionComponent;
class UAISenseConfig_Sight;

/// Every NPC will have a name, and an Ability System Component with LifeAttributeSet
/// AIController's are not replicated, so values needed on the clients should go in the NPCCharacter
UCLASS(Blueprintable, BlueprintType, Abstract)
class MYTHIC_API AMythicAIController : public ADetourCrowdAIController, public IAbilitySystemInterface {
    GENERATED_BODY()

public:
    AMythicAIController();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

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

    // SERVER: toggle companion-follow for this NPC. Called by the party subsystem on recruit/remove. When active, a
    // dedicated FAST timer paces the NPC to its party leader (PlayerId) — smoother than piggybacking the 2s idle timer —
    // and TickIdleBehavior early-outs (no schedule, no per-tick party scan). When inactive, the timer stops.
    void SetCompanionFollow(bool bActive, int32 PlayerId);

protected:
    // Sight perception that detects only enemies (affiliation filtered via GetTeamAttitudeTowards).
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Mythic AI|Perception")
    TObjectPtr<UAIPerceptionComponent> AIPerception;

    UPROPERTY()
    TObjectPtr<UAISenseConfig_Sight> SightConfig;

    // Current hostile target selected from perception (server-only; AI controllers don't replicate).
    UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    TObjectPtr<AActor> CurrentHostileTarget;

    // Distance (cm) within which the NPC attempts its melee attack on the current target.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Combat")
    float MeleeAttackRange = 180.0f;

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

    // Stop-band (cm) a recruited companion holds from its party leader. Default is a shippable value; the tuned
    // follow distance / re-path cadence is a balance pass.
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Mythic AI|Idle")
    float FollowAcceptanceRadius = 250.0f;

    // Repeating timer driving out-of-combat intention dispatch (interim driver; a proper behavior tree is deferred).
    FTimerHandle IdleTimerHandle;

    // Timer callback: when NOT engaged, steer the NPC toward its committed home-anchored desire's (Defend/Rest) cell.
    void TickIdleBehavior();

    // ── Companion follow (dedicated fast timer, set via SetCompanionFollow) ──
    // Cadence (seconds) of the follow re-anchor. Fast so the companion paces the leader without the 2s idle-tick lag.
    float CompanionFollowInterval = 0.3f;
    // True while this NPC is a recruited companion that should follow its leader (cached — no per-tick party scan).
    bool bCompanionFollowActive = false;
    // The player whose pawn this companion follows (resolved via the party subsystem's GetLeaderPawn).
    int32 CompanionLeaderPlayerId = 0;
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
