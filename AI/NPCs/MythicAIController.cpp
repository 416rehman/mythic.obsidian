// 


#include "MythicAIController.h"
#include "MythicNPCCharacter.h"
#include "GAS/MythicAbilitySystemComponent.h"
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
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
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

void AMythicAIController::OnTargetPerceptionUpdated(AActor *Actor, FAIStimulus Stimulus) {
    if (!Actor) {
        return;
    }

    // Re-confirm hostility through the single source of truth (faction + player reputation), not just the
    // sense's affiliation pre-filter.
    if (Stimulus.WasSuccessfullySensed() && GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile) {
        if (CurrentHostileTarget != Actor) {
            CurrentHostileTarget = Actor;
            // Pursue + keep facing the target (SetFocus orients the pawn even once stopped in range, so a
            // forward melee swing lands), then drive attack attempts on a timer. Content may also react to the
            // engage event.
            MoveToActor(Actor, PursueAcceptanceRadius); // close to INSIDE swing range (150 stopped ~187 > 180, never swung)
            SetFocus(Actor);
            bFleeingMove = false; // engage is a toward-the-target move
            if (UWorld *World = GetWorld()) {
                World->GetTimerManager().SetTimer(AttackTimerHandle, this, &AMythicAIController::TryAttackCurrentTarget,
                                                  AttackAttemptInterval, /*bLoop=*/true, /*InitialDelay=*/0.0f);
            }
            OnEngageHostileTarget(Actor);
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
    // Only the grounded-target desires carry a real move cell set by the brain's scorer: Defend → near-home threat cell,
    // Rest → HomeCell, FollowSchedule → WorkCell (Work phase only), Avenge → the highest-confidence grievance belief cell
    // (this runs only out of combat — the early CurrentHostileTarget return above hands in-combat Avenge to the engage
    // logic). Every OTHER desire has no authored cell, so idle dispatch leaves the NPC put rather than steering to (0,0).
    const FMythicIntention &Intention = Brain->GetCurrentIntention();
    if (!Intention.bValid ||
        (Intention.Desire.Type != EMythicDesireType::Defend && Intention.Desire.Type != EMythicDesireType::Rest
            && Intention.Desire.Type != EMythicDesireType::FollowSchedule && Intention.Desire.Type != EMythicDesireType::Avenge)) {
        return;
    }
    // Resolve the territory grid to turn the home cell into a world location (same subsystem as faction lookups).
    UMythicTerritoryGrid *Grid = nullptr;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (const UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            Grid = LW->GetTerritoryGrid();
        }
    }
    if (!Grid) {
        return;
    }
    const FVector HomeLoc = Grid->CellToWorld(Intention.Desire.TargetCell);
    // Already home (within acceptance) — don't thrash the path system re-issuing the same move every tick.
    if (FVector::DistSquared2D(MyPawn->GetActorLocation(), HomeLoc) <= FMath::Square(IdleMoveAcceptanceRadius)) {
        return;
    }
    // (Re)issue only when not already moving (anti-spam). A fresh perception engage preempts via its own MoveToActor.
    if (GetMoveStatus() != EPathFollowingStatus::Moving) {
        MoveToLocation(HomeLoc, IdleMoveAcceptanceRadius);
    }
}

void AMythicAIController::SetCompanionFollow(bool bActive, int32 PlayerId) {
    bCompanionFollowActive = bActive;
    CompanionLeaderPlayerId = PlayerId;
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
    UMythicPartySubsystem *Party = GetWorld() ? GetWorld()->GetSubsystem<UMythicPartySubsystem>() : nullptr;
    APawn *Leader = Party ? Party->GetLeaderPawn(CompanionLeaderPlayerId) : nullptr;
    if (!Leader) {
        return; // no leader pawn yet (e.g. not possessed) — stand put, don't crash
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

void AMythicAIController::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(AttackTimerHandle);
        World->GetTimerManager().ClearTimer(IdleTimerHandle);
        World->GetTimerManager().ClearTimer(FollowTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
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
