

#include "MythicAIController.h"
#include "MythicNPCCharacter.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/MythicTags_GAS.h"
#include "Settings/MythicDeveloperSettings.h"
#include "AbilitySystemGlobals.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"
#include "NavigationSystem.h"
#include "Navigation/PathFollowingComponent.h"
#include "Perception/AISense_Sight.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AI/Party/PartySubsystem.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/LivingWorldSettings.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"
#include "World/LivingWorld/Activities/ActivityTypes.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentController.h"
#include "EngineUtils.h"
#include "MassEntitySubsystem.h"
#include "Mass/Fragments/MythicMassFragments.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "GameFramework/Pawn.h"
#include "TimerManager.h"
#include "GAS/MythicTags_GAS.h"

AMythicAIController::AMythicAIController() {
    bReplicates = false;

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

    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(IdleTimerHandle, this, &AMythicAIController::TickIdleBehavior,
                                          IdleDispatchInterval,true,IdleDispatchInterval);
    }
}

FGenericTeamId AMythicAIController::GetGenericTeamId() const {
    return FGenericTeamId(1);
}

void AMythicAIController::SanitizePerception(float &SightRadius, float &LoseSightRadius, float &PeripheralAngleDegrees) {
    SightRadius = FMath::Max(0.0f, SightRadius);
    LoseSightRadius = FMath::Max(LoseSightRadius, SightRadius);
    PeripheralAngleDegrees = FMath::Clamp(PeripheralAngleDegrees, 0.0f, 180.0f);
}

void AMythicAIController::OnPossess(APawn *InPawn) {
    Super::OnPossess(InPawn);

    UnbindThreatEvent();
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        FGameplayEventMulticastDelegate &Del = MyASC->GenericGameplayEventCallbacks.FindOrAdd(GAS_EVENT_DMG_RECEIVED);
        Del.AddUObject(this, &AMythicAIController::HandleThreatFromHit);
        ThreatBoundASC = MyASC;
    }

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
    float BestThreat = 0.0f;
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

    if (Stimulus.WasSuccessfullySensed() && GetTeamAttitudeTowards(*Actor) == ETeamAttitude::Hostile) {
        if (!IsValid(CurrentHostileTarget)) {
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
                int32 ChosenIdx = INDEX_NONE;
                const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
                if (Settings && Settings->bThreatTargetingEnabled) {
                    TArray<float, TInlineAllocator<8>> Threats;
                    for (const AActor *H : Candidates) {
                        const float *Found = ThreatTable.Find(H);
                        Threats.Add(Found ? *Found : 0.0f);
                    }
                    ChosenIdx = SelectHighestThreatIndex(Threats);
                }
                if (ChosenIdx == INDEX_NONE) {
                    ChosenIdx = SelectClosestHostileIndex(DistancesSq);
                }
                if (Candidates.IsValidIndex(ChosenIdx)) {
                    Target = Candidates[ChosenIdx];
                }
            }

            ForceEngageTarget(Target);
        }
    }
    else if (CurrentHostileTarget == Actor) {
        ReleaseHostileTarget();
    }
}

void AMythicAIController::ForceEngageTarget(AActor *Target) {
    if (!HasAuthority() || !IsValid(Target)) {
        return;
    }

    CurrentHostileTarget = Target;
    if (AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(GetPawn())) {
        NPC->SetEngagedTarget(Target);
    }
    if (const APawn *MyPawn = GetPawn()) {
        EngageAnchorLocation = MyPawn->GetActorLocation();
    }
    MoveToActor(Target, PursueAcceptanceRadius);
    SetFocus(Target);
    bFleeingMove = false;
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(AttackTimerHandle, this, &AMythicAIController::TryAttackCurrentTarget,
                                          AttackAttemptInterval,true,0.0f);
    }
    OnEngageHostileTarget(Target);
}

void AMythicAIController::ReleaseHostileTarget() {
    AActor *Previous = CurrentHostileTarget;
    CurrentHostileTarget = nullptr;
    if (AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(GetPawn())) {
        NPC->SetEngagedTarget(nullptr);
    }
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
        ReleaseHostileTarget();
        return;
    }

    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        if (MyASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            ReleaseHostileTarget();
            return;
        }
    }

    if (UAbilitySystemComponent *TargetASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(CurrentHostileTarget)) {
        if (TargetASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            ReleaseHostileTarget();
            return;
        }
    }

    if (ShouldReleaseLeash(FVector::DistSquared(MyPawn->GetActorLocation(), EngageAnchorLocation), FMath::Square(LeashRange))) {
        ReleaseHostileTarget();
        return;
    }

    RefreshLiveCell();

    EMythicDesireType CommittedDesire = EMythicDesireType::FollowSchedule;
    if (UMythicCognitiveBrainComponent *Brain = MyPawn->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        const FMythicIntention &Intention = Brain->GetCurrentIntention();
        if (Intention.bValid) {
            CommittedDesire = Intention.Desire.Type;
        }
    }

    if (CommittedDesire == EMythicDesireType::Flee || CommittedDesire == EMythicDesireType::Survive) {
        if (!bFleeingMove || GetMoveStatus() != EPathFollowingStatus::Moving) {
            StopMovement();
            const FVector MyLoc = MyPawn->GetActorLocation();
            FVector FleeDir = (MyLoc - CurrentHostileTarget->GetActorLocation()).GetSafeNormal2D();
            if (FleeDir.IsNearlyZero()) {
                FleeDir = MyPawn->GetActorForwardVector();
            }
            FVector Goal = MyLoc + FleeDir * FleeDistance;
            if (UNavigationSystemV1 *Nav = UNavigationSystemV1::GetCurrent(GetWorld())) {
                FNavLocation Projected;
                if (Nav->ProjectPointToNavigation(Goal, Projected, FVector(200.0f, 200.0f, 400.0f))) {
                    Goal = Projected.Location;
                }
            }
            MoveToLocation(Goal,50.0f);
            bFleeingMove = true;
        }
        return;
    }

    const float DistSq = FVector::DistSquared(MyPawn->GetActorLocation(), CurrentHostileTarget->GetActorLocation());
    if (DistSq > FMath::Square(MeleeAttackRange)) {
        if (CommittedDesire == EMythicDesireType::Avenge && (bFleeingMove || GetMoveStatus() != EPathFollowingStatus::Moving)) {
            StopMovement();
            MoveToActor(CurrentHostileTarget, PursueAcceptanceRadius);
            SetFocus(CurrentHostileTarget);
            bFleeingMove = false;
        }
        return;
    }

    NPC->TryActivateAttack();
}

FMythicCellCoord AMythicAIController::GetPatrolCell(FMythicCellCoord Anchor, int32 LegIndex) {
    static const FMythicCellCoord Ring[4] = {
        FMythicCellCoord(1, 0), FMythicCellCoord(0, 1), FMythicCellCoord(-1, 0), FMythicCellCoord(0, -1)
    };
    const FMythicCellCoord &Off = Ring[((LegIndex % 4) + 4) % 4];
    return FMythicCellCoord(Anchor.X + Off.X, Anchor.Y + Off.Y);
}

void AMythicAIController::TickIdleBehavior() {
    if (IsValid(CurrentHostileTarget)) {
        return;
    }
    APawn *MyPawn = GetPawn();
    if (!MyPawn) {
        return;
    }
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        if (MyASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            return;
        }
    }

    if (bCompanionFollowActive) {
        return;
    }

    RefreshLiveCell();

    UMythicCognitiveBrainComponent *Brain = MyPawn->FindComponentByClass<UMythicCognitiveBrainComponent>();
    if (!Brain) {
        return;
    }
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

    const bool bRoutineDesire = (DesireType == EMythicDesireType::FollowSchedule || DesireType == EMythicDesireType::Patrol
        || DesireType == EMythicDesireType::Socialize || DesireType == EMythicDesireType::Trade
        || DesireType == EMythicDesireType::Rest);
    if (bRoutineDesire) {
        const FMythicCellCoord LiveCell = Grid->WorldToCell(MyPawn->GetActorLocation());
        if (TickActivityBehavior(Brain, LW, Grid, LiveCell)) {
            return;
        }
    }

    if (DesireType == EMythicDesireType::Patrol) {
        constexpr int32 NumLegs = 4;
        const FMythicCellCoord &Anchor = Intention.Desire.TargetCell;
        const FMythicCellCoord PatrolCell = GetPatrolCell(Anchor, PatrolLegIndex);
        if (!Grid->IsValidCoord(PatrolCell)) {
            PatrolLegIndex = (PatrolLegIndex + 1) % NumLegs;
            return;
        }
        const FVector PatrolLoc = Grid->CellToWorld(PatrolCell);
        if (FVector::DistSquared2D(MyPawn->GetActorLocation(), PatrolLoc) <= FMath::Square(IdleMoveAcceptanceRadius)) {
            PatrolLegIndex = (PatrolLegIndex + 1) % NumLegs;
        }
        else if (GetMoveStatus() != EPathFollowingStatus::Moving) {
            MoveToLocation(PatrolLoc, IdleMoveAcceptanceRadius);
        }
        return;
    }

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

    const FVector HomeLoc = Grid->CellToWorld(Intention.Desire.TargetCell);
    if (FVector::DistSquared2D(MyPawn->GetActorLocation(), HomeLoc) <= FMath::Square(IdleMoveAcceptanceRadius)) {
        return;
    }
    if (GetMoveStatus() != EPathFollowingStatus::Moving) {
        MoveToLocation(HomeLoc, IdleMoveAcceptanceRadius);
    }
}


bool AMythicAIController::IsDayHour(float Hour) {
    return Hour >= 6.0f && Hour < 20.0f;
}

float AMythicAIController::ResolveGameHour() const {
    const UWorld *World = GetWorld();
    if (!World) {
        return 12.0f;
    }
    if (const UGameInstance *GI = World->GetGameInstance()) {
        if (const UMythicEnvironmentSubsystem *Env = GI->GetSubsystem<UMythicEnvironmentSubsystem>()) {
            if (const AMythicEnvironmentController *Controller = Env->GetEnvironmentController()) {
                const FTimespan Timespan = Controller->GetTimespan();
                return Timespan.GetHours() + Timespan.GetMinutes() / 60.0f;
            }
        }
        if (const UMythicLivingWorldSubsystem *LWS = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            if (const UMythicLivingWorldSettings *Settings = LWS->GetSettings()) {
                const float DayLengthSeconds = Settings->DayLengthSeconds;
                if (DayLengthSeconds > 0.0f) {
                    const double GameTime = World->GetTimeSeconds();
                    const float DayProgress = FMath::Fmod(static_cast<float>(GameTime), DayLengthSeconds) / DayLengthSeconds;
                    return DayProgress * 24.0f;
                }
            }
        }
    }
    return 12.0f;
}

AActor *AMythicAIController::ScanNearbyMerchant(float Radius, bool &bOutFound) const {
    bOutFound = false;
    const APawn *MyPawn = GetPawn();
    UWorld *World = GetWorld();
    if (!MyPawn || !World || Radius <= 0.0f) {
        return nullptr;
    }
    const FVector MyLoc = MyPawn->GetActorLocation();
    const float RadiusSq = FMath::Square(Radius);
    AActor *BestMerchant = nullptr;
    float BestDistSq = TNumericLimits<float>::Max();
    constexpr int32 ScanCap = 12;
    int32 Examined = 0;
    for (TActorIterator<AMythicNPCCharacter> It(World); It; ++It) {
        if (Examined >= ScanCap) {
            break;
        }
        AMythicNPCCharacter *NPC = *It;
        if (!IsValid(NPC) || NPC == MyPawn) {
            continue;
        }
        if (!NPC->IsMerchant()) {
            continue;
        }
        ++Examined;
        const float DistSq = FVector::DistSquared(MyLoc, NPC->GetActorLocation());
        if (DistSq <= RadiusSq && DistSq < BestDistSq) {
            BestDistSq = DistSq;
            BestMerchant = NPC;
        }
    }
    bOutFound = (BestMerchant != nullptr);
    return BestMerchant;
}

bool AMythicAIController::TickActivityBehavior(UMythicCognitiveBrainComponent *Brain, UMythicLivingWorldSubsystem *LW,
                                              const UMythicTerritoryGrid *Grid, FMythicCellCoord LiveCell) {
    APawn *MyPawn = GetPawn();
    AMythicNPCCharacter *NPC = Cast<AMythicNPCCharacter>(MyPawn);
    if (!NPC || !Brain || !Grid) {
        return false;
    }

    if (!bActivitySourceResolved) {
        bActivitySourceResolved = true;
        if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
            if (const UMythicLivingWorldSubsystem *Subsys = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
                if (const UMythicLivingWorldSettings *Settings = Subsys->GetSettings()) {
                    if (UMythicActivityCatalog *Catalog = Settings->ActivityCatalog.LoadSynchronous()) {
                        CachedActivityCatalog = Catalog;
                    }
                }
            }
        }
        if (!CachedActivityCatalog.IsValid()) {
            MythicActivityDefaults::BuildDefaultActivities(DefaultActivities);
        }
    }

    TConstArrayView<FMythicActivityDef> Activities;
    if (const UMythicActivityCatalog *Catalog = CachedActivityCatalog.Get()) {
        Activities = Catalog->Activities;
    } else {
        Activities = DefaultActivities;
    }
    if (Activities.Num() == 0) {
        return false;
    }

    FMythicActivityContext Ctx;
    Ctx.Role = Brain->GetRole();
    Ctx.Biome = Grid->GetBiomeAtCell(LiveCell);
    Ctx.bIsDay = IsDayHour(ResolveGameHour());
    Ctx.Phase = Brain->GetCachedSchedulePhase();

    Ctx.NameHash = 0;
    const FMassEntityHandle SourceEntity = Brain->GetSourceEntity();
    if (UMassEntitySubsystem *EntitySubsystem = GetWorld() ? GetWorld()->GetSubsystem<UMassEntitySubsystem>() : nullptr) {
        FMassEntityManager &EntityManager = EntitySubsystem->GetMutableEntityManager();
        if (SourceEntity.IsValid() && EntityManager.IsEntityValid(SourceEntity)) {
            if (const FMythicIdentityFragment *Identity = EntityManager.GetFragmentDataPtr<FMythicIdentityFragment>(SourceEntity)) {
                Ctx.NameHash = Identity->NameHash;
            }
        }
    }

    bool bAnyNeedsMerchant = false;
    for (const FMythicActivityDef &A : Activities) {
        if (A.bRequiresNearbyMerchant) {
            bAnyNeedsMerchant = true;
            break;
        }
    }
    AActor *NearbyMerchant = nullptr;
    if (bAnyNeedsMerchant) {
        bool bFound = false;
        NearbyMerchant = ScanNearbyMerchant(MerchantScanRadius, bFound);
        Ctx.bHasNearbyMerchant = bFound;
    }

    const int32 ChosenIdx = MythicActivityDefaults::PickActivityIndex(Activities, Ctx);
    if (!Activities.IsValidIndex(ChosenIdx)) {
        return false;
    }
    const FMythicActivityDef &Chosen = Activities[ChosenIdx];

    bool bHasTargetLoc = false;
    FVector TargetLoc = FVector::ZeroVector;
    AActor *TargetActor = nullptr;
    switch (Chosen.TargetKind) {
    case EMythicActivityTargetKind::HomeCell:
        TargetLoc = Grid->CellToWorld(Brain->GetHomeCell());
        bHasTargetLoc = true;
        break;
    case EMythicActivityTargetKind::WorkCell:
        TargetLoc = Grid->CellToWorld(Brain->GetCachedWorkCell());
        bHasTargetLoc = true;
        break;
    case EMythicActivityTargetKind::SettlementCenter: {
        FMythicSettlementData Settlement;
        if (LW && LW->CopySettlementAtCell(LiveCell, Settlement)) {
            TargetLoc = Grid->CellToWorld(Settlement.CenterCell);
            bHasTargetLoc = true;
        }
        break;
    }
    case EMythicActivityTargetKind::NearbyMerchant:
        TargetActor = NearbyMerchant;
        break;
    case EMythicActivityTargetKind::CurrentCell:
    case EMythicActivityTargetKind::BiomeWander:
    default:
        TargetLoc = Grid->CellToWorld(LiveCell);
        bHasTargetLoc = true;
        break;
    }

    const FVector MyLoc = MyPawn->GetActorLocation();
    if (TargetActor) {
        if (FVector::DistSquared2D(MyLoc, TargetActor->GetActorLocation()) > FMath::Square(IdleMoveAcceptanceRadius)) {
            if (GetMoveStatus() != EPathFollowingStatus::Moving) {
                MoveToActor(TargetActor, IdleMoveAcceptanceRadius);
            }
        } else {
            NPC->ServerSetActivity(Chosen.ActivityTag);
        }
    } else if (bHasTargetLoc) {
        if (FVector::DistSquared2D(MyLoc, TargetLoc) > FMath::Square(IdleMoveAcceptanceRadius)) {
            if (GetMoveStatus() != EPathFollowingStatus::Moving) {
                MoveToLocation(TargetLoc, IdleMoveAcceptanceRadius);
            }
        } else {
            NPC->ServerSetActivity(Chosen.ActivityTag);
        }
    } else {
        NPC->ServerSetActivity(Chosen.ActivityTag);
    }
    return true;
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
                                          CompanionFollowInterval, true);
    }
    else {
        World->GetTimerManager().ClearTimer(FollowTimerHandle);
    }
}

void AMythicAIController::TickCompanionFollow() {
    if (!bCompanionFollowActive) {
        return;
    }
    if (IsValid(CurrentHostileTarget)) {
        return;
    }
    APawn *MyPawn = GetPawn();
    if (!MyPawn) {
        return;
    }
    if (UAbilitySystemComponent *MyASC = GetAbilitySystemComponent()) {
        if (MyASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
            return;
        }
    }
    UMythicPlayerRegistrySubsystem *Registry = GetWorld() ? GetWorld()->GetSubsystem<UMythicPlayerRegistrySubsystem>() : nullptr;
    APawn *Leader = Registry ? Registry->GetPawnForKey(CompanionLeaderKey) : nullptr;
    if (!Leader) {
        return;
    }
    if (FVector::DistSquared2D(MyPawn->GetActorLocation(), Leader->GetActorLocation())
        > FMath::Square(FollowAcceptanceRadius)) {
        MoveToActor(Leader, FollowAcceptanceRadius);
    }

    RefreshLiveCell();
}

void AMythicAIController::RefreshLiveCell() {
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
        if (FMythicSignificanceFragment *Sig = EntityManager.GetFragmentDataPtr<FMythicSignificanceFragment>(SourceEntity)) {
            Sig->bDirty = true;
        }
    }
}

void AMythicAIController::OnUnPossess() {
    UnbindThreatEvent();
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
        return;
    }
    AActor *Attacker = const_cast<AActor *>(Payload->Instigator.Get());
    if (!Attacker || Attacker == GetPawn()) {
        return;
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
            It.RemoveCurrent();
        }
    }
}

int32 AMythicAIController::CopyThreatTable(TArray<TPair<TWeakObjectPtr<AActor>, float>> &OutThreats) const {
    OutThreats.Reset();
    OutThreats.Reserve(ThreatTable.Num());
    for (const TPair<TWeakObjectPtr<AActor>, float> &Pair : ThreatTable) {
        if (Pair.Key.IsValid()) {
            OutThreats.Add(Pair);
        }
    }
    return OutThreats.Num();
}

void AMythicAIController::CopyAIDebugState(FMythicAIDebugState &Out) const {
    Out.EngageAnchorLocation = EngageAnchorLocation;
    Out.LeashRange = LeashRange;
    Out.PatrolLegIndex = PatrolLegIndex;
    Out.bFleeingMove = bFleeingMove;
    Out.bCompanionFollowActive = bCompanionFollowActive;
    Out.CompanionLeaderKey = CompanionLeaderKey;
    Out.bHasHostileTarget = (CurrentHostileTarget != nullptr);
}

UAbilitySystemComponent *AMythicAIController::GetAbilitySystemComponent() const {
    if (const AMythicNPCCharacter *MythicNPCCharacter = Cast<AMythicNPCCharacter>(GetPawn())) {
        return MythicNPCCharacter->GetAbilitySystemComponent();
    }

    return nullptr;
}

ETeamAttitude::Type AMythicAIController::GetTeamAttitudeTowards(const AActor &Other) const {
    APawn *MyPawn = GetPawn();
    UMythicCognitiveBrainComponent *MyBrain = MyPawn ? MyPawn->FindComponentByClass<UMythicCognitiveBrainComponent>() : nullptr;
    if (!MyBrain) {
        return ETeamAttitude::Neutral;
    }
    const FMythicFactionId MyFaction = MyBrain->GetFaction();
    if (!MyFaction.IsValid()) {
        return ETeamAttitude::Neutral;
    }

    const UMythicFactionDatabase *FactionDB = nullptr;
    if (const UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr) {
        if (const UMythicLivingWorldSubsystem *LW = GI->GetSubsystem<UMythicLivingWorldSubsystem>()) {
            FactionDB = LW->GetFactionDatabase();
        }
    }
    if (!FactionDB) {
        return ETeamAttitude::Neutral;
    }

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
            return ETeamAttitude::Friendly;
        }
        // The live diplomacy relation sets the baseline stance; the NPC's authored per-faction delta
        // biases it (a raider hates the law harder than his faction does), then the shared standing
        // thresholds band the sum. Data never replaces the sim, it leans on it.
        float Stance = FactionDB->GetRelationStandingBaseline(FactionDB->GetRelationship(MyFaction, OtherFaction));
        if (const AMythicNPCCharacter *MyNPC = Cast<AMythicNPCCharacter>(MyPawn)) {
            const TMap<FGameplayTag, float> &Personal = MyNPC->GetNPCDataRef().AffiliationOverrides;
            if (Personal.Num() > 0) {
                FMythicFactionData OtherData;
                if (FactionDB->GetFaction(OtherFaction, OtherData)) {
                    if (const float *Delta = Personal.Find(OtherData.FactionTag)) {
                        Stance = FMath::Clamp(Stance + *Delta, -100.0f, 100.0f);
                    }
                }
            }
        }
        const UMythicFactionStandingComponent *StandingDefaults = GetDefault<UMythicFactionStandingComponent>();
        return UMythicFactionDatabase::BandStanding(Stance,
                                                    StandingDefaults->GetHostileThreshold(),
                                                    StandingDefaults->GetFriendlyThreshold());
    }

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

    FMythicFactionData MyData;
    if (FactionDB->GetFaction(MyFaction, MyData) && !MyData.bCanNegotiate) {
        return ETeamAttitude::Hostile;
    }
    return ETeamAttitude::Neutral;
}
