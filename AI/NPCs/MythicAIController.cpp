// 


#include "MythicAIController.h"
#include "MythicNPCCharacter.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"                  // GAS_EVENT_DMG_RECEIVED (threat accrual)
#include "Settings/MythicDeveloperSettings.h"    // bThreatTargetingEnabled / ThreatPerDamage
#include "AbilitySystemGlobals.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AISense_Sight.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h" // companion follow + leader pawn
#include "Player/MythicPlayerRegistrySubsystem.h" // resolve leader canonical key -> pawn
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h" // FMythicSettlementData (Socialize -> settlement centre)
#include "MassEntitySubsystem.h" // FROZEN-CELL #34: the live-cell refresh writes the Identity fragment
#include "Mass/Fragments/MythicMassFragments.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "GAS/MythicTags_GAS.h"

AMythicAIController::AMythicAIController() {
    // AI Controllers don't replicate in multiplayer
    bReplicates = false;

    // Sight perception that surfaces only enemies. The affiliation filter calls GetTeamAttitudeTowards (made
    // live by GetGenericTeamId below), so "enemy" here means faction- + reputation-hostile to this NPC.
    AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>(TEXT("AIPerception"));
    SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>(TEXT("SightConfig"));
    SightConfig->SightRadius = 1500.0f;
    SightConfig->LoseSightRadius = 2000.0f;
    SightConfig->PeripheralVisionAngleDegrees = 90.0f;
    SightConfig->DetectionByAffiliation.bDetectEnemies = true;
    SightConfig->DetectionByAffiliation.bDetectNeutrals = false;
    SightConfig->DetectionByAffiliation.bDetectFriendlies = false;
    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->SetDominantSense(UAISense_Sight::StaticClass());
    SetPerceptionComponent(*AIPerception);
}

void AMythicAIController::BeginPlay() {
    Super::BeginPlay();

    if (AIPerception) {
        AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &AMythicAIController::OnTargetPerceptionUpdated);
    }

    // Drive out-of-combat intention dispatch on its own cadence (combat preempts via the CurrentHostileTarget gate in
    // the callback). Initial delay = one interval so the brain has committed an intention before the first dispatch.
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(IdleTimerHandle, this, &AMythicAIController::TickIdleBehavior,
                                          IdleDispatchInterval, /*bLoop=*/true, /*InitialDelay=*/IdleDispatchInterval);
    }
}

FGenericTeamId AMythicAIController::GetGenericTeamId() const {
    // A constant valid (non-NoTeam) id. The real per-actor decision lives in GetTeamAttitudeTowards; this only
    // makes the engine treat this controller as a team agent so that function is actually consulted.
    return FGenericTeamId(1);
}

void AMythicAIController::SanitizePerception(float &SightRadius, float &LoseSightRadius, float &PeripheralAngleDegrees) {
    SightRadius = FMath::Max(0.0f, SightRadius);
    // Lose-sight must be at least the sight radius — a smaller lose-radius makes a target sensed then INSTANTLY lost.
    LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
    PeripheralAngleDegrees = FMath::Clamp(PeripheralAngleDegrees, 0.0f, 180.0f);
}

void AMythicAIController::OnPossess(APawn *InPawn) {
    Super::OnPossess(InPawn);

    // Subscribe threat accrual to the possessed pawn's received-hit event (aggro table). Rebind-safe (RemoveAll first,
    // for a pooled pawn re-possession) and cached for a clean unbind. Done before the sight early-return below so it
    // is never skipped. Gated internally on the dev flag, so it's zero-cost when threat targeting is off.
    UnbindThreatEvent();
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        FGameplayEventMulticastDelegate &Del = MyASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED);
        Del.AddUObject(this, &AMythicAIController::HandleThreatFromHit);
        ThreatBoundASC = MyASC;
    }

    // Override the constructor's default sight with the possessed NPC's per-type perception (data-driven). A
    // default-constructed / MASS-baseline NPCData carries the standard defaults, so behavior is unchanged unless a
    // designer tuned the NPC's definition.
    const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(InPawn);
    if (!NPC || !SightConfig || !AIPerception) {
        return;
    }
    const FMythicNPCData Data = NPC->GetNPCData();
    float Sight = Data.SightRadius;
    float LoseSight = Data.LoseSightRadius;
    float Angle = Data.PeripheralVisionAngleDegrees;
    SanitizePerception(Sight, LoseSight, Angle);

    SightConfig->SightRadius = Sight;
    SightConfig->LoseSightRadius = LoseSight;
    SightConfig->PeripheralVisionAngleDegrees = Angle;
    AIPerception->ConfigureSense(*SightConfig);
    AIPerception->RequestStimuliListenerUpdate();
}

bool AMythicAIController::ShouldReleaseLeash(float DistSqFromAnchor, float LeashRangeSq) {
    // A non-positive range disables the leash (infinite pursuit — the default).
    return LeashRangeSq > 0.0f && DistSqFromAnchor > LeashRangeSq;
}

int32 AMythicAIController::SelectClosestHostileIndex(TConstArrayView<float> DistancesSq) {
    int32 Best = INDEX_NONE;
    float BestDistSq = TNumericLimits<float>::Max();
    for (int32 i = 0; i < DistancesSq.Num(); ++i) {
        if (DistancesSq[i] < BestDistSq) {
            BestDistSq = DistancesSq[i];
            Best = i;
        }
    }
    return Best;
}

int32 AMythicAIController::SelectHighestThreatIndex(TConstArrayView<float> Threats) {
    int32 Best = INDEX_NONE;
    float BestThreat = 0.0f; // strictly-positive: zero/negative threat never wins → caller falls back to closest
    for (int32 i = 0; i < Threats.Num(); ++i) {
        if (Threats[i] > BestThreat) {
            BestThreat = Threats[i];
            Best = i;
        }
    }
    return Best;
}

float AMythicAIController::ComputeThreatDelta(float Damage, float ThreatPerDamage, float BonusThreat) {
    return FMath::Max(0.0f, Damage) * FMath::Max(0.0f, ThreatPerDamage) + FMath::Max(0.0f, BonusThreat);
}

void AMythicAIController::OnTargetPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus) {
    if (!Actor) {
        return;
    }

    // Re-confirm hostility through the single source of truth (faction + player reputation), not just the
    // sense's affiliation pre-filter.
    if (Stimulus.WasSuccessfullySensed() && GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile) {
        // COMMIT to the current target: only acquire when we don't already have a valid one. The prior `!= Actor` test
        // switched to ANY newly-sensed hostile, so with two continuously-sensed enemies the target flip-flopped on every
        // perception update (the NPC never committed to a fight). A target is released (→ re-acquirable) on lost/dead/
        // no-longer-hostile via the else branch + ReleaseHostileTarget.
        if (!IsValid(CurrentHostileTarget)) {
            // Acquire the CLOSEST perceived hostile (sane default), not merely this stimulus's actor — so a multi-enemy
            // engage commits to the nearest threat instead of whichever perception update fired first. Each candidate is
            // re-confirmed hostile through GetTeamAttitudeTowards (the SoT), never trusting the sense pre-filter. A
            // threat-weighted policy remains the logged aggro-model design call. (Acquisition is a rare event, not a tick
            // — the transient allocation here is fine.)
            AActor *Target = Actor;
            const APawn *MyPawn = GetPawn();
            if (AIPerception && MyPawn) {
                TArray<AActor *> Perceived;
                AIPerception->GetPerceivedHostileActors(Perceived);
                TArray<AActor *, TInlineAllocator<8>> Candidates;
                TArray<float, TInlineAllocator<8>> DistancesSq;
                const FVector MyLoc = MyPawn->GetActorLocation();
                for (AActor *H : Perceived) {
                    if (IsValid(H) && GetTeamAttitudeTowards(*H) == ETeamAttitude::Hostile) {
                        Candidates.Add(H);
                        DistancesSq.Add(FVector::DistSquared(MyLoc, H->GetActorLocation()));
                    }
                }
                // Aggro/threat policy (default-off): when enabled AND some candidate has accrued threat, engage the
                // HIGHEST-threat hostile (a tank holding aggro); otherwise fall back to the CLOSEST. Threat targeting
                // off → SelectHighestThreatIndex isn't consulted → byte-identical to the closest-only behaviour.
                int32 ChosenIdx = INDEX_NONE;
                const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
                if (Settings && Settings->bThreatTargetingEnabled) {
                    TArray<float, TInlineAllocator<8>> Threats;
                    for (const AActor *H : Candidates) {
                        const float *Found = ThreatTable.Find(H);
                        Threats.Add(Found ? *Found : 0.0f);
                    }
                    ChosenIdx = SelectHighestThreatIndex(Threats); // INDEX_NONE if no candidate has positive threat
                }
                if (ChosenIdx == INDEX_NONE) {
                    ChosenIdx = SelectClosestHostileIndex(DistancesSq);
                }
                if (Candidates.IsValidIndex(ChosenIdx)) {
                    Target = Candidates[ChosenIdx];
                }
            }

            CurrentHostileTarget = Target;
            // Anchor the leash at the engage point (where we acquired). The leash check in TryAttackCurrentTarget
            // resets the NPC if it is later pulled beyond LeashRange from here.
            if (MyPawn) {
                EngageAnchorLocation = MyPawn->GetActorLocation();
            }
            // Pursue + keep facing the target (SetFocus orients the pawn even once stopped in range, so a
            // forward melee swing lands), then drive attack attempts on a timer. Content may also react to the
            // engage event.
            MoveToActor(Target, PursueAcceptanceRadius); // close to INSIDE swing range (150 stopped ~187 > 180, never swung)
            SetFocus(Target);
            bFleeingMove = false; // engage is a toward-the-target move
            if (UWorld *World = GetWorld()) {
                World->GetTimerManager().SetTimer(AttackTimerHandle, this, &AMythicAIController::TryAttackCurrentTarget,
                                                  AttackAttemptInterval, /*bLoop=*/true, /*InitialDelay=*/0.0f);
            }
            OnEngageHostileTarget(Target);
        }
    }
    else if (CurrentHostileTarget == Actor) {
        // Lost sight of (or no longer hostile to) the current target.
        ReleaseHostileTarget();
    }
}

void AMythicAIController::ReleaseHostileTarget() {
    AActor *Previous = CurrentHostileTarget;
    CurrentHostileTarget = nullptr;
    StopMovement();
    bFleeingMove = false;
    ClearFocus(EAIFocusPriority::Gameplay);
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(AttackTimerHandle);
    }
    if (Previous) {
        OnHostileTargetLost(Previous);
    }
}

void AMythicAIController::TryAttackCurrentTarget() {
    APawn *MyPawn = GetPawn();
    AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(MyPawn);
    if (!NPC || !IsValid(CurrentHostileTarget)) {
        ReleaseHostileTarget(); // also clears the timer so it stops firing on a torn-down pawn/target
        return;
    }

    // Stop (and tear down the loop) if I'm dead - otherwise this timer keeps firing on the corpse.
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        if (MyASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            ReleaseHostileTarget();
            return;
        }
    }

    // Stop if the TARGET is dead. A dead-but-not-destroyed target still in view emits no perception-loss, so
    // without this the loop would keep swinging at a corpse forever.
    if (UAbilitySystemComponent *TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentHostileTarget)) {
        if (TargetASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            ReleaseHostileTarget();
            return;
        }
    }

    // Leash: if pulled too far from where we engaged, give up the target and reset (prevents infinite cross-map
    // pursuit / dragging enemy trains). Default LeashRange 0 disables this → prior infinite-pursuit behaviour. The
    // NPC then resumes its idle/schedule behaviour (which paths it home) on the next idle dispatch.
    if (ShouldReleaseLeash(FVector::DistSquared(MyPawn->GetActorLocation(), EngageAnchorLocation), FMath::Square(LeashRange))) {
        ReleaseHostileTarget();
        return;
    }

    // FROZEN-CELL #34-r2: a fleeing / chasing (Avenge) / engaging embodied NPC moves below; keep its live cell tracking
    // the actor so the spatial readers see where the combatant actually is. After the self+target-dead early-outs (a
    // corpse never refreshes) and before the movement branches → fires every live combat tick.
    RefreshLiveCell();

    // Read the brain's committed intention ONCE (now game-thread-safe — committed on the game thread). It drives
    // whether the NPC retreats (Flee) or presses the attack relentlessly (Avenge). The scorers populate Desire.TargetCell
    // ONLY for the home-anchored Defend/Rest (= HomeCell, consumed by TickIdleBehavior); TargetEntity and the other
    // desires' TargetCell stay unset, so Avenge means "do not flee + relentlessly close on THIS already-perceived
    // target", NOT a hunt for a particular attacker.
    EMythicDesireType CommittedDesire = EMythicDesireType::FollowSchedule; // neutral baseline — no special combat behavior
    if (UMythicCognitiveBrainComponent *Brain = MyPawn->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        const FMythicIntention &Intention = Brain->GetCurrentIntention();
        if (Intention.bValid) {
            CommittedDesire = Intention.Desire.Type;
        }
    }

    // Flee OR Survive: retreat instead of swinging. Both are Threat-driven self-preservation (Flee scales by FleeWeight;
    // Survive is quadratic in Threat — ScoreSurvive). A high-threat NPC that commits EITHER should backpedal, not
    // suicidally trade blows — without this, a committed Survive fell through to the swing below (identical to a neutral
    // NPC). They share this retreat (StopMovement + navmesh-projected flee point); if the panic lifts next tick the
    // resumed melee swing still faces correctly.
    if (CommittedDesire == EMythicDesireType::Flee || CommittedDesire == EMythicDesireType::Survive) {
        // Issue a fresh retreat when one isn't already a FLEE move in flight (anti-spam), OR when the current move
        // is a stale toward-the-target move (engage/Avenge) that this Flee flip must override. StopMovement aborts
        // the stale path so we don't keep charging in for seconds before turning to run.
        if (!bFleeingMove || GetMoveStatus() != EPathFollowingStatus::Moving) {
            StopMovement();
            const FVector MyLoc = MyPawn->GetActorLocation();
            FVector FleeDir = (MyLoc - CurrentHostileTarget->GetActorLocation()).GetSafeNormal2D();
            if (FleeDir.IsNearlyZero()) {
                FleeDir = MyPawn->GetActorForwardVector(); // degenerate overlap — just move off somewhere
            }
            FVector Goal = MyLoc + FleeDir * FleeDistance;
            // Project the away-point onto the navmesh so we never path toward off-mesh space (e.g. into a wall),
            // which on a crowd controller would otherwise leave the NPC grinding against geometry.
            if (UNavigationSystemV1 *Nav = UNavigationSystemV1::GetCurrent(GetWorld())) {
                FNavLocation Projected;
                if (Nav->ProjectPointToNavigation(Goal, Projected, FVector(200.0f, 200.0f, 400.0f))) {
                    Goal = Projected.Location;
                }
            }
            MoveToLocation(Goal, /*AcceptanceRadius=*/50.0f);
            bFleeingMove = true;
        }
        return; // do NOT swing while fleeing
    }

    // Only swing when actually in melee range.
    const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), CurrentHostileTarget->GetActorLocation());
    if (DistSq > FMath::Square(MeleeAttackRange)) {
        // Avenge (committed when Wrath/Grief dominate): relentlessly close on the target instead of passively
        // waiting on the one-shot engage move — a wrathful NPC chases a target that slipped out of range, where a
        // default NPC just holds. Issue when not already pursuing (anti-spam) OR when the current move is a stale
        // Flee retreat (away) that Avenge must override (StopMovement aborts it). Acceptance is PursueAcceptanceRadius
        // (< MeleeAttackRange) so the crowd reach test stops the agent INSIDE swing range — MeleeAttackRange itself
        // stopped it ~217 (outside the 180 swing gate), so it never landed a hit.
        if (CommittedDesire == EMythicDesireType::Avenge && (bFleeingMove || GetMoveStatus() != EPathFollowingStatus::Moving)) {
            StopMovement();
            MoveToActor(CurrentHostileTarget, PursueAcceptanceRadius);
            SetFocus(CurrentHostileTarget);
            bFleeingMove = false;
        }
        return;
    }

    // Cooldown is enforced by the ability's own Cooldown GE; over-frequent attempts simply fail to activate.
    NPC->TryActivateAttack();
}

FMythicCellCoord AMythicAIController::GetPatrolCell(FMythicCellCoord Anchor, int32 LegIndex) {
    // Cardinal ring: East, North, West, South. ((idx % 4) + 4) % 4 keeps a defensively-negative leg in range.
    static const FMythicCellCoord Ring[4] = {
        FMythicCellCoord(1, 0), FMythicCellCoord(0, 1), FMythicCellCoord(-1, 0), FMythicCellCoord(0, -1)
    };
    const FMythicCellCoord &Off = Ring[((LegIndex % 4) + 4) % 4];
    return FMythicCellCoord(Anchor.X + Off.X, Anchor.Y + Off.Y);
}

void AMythicAIController::TickIdleBehavior() {
    // Combat owns movement — never fight the engage / Flee / Avenge logic.
    if (IsValid(CurrentHostileTarget)) {
        return;
    }
    APawn *MyPawn = GetPawn();
    if (!MyPawn) {
        return;
    }
    // Dead NPCs don't wander home.
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        if (MyASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            return;
        }
    }

    // A recruited companion is driven by the dedicated FAST follow timer (TickCompanionFollow, armed via
    // SetCompanionFollow) — not this 2s schedule dispatch — so early-out and never run its home schedule. The cached
    // flag avoids a per-tick party-membership scan for every NPC in the world.
    if (bCompanionFollowActive) {
        return;
    }

    // FROZEN-CELL #34-r2 (red-team fix): refresh the live cell HERE — before the intention/grid/already-home early-returns
    // below — so a roaming non-companion NPC still traversing a previously-issued path keeps its cell live even on a tick
    // where the move logic early-outs. (Reaching here = a live, non-companion, embodied NPC; companions + dead/pawn-less
    // already returned above.)
    RefreshLiveCell();

    UMythicCognitiveBrainComponent *Brain = MyPawn->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) {
        return;
    }
    // Grounded-target desires carry a real move cell set by the brain's scorer: Defend → near-home threat cell,
    // Rest → HomeCell, FollowSchedule → WorkCell (Work phase only), Avenge → the highest-confidence grievance belief
    // cell (this runs only out of combat). Patrol → HomeCell ANCHOR, around which we walk a ring (below). Every OTHER
    // desire has no authored cell, so idle dispatch leaves the NPC put rather than steering to (0,0).
    const FMythicIntention &Intention = Brain->GetCurrentIntention();
    if (!Intention.bValid) {
        return;
    }
    const EMythicDesireType DesireType = Intention.Desire.Type;
    const bool bGroundedTarget = (DesireType == EMythicDesireType::Defend || DesireType == EMythicDesireType::Rest
        || DesireType == EMythicDesireType::FollowSchedule || DesireType == EMythicDesireType::Avenge);
    if (DesireType != EMythicDesireType::Patrol && DesireType != EMythicDesireType::Socialize && !bGroundedTarget) {
        return;
    }
    // Resolve the living world + territory grid (same subsystem as faction lookups). LW is kept (non-const) for the
    // Socialize settlement-centre lookup below.
    UMythicLivingWorldSubsystem *LW = nullptr;
    UMythicTerritoryGrid *Grid = nullptr;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
        if (LW) {
            Grid = LW->GetTerritoryGrid();
        }
    }
    if (!Grid) {
        return;
    }

    // Patrol: walk a bounded ring of neighbour cells around the home anchor (Desire.TargetCell = HomeCell), stepping to
    // the next leg each time we arrive — so an Enforce-driven guard with nothing acute to do actively patrols its post
    // instead of standing still. Combat / Flee / Avenge all preempt (the CurrentHostileTarget early-return + the brain
    // re-committing a higher-utility intention). Controller-owned rotation: no brain/threading/hysteresis coupling.
    if (DesireType == EMythicDesireType::Patrol) {
        constexpr int32 NumLegs = 4; // matches GetPatrolCell's cardinal ring
        const FMythicCellCoord &Anchor = Intention.Desire.TargetCell;
        const FMythicCellCoord PatrolCell = GetPatrolCell(Anchor, PatrolLegIndex);
        // Skip an off-grid leg (an edge-anchored guard would otherwise try to reach an unreachable cell forever and,
        // never arriving, never advance the rotation — stalling the whole patrol). Advance and let the next tick try
        // the next leg. (At least two cardinal neighbours of any in-grid anchor are in-grid, so this always converges.)
        if (!Grid->IsValidCoord(PatrolCell)) {
            PatrolLegIndex = (PatrolLegIndex + 1) % NumLegs;
            return;
        }
        const FVector PatrolLoc = Grid->CellToWorld(PatrolCell);
        if (FVector::DistSquared2D(MyPawn->GetActorLocation(), PatrolLoc) <= FMath::Square(IdleMoveAcceptanceRadius)) {
            PatrolLegIndex = (PatrolLegIndex + 1) % NumLegs; // reached this post — advance to the next leg next tick
        }
        else if (GetMoveStatus() != EPathFollowingStatus::Moving) {
            MoveToLocation(PatrolLoc, IdleMoveAcceptanceRadius);
        }
        return;
    }

    // Socialize: a socially-driven NPC (high Tend/Rally, low recent contact) heads to the CENTRE of the settlement
    // governing its current cell — NPCs organically gather in the town square. A wilderness NPC (cell governed by no
    // settlement) has no social hub, so it stays put. Controller-owned destination on the game thread, and
    // CopySettlementAtCell is SimulationLock-safe, so the brain needs no worker-thread settlement access. (Behavioural
    // reading: socialise = converge where people are; the settlement's own CenterCell, no fabricated data.)
    if (DesireType == EMythicDesireType::Socialize) {
        FMythicSettlementData Settlement;
        if (LW && LW->CopySettlementAtCell(Grid->WorldToCell(MyPawn->GetActorLocation()), Settlement)) {
            const FVector CenterLoc = Grid->CellToWorld(Settlement.CenterCell);
            if (FVector::DistSquared2D(MyPawn->GetActorLocation(), CenterLoc) > FMath::Square(IdleMoveAcceptanceRadius)
                && GetMoveStatus() != EPathFollowingStatus::Moving) {
                MoveToLocation(CenterLoc, IdleMoveAcceptanceRadius);
            }
        }
        return;
    }

    // Grounded single-cell desires (Defend / Rest / FollowSchedule / Avenge): steer toward the authored cell.
    const FVector HomeLoc = Grid->CellToWorld(Intention.Desire.TargetCell);
    // Already there (within acceptance) — don't thrash the path system re-issuing the same move every tick.
    if (FVector::DistSquared2D(MyPawn->GetActorLocation(), HomeLoc) <= FMath::Square(IdleMoveAcceptanceRadius)) {
        return;
    }
    // (Re)issue only when not already moving (anti-spam). A fresh perception engage preempts via its own MoveToActor.
    if (GetMoveStatus() != EPathFollowingStatus::Moving) {
        MoveToLocation(HomeLoc, IdleMoveAcceptanceRadius);
    }
}

void AMythicAIController::SetCompanionFollow(bool bActive, const FString &LeaderKey) {
    bCompanionFollowActive = bActive;
    CompanionLeaderKey = LeaderKey;
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (bActive) {
        World->GetTimerManager().SetTimer(FollowTimerHandle, this, &AMythicAIController::TickCompanionFollow,
                                          CompanionFollowInterval, /*bLoop*/ true);
    }
    else {
        World->GetTimerManager().ClearTimer(FollowTimerHandle);
    }
}

void AMythicAIController::TickCompanionFollow() {
    if (!bCompanionFollowActive) {
        return;
    }
    // Combat owns movement — don't fight the engage/flee logic.
    if (IsValid(CurrentHostileTarget)) {
        return;
    }
    APawn *MyPawn = GetPawn();
    if (!MyPawn) {
        return;
    }
    // Dead companions don't follow.
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        if (MyASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            return;
        }
    }
    // Resolve the live leader pawn from the leader's canonical key via the player registry — works for ANY player
    // (the recruiter), not just the host, and survives the leader re-possessing a pawn.
    UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr;
    APawn *Leader = Registry ? Registry->GetPawnForKey(CompanionLeaderKey) : nullptr;
    if (!Leader) {
        return; // no leader pawn yet (key empty / leader unregistered / not possessed) — stand put, don't crash
    }
    // Re-anchor to the LIVE leader pawn whenever out of the stop-band. MoveToActor de-dupes an identical in-flight
    // goal-actor request, so re-issuing every fast tick is cheap and removes the post-arrival rubber-band.
    if (FVector::DistSquared2D(MyPawn->GetActorLocation(), Leader->GetActorLocation())
        > FMath::Square(FollowAcceptanceRadius)) {
        MoveToActor(Leader, FollowAcceptanceRadius);
    }

    // FROZEN-CELL #34-r2: refresh this companion's live coarse cell so the spatial readers track its real position.
    // Routed through the shared helper, which also serves the roaming (TickIdleBehavior) + combat (TryAttackCurrentTarget)
    // move loops so EVERY embodied moving NPC keeps a live cell, not just companions.
    RefreshLiveCell();
}

void AMythicAIController::RefreshLiveCell() {
    // FROZEN-CELL #34-r2: write THIS embodied NPC's live coarse Identity.Cell so the game-thread spatial readers —
    // significance proximity, witness hearing, pressure propagation — see where it ACTUALLY is, not its frozen spawn
    // origin. Identity.Cell is write-once-at-spawn, so a moving NPC otherwise mis-witnesses crimes + mis-propagates
    // pressure at its birth cell. Change-gated to a cell-boundary crossing (cheap integer compare intra-cell). Game-thread
    // (timer-driven), serialized with the spawner/director writers + the significance/witness/pressure readers (all
    // game-thread) — no Mass race: the one off-thread Identity.Cell reader, CreatureEcologyProcessor, is FMythicCreatureTag-
    // gated, and every embodied actor is an FMythicNPCTag AMythicNPCCharacter (disjoint archetypes). The HomeCell anchor is
    // decoupled (InitializeFromMassEntity snapshots the never-mutated Schedule.HomeCell).
    const APawn *MyPawn = GetPawn();
    if (!MyPawn) {
        return;
    }
    const AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(MyPawn);
    if (!NPC) {
        return;
    }
    const UMythicCognitiveBrainComponent *Brain = NPC->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) {
        return;
    }
    const FMassEntityHandle SourceEntity = Brain->GetSourceEntity();
    UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicTerritoryGrid *Grid = LWS ? LWS->GetTerritoryGrid() : nullptr;
    UMassEntitySubsystem *EntitySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr;
    if (!Grid || !EntitySubsystem || !SourceEntity.IsValid()) {
        return;
    }
    FMassEntityManager &EntityManager = EntitySubsystem->GetMutableEntityManager();
    if (!EntityManager.IsEntityValid(SourceEntity)) {
        return;
    }
    const FMythicCellCoord NewCell = Grid->WorldToCell(MyPawn->GetActorLocation());
    FMythicIdentityFragment &Identity = EntityManager.GetFragmentDataChecked<FMythicIdentityFragment>(SourceEntity);
    if (Identity.Cell.X != NewCell.X || Identity.Cell.Y != NewCell.Y) {
        Identity.Cell = NewCell;
        // Tie the proximity rescore to the cell change: SignificanceProcessor only recomputes ProximityScore for bDirty
        // entities, and nothing else dirties on movement (the existing dirties are event-driven). Cheap — boundary only.
        if (FMythicSignificanceFragment *Sig = EntityManager.GetFragmentDataPtr<FMythicSignificanceFragment>(SourceEntity)) {
            Sig->bDirty = true;
        }
    }
}

void AMythicAIController::OnUnPossess() {
    UnbindThreatEvent(); // drop the threat subscription before losing the pawn (pooled re-possess starts clean)
    Super::OnUnPossess();
}

void AMythicAIController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    UnbindThreatEvent();
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(AttackTimerHandle);
        World->GetTimerManager().ClearTimer(IdleTimerHandle);
        World->GetTimerManager().ClearTimer(FollowTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void AMythicAIController::UnbindThreatEvent() {
    if (UAbilitySystemComponent *BoundASC = ThreatBoundASC.Get()) {
        if (FGameplayEventMulticastDelegate *Del = BoundASC->GenericGameplayEventCallbacks.Find(GAS_EVENT_DMG_RECEIVED)) {
            Del->RemoveAll(this);
        }
    }
    ThreatBoundASC.Reset();
    ThreatTable.Empty();
}

void AMythicAIController::HandleThreatFromHit(const FGameplayEventData *Payload) {
    if (!Payload) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bThreatTargetingEnabled) {
        return; // accrue only while the feature is on → zero cost (and empty table) when off
    }
    AActor *Attacker = const_cast<AActor *>(Payload->Instigator.Get());
    if (!Attacker || Attacker == GetPawn()) {
        return; // ignore self-damage (fall / env / self-DoT) — it must not pollute the aggro table
    }
    const float Delta = ComputeThreatDelta(Payload->EventMagnitude, Settings->ThreatPerDamage, 0.0f);
    if (Delta <= 0.0f) {
        return;
    }
    ThreatTable.FindOrAdd(Attacker) += Delta;
    PruneThreatTable();
}

void AMythicAIController::PruneThreatTable() {
    for (auto It = ThreatTable.CreateIterator(); It; ++It) {
        if (!It.Key().IsValid() || It.Value() <= 0.0f) {
            It.RemoveCurrent(); // stale/destroyed attacker or drained threat — drop it (keeps the table small)
        }
    }
}

UAbilitySystemComponent *AMythicAIController::GetAbilitySystemComponent() const {
    // If controlled pawn is MythicNPCCharacter, return their AbilitySystemComponent
    if (const AMythicNPCCharacter *MythicNPCCharacter = Cast<AMythicNPCCharacter>(GetPawn())) {
        return MythicNPCCharacter->GetAbilitySystemComponent();
    }

    return nullptr;
}

ETeamAttitude::Type AMythicAIController::GetTeamAttitudeTowards(const AActor &Other) const {
    // My faction comes from my pawn's cognitive brain.
    APawn *MyPawn = GetPawn();
    UMythicCognitiveBrainComponent *MyBrain = MyPawn ? MyPawn->FindComponentByClass<UMythicCognitiveBrainComponent>() : nullptr;
    if (!MyBrain) {
        return ETeamAttitude::Neutral;
    }
    const FMythicFactionId MyFaction = MyBrain->GetFaction();
    if (!MyFaction.IsValid()) {
        return ETeamAttitude::Neutral;
    }

    // Resolve the faction database from the Living World subsystem.
    const UMythicFactionDatabase *FactionDB = nullptr;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (const UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            FactionDB = LW->GetFactionDatabase();
        }
    }
    if (!FactionDB) {
        return ETeamAttitude::Neutral;
    }

    // Resolve the other actor's faction (a perceived controller resolves to its pawn first).
    const AActor *OtherActor = &Other;
    if (const AController *OtherController = Cast<AController>(OtherActor)) {
        if (OtherController->GetPawn()) {
            OtherActor = OtherController->GetPawn();
        }
    }
    UMythicCognitiveBrainComponent *OtherBrain = OtherActor ? OtherActor->FindComponentByClass<UMythicCognitiveBrainComponent>() : nullptr;
    const FMythicFactionId OtherFaction = OtherBrain ? OtherBrain->GetFaction() : FMythicFactionId();

    if (OtherFaction.IsValid()) {
        if (OtherFaction == MyFaction) {
            return ETeamAttitude::Friendly; // same faction => allies
        }
        switch (FactionDB->GetRelationship(MyFaction, OtherFaction)) {
        case EMythicFactionRelation::Allied:
        case EMythicFactionRelation::Friendly:
            return ETeamAttitude::Friendly;
        case EMythicFactionRelation::Unfriendly:
        case EMythicFactionRelation::Hostile:
            return ETeamAttitude::Hostile;
        case EMythicFactionRelation::Neutral:
        default:
            return ETeamAttitude::Neutral;
        }
    }

    // The other actor has no faction (e.g. a player). First consult the player's per-faction standing: a
    // sufficiently bad standing toward MY faction makes me Hostile, a good one makes me Friendly (backlog #14).
    if (const APawn *OtherPawn = Cast<APawn>(OtherActor)) {
        if (const AMythicPlayerState *OtherPS = OtherPawn->GetPlayerState<AMythicPlayerState>()) {
            if (const UMythicFactionStandingComponent *Standing = OtherPS->GetFactionStanding()) {
                const float StandingValue = Standing->GetStanding(MyFaction);
                if (StandingValue <= Standing->GetHostileThreshold()) {
                    return ETeamAttitude::Hostile;
                }
                if (StandingValue >= Standing->GetFriendlyThreshold()) {
                    return ETeamAttitude::Friendly;
                }
            }
        }
    }

    // No decisive standing: mindless / non-negotiable factions (creatures, undead) attack on sight; sentient
    // factions stay neutral to factionless actors.
    FMythicFactionData MyData;
    if (FactionDB->GetFaction(MyFaction, MyData) && !MyData.bCanNegotiate) {
        return ETeamAttitude::Hostile;
    }
    return ETeamAttitude::Neutral;
}
