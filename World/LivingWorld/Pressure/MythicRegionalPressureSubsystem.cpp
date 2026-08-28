
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"

#include "World/LivingWorld/Pressure/MythicTags_Pressure.h"
#include "World/Farming/MythicFarmPlot.h"
#include "World/Farming/MythicTags_Farming.h"
#include "World/Camping/MythicInfluenceSourceComponent.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicAIController.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Mythic.h"


bool UMythicRegionalPressureSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicRegionalPressureSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        Config = Settings->RegionalPressure;
        FarmingConfig = Settings->Farming;
        bFarmRaidsEnabled = Settings->bEnableFarmRaids;
        HarvestConfig = Settings->HarvestPressure;
        bHarvestPressureEnabled = Settings->bHarvestPressureEnabled;
    }
}

void UMythicRegionalPressureSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(CheckTimerHandle);
    }
    Sources.Reset();
    Cells.Reset();
    HabituationByCell.Reset();
    PendingRaids.Reset();
    LastRaidTimeByCell.Reset();
    Super::Deinitialize();
}

bool UMythicRegionalPressureSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

double UMythicRegionalPressureSubsystem::Now() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}

FIntPoint UMythicRegionalPressureSubsystem::CellOf(const FVector &Location) const {
    const UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    if (LWS && LWS->IsSystemActive()) {
        if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
            const FMythicCellCoord Coord = Grid->WorldToCell(Location);
            return FIntPoint(Coord.X, Coord.Y);
        }
    }
    return FMythicRegionalPressureRules::QuantizeToCell(Location, Config.FallbackCellSizeCm);
}


void UMythicRegionalPressureSubsystem::AddPressure(const FVector &Location, const FGameplayTag &Channel, float Amount) {
    if (!IsAuthority() || !Channel.IsValid() || Amount <= 0.0f) {
        return;
    }
    FMythicPressureCellState &State = Cells.FindOrAdd(FMythicPressureKey(CellOf(Location), Channel));
    FMythicRegionalPressureRules::Accumulate(State, Amount, Now(), Config.DecayPerSecond);
}

float UMythicRegionalPressureSubsystem::QueryPressure(const FVector &Location, const FGameplayTag &Channel) {
    if (!Channel.IsValid()) {
        return 0.0f;
    }
    const FMythicPressureCellState *State = Cells.Find(FMythicPressureKey(CellOf(Location), Channel));
    if (!State) {
        return 0.0f;
    }
    return FMythicRegionalPressureRules::ValueAtTime(State->Value, State->LastUpdateTime, Now(), Config.DecayPerSecond);
}


void UMythicRegionalPressureSubsystem::RegisterRatedSource(AActor *Source, const FGameplayTag &Channel, float RatePerSecond) {
    if (!IsAuthority() || !Source || !Channel.IsValid()) {
        return;
    }
    for (FRatedSource &Existing : Sources) {
        if (Existing.Actor == Source && Existing.Channel == Channel) {
            Existing.RatePerSecond = FMath::Max(0.0f, RatePerSecond);
            return;
        }
    }
    FRatedSource NewSource;
    NewSource.Actor = Source;
    NewSource.Channel = Channel;
    NewSource.RatePerSecond = FMath::Max(0.0f, RatePerSecond);
    NewSource.LastAccrueTime = Now();
    Sources.Add(NewSource);
    UpdateCheckTimer();
}

void UMythicRegionalPressureSubsystem::UnregisterRatedSource(AActor *Source, const FGameplayTag &Channel) {
    if (!Source) {
        return;
    }
    const double CurrentTime = Now();
    const int32 Removed = Sources.RemoveAll([&](FRatedSource &Entry) {
        if (Entry.Actor != Source || Entry.Channel != Channel) {
            return false;
        }
        if (AActor *Actor = Entry.Actor.Get()) {
            const float Tail = Entry.RatePerSecond * static_cast<float>(FMath::Max(0.0, CurrentTime - Entry.LastAccrueTime));
            if (Tail > 0.0f) {
                FMythicPressureCellState &State = Cells.FindOrAdd(FMythicPressureKey(CellOf(Actor->GetActorLocation()), Entry.Channel));
                FMythicRegionalPressureRules::Accumulate(State, Tail, CurrentTime, Config.DecayPerSecond);
            }
        }
        return true;
    });
    if (Removed > 0) {
        UpdateCheckTimer();
    }
}

void UMythicRegionalPressureSubsystem::RegisterFarmPlot(AMythicFarmPlot *Plot) {
    RegisterRatedSource(Plot, TAG_Pressure_Farm, Config.FarmPressurePerMaturePlotPerSecond);
}

void UMythicRegionalPressureSubsystem::UnregisterFarmPlot(AMythicFarmPlot *Plot) {
    UnregisterRatedSource(Plot, TAG_Pressure_Farm);
}

void UMythicRegionalPressureSubsystem::NotifyHuntingKillNear(const FVector &Location) {
    if (!IsAuthority()) {
        return;
    }
    if (FMythicPressureCellState *Habituation = HabituationByCell.Find(CellOf(Location))) {
        Habituation->Value = 0.0f;
        Habituation->LastUpdateTime = Now();
    }
}


void UMythicRegionalPressureSubsystem::ServerRegisterHarvest(const FVector &Location, float Amount) {
    const float Push = Amount > 0.0f ? Amount : FMath::Max(0.0f, HarvestConfig.HarvestPressurePerGather);
    if (Push > 0.0f) {
        AddPressure(Location, TAG_Pressure_Harvest, Push);
    }
}

float UMythicRegionalPressureSubsystem::QueryHarvestYieldMultiplier(const FVector &Location) {
    if (!bHarvestPressureEnabled) {
        return 1.0f;
    }
    const float Pressure = QueryPressure(Location, TAG_Pressure_Harvest);
    return FMythicHarvestPressureRules::DepletionYieldMultiplier(Pressure, HarvestConfig);
}

float UMythicRegionalPressureSubsystem::ScaledHarvestRespawnDelay(const FVector &Location, float BaseDelay) {
    if (BaseDelay <= 0.0f) {
        return BaseDelay;
    }
    if (!bHarvestPressureEnabled) {
        return BaseDelay;
    }
    const float Pressure = QueryPressure(Location, TAG_Pressure_Harvest);
    return BaseDelay * FMythicHarvestPressureRules::RespawnDelayMultiplier(Pressure, HarvestConfig);
}

bool UMythicRegionalPressureSubsystem::IsHarvestRespawnGated(const FVector &Location) {
    if (!bHarvestPressureEnabled) {
        return false;
    }
    const float Pressure = QueryPressure(Location, TAG_Pressure_Harvest);
    return FMythicHarvestPressureRules::IsRespawnGated(Pressure, HarvestConfig.RespawnGateThreshold);
}

EMythicYieldQuality UMythicRegionalPressureSubsystem::DepleteHarvestQuality(const FVector &Location, EMythicYieldQuality RolledTier) {
    if (!bHarvestPressureEnabled) {
        return RolledTier;
    }
    const float Pressure = QueryPressure(Location, TAG_Pressure_Harvest);
    const int32 DropSteps = FMythicHarvestPressureRules::QualityTierDropSteps(Pressure, HarvestConfig.PressurePerQualityTierDrop);
    return FMythicYieldQuality::DepleteTier(RolledTier, DropSteps, EMythicYieldQuality::Common);
}

int32 UMythicRegionalPressureSubsystem::GetLiveSourceCount() const {
    int32 Count = 0;
    for (const FRatedSource &Source : Sources) {
        if (Source.Actor.IsValid()) {
            ++Count;
        }
    }
    return Count;
}


void UMythicRegionalPressureSubsystem::UpdateCheckTimer() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    const bool bAnySource = GetLiveSourceCount() > 0;
    FTimerManager &Timers = World->GetTimerManager();
    if (bAnySource && !Timers.IsTimerActive(CheckTimerHandle)) {
        const float Interval = FMath::Max(10.0f, Config.CheckIntervalSeconds);
        Timers.SetTimer(CheckTimerHandle, this, &UMythicRegionalPressureSubsystem::HandleCheck, Interval, true,
 Interval);
    }
    else if (!bAnySource && Timers.IsTimerActive(CheckTimerHandle)) {
        Timers.ClearTimer(CheckTimerHandle);
        PendingRaids.Reset();
    }
}

void UMythicRegionalPressureSubsystem::HandleCheck() {
    if (!IsAuthority()) {
        return;
    }
    const double CurrentTime = Now();

    struct FCluster {
        FVector Sum = FVector::ZeroVector;
        int32 Count = 0;
    };
    TMap<FIntPoint, FCluster> FarmClusters;

    bool bAnyDead = false;
    for (FRatedSource &Source : Sources) {
        AActor *Actor = Source.Actor.Get();
        if (!Actor) {
            bAnyDead = true;
            continue;
        }
        const float Dt = static_cast<float>(FMath::Max(0.0, CurrentTime - Source.LastAccrueTime));
        Source.LastAccrueTime = CurrentTime;
        const FVector Location = Actor->GetActorLocation();
        const FIntPoint Cell = CellOf(Location);
        if (Source.RatePerSecond > 0.0f && Dt > 0.0f) {
            FMythicPressureCellState &State = Cells.FindOrAdd(FMythicPressureKey(Cell, Source.Channel));
            FMythicRegionalPressureRules::Accumulate(State, Source.RatePerSecond * Dt, CurrentTime, Config.DecayPerSecond);
        }
        if (Source.Channel == TAG_Pressure_Farm) {
            FCluster &Cluster = FarmClusters.FindOrAdd(Cell);
            Cluster.Sum += Location;
            ++Cluster.Count;
        }
    }
    if (bAnyDead) {
        Sources.RemoveAll([](const FRatedSource &Entry) { return !Entry.Actor.IsValid(); });
    }

    ProcessDueRaids(CurrentTime);

    if (bFarmRaidsEnabled) {
        for (const TPair<FIntPoint, FCluster> &Pair : FarmClusters) {
            EvaluateFarmCell(Pair.Key, Pair.Value.Sum / FMath::Max(1, Pair.Value.Count), CurrentTime);
        }
    }

    for (auto It = Cells.CreateIterator(); It; ++It) {
        if (FMythicRegionalPressureRules::ValueAtTime(It->Value.Value, It->Value.LastUpdateTime, CurrentTime, Config.DecayPerSecond) <= 0.0f) {
            It.RemoveCurrent();
        }
    }
    UpdateCheckTimer();
}


void UMythicRegionalPressureSubsystem::EvaluateFarmCell(const FIntPoint &Cell, const FVector &Center, double CurrentTime) {
    for (const FPendingFarmRaid &Pending : PendingRaids) {
        if (Pending.Cell == Cell) {
            return;
        }
    }
    if (const double *Last = LastRaidTimeByCell.Find(Cell)) {
        if (CurrentTime - *Last < Config.PerCellCooldownSeconds) {
            return;
        }
    }

    FMythicPressureCellState *State = Cells.Find(FMythicPressureKey(Cell, TAG_Pressure_Farm));
    if (!State) {
        return;
    }
    const float Value = FMythicRegionalPressureRules::Resolve(*State, CurrentTime, Config.DecayPerSecond);
    if (Value < Config.FarmRaidThreshold || Config.FarmRaidThreshold <= 0.0f) {
        return;
    }

    const float RawDeterrence = UMythicInfluenceSourceComponent::GetTotalInfluenceAt(GetWorld(), Center, TAG_Influence_Deterrence);
    FMythicPressureCellState &Habituation = HabituationByCell.FindOrAdd(Cell);
    const float Deposits = FMythicFarmingRules::HabituationAtTime(Habituation.Value, Habituation.LastUpdateTime, CurrentTime,
                                                                  FarmingConfig.HabituationDecayPerSecond);
    const float EffectiveDeterrence = FMythicFarmingRules::DeterrenceEffectiveness(RawDeterrence, Deposits,
                                                                                   FarmingConfig.HabituationEffectFactor);
    const float EffectiveThreshold = FMythicRegionalPressureRules::EffectiveRaidThreshold(
        Config.FarmRaidThreshold, EffectiveDeterrence, Config.DeterrenceThresholdFactor);

    if (Value < EffectiveThreshold) {
        Habituation.Value = Deposits + FarmingConfig.HabituationPerDeterredCheck;
        Habituation.LastUpdateTime = CurrentTime;
        return;
    }

    if (IsPacingRestPhase()) {
        return;
    }
    if (!NearestPlayerPawn(Center, Config.PlayerNearRadius)) {
        return;
    }
    if (!Config.RaidNPCType.IsValid()) {
        if (!bWarnedMissingRaidContent) {
            bWarnedMissingRaidContent = true;
            UE_LOG(Myth, Warning,
                   TEXT("RegionalPressure: bEnableFarmRaids is ON but RegionalPressure.RaidNPCType is unset — no raid "
                        "will spawn. Author an NPC type tag (CONTENT) to activate farm raids."));
        }
        return;
    }

    SubmitFarmChronicle(Center, false);
    FPendingFarmRaid Pending;
    Pending.Cell = Cell;
    Pending.Center = Center;
    Pending.DueTime = CurrentTime + FMath::Max(0.0f, Config.TelegraphDelaySeconds);
    Pending.PressureAtTelegraph = Value;
    PendingRaids.Add(Pending);
    UE_LOG(Myth, Log, TEXT("RegionalPressure: FARM RAID telegraphed at cell (%d,%d) (pressure %.1f, fires in %.0fs)"),
           Cell.X, Cell.Y, Value, Config.TelegraphDelaySeconds);
}

void UMythicRegionalPressureSubsystem::ProcessDueRaids(double CurrentTime) {
    for (int32 i = PendingRaids.Num() - 1; i >= 0; --i) {
        const FPendingFarmRaid Pending = PendingRaids[i];
        if (Pending.DueTime > CurrentTime) {
            continue;
        }
        if (IsPacingRestPhase()) {
            continue;
        }
        if (NearestPlayerPawn(Pending.Center, Config.PlayerNearRadius)) {
            LastRaidTimeByCell.Add(Pending.Cell, CurrentTime);
            DispatchFarmRaid(Pending);
            SubmitFarmChronicle(Pending.Center, true);
        }
        else {
            UE_LOG(Myth, Log, TEXT("RegionalPressure: pending farm raid dissolved (party left the fields)"));
        }
        PendingRaids.RemoveAt(i);
    }
}

void UMythicRegionalPressureSubsystem::DispatchFarmRaid(const FPendingFarmRaid &Raid) {
    UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicNPCManager *NPCManager = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr;
    if (!World || !NPCManager || !Config.RaidNPCType.IsValid()) {
        return;
    }
    APawn *TargetPawn = NearestPlayerPawn(Raid.Center, Config.PlayerNearRadius);
    if (!TargetPawn) {
        return;
    }

    const int32 PackCount = FMythicRegionalPressureRules::RaidPackCount(Raid.PressureAtTelegraph, Config.FarmRaidThreshold,
                                                                        Config.RaidBaseCount, Config.RaidMaxCount);
    const float MinDist = FMath::Max(500.0f, Config.MinSpawnDistance);
    const float MaxDist = FMath::Max(MinDist, Config.MaxSpawnDistance);
    int32 Spawned = 0;
    for (int32 i = 0; i < PackCount; ++i) {
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        const float Dist = FMath::FRandRange(MinDist, MaxDist);

        FMythicPlacementParams Params;
        Params.CellCenterXY = Raid.Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
        Params.ScatterRadius = 400.0f;

        FTransform SpawnXf;
        if (!MythicPlacement::FindValidSpawn(World, Params, SpawnXf)) {
            continue;
        }
        AMythicNPCCharacter *Raider =
            NPCManager->SpawnRandomNPC(Config.RaidNPCType, SpawnXf.GetLocation(), SpawnXf.GetRotation().Rotator());
        if (!Raider) {
            UE_LOG(Myth, Warning, TEXT("RegionalPressure: SpawnRandomNPC failed for raid type %s — check the NPC type data table (CONTENT)"),
                   *Config.RaidNPCType.ToString());
            break;
        }
        if (UMythicCognitiveBrainComponent *Brain = Raider->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
            Brain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK, Brain->GetHomeCell());
        }
        if (AMythicAIController *AI = Cast<AMythicAIController>(Raider->GetController())) {
            AI->ForceEngageTarget(TargetPawn);
        }
        ++Spawned;
    }

    TArray<AMythicFarmPlot *> CellPlots;
    for (const FRatedSource &Source : Sources) {
        if (Source.Channel != TAG_Pressure_Farm) {
            continue;
        }
        AMythicFarmPlot *Plot = Cast<AMythicFarmPlot>(Source.Actor.Get());
        if (Plot && CellOf(Plot->GetActorLocation()) == Raid.Cell) {
            CellPlots.Add(Plot);
        }
    }
    for (AMythicFarmPlot *Plot : CellPlots) {
        Plot->ServerApplyRaidStageRegression(Config.StageRegressionPerRaid);
    }

    if (FMythicPressureCellState *State = Cells.Find(FMythicPressureKey(Raid.Cell, TAG_Pressure_Farm))) {
        State->Value = 0.0f;
        State->LastUpdateTime = Now();
    }

    UE_LOG(Myth, Log, TEXT("RegionalPressure: farm raid — %d/%d raider(s) on the fields at %s, %d plot(s) trampled (stage regression only)"),
           Spawned, PackCount, *Raid.Center.ToCompactString(), CellPlots.Num());
}

void UMythicRegionalPressureSubsystem::SubmitFarmChronicle(const FVector &NearLocation, bool bDispatched) const {
    const UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    FMythicCellCoord Cell;
    if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
        Cell = Grid->WorldToCell(NearLocation);
    }
    FMythicWorldEvent Event;
    Event.EventTag = bDispatched ? TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED : TAG_LIVINGWORLD_EVENT_SCHEME_DISCOVERED;
    Event.Cell = Cell;
    Event.WorldTime = World ? World->GetTimeSeconds() : 0.0;
    Event.Significance = bDispatched ? 0.7f : 0.55f;
    Event.CategoryFlags = EMythicEventCategory::Scheme;
    LWS->SubmitWorldEvent(Event);
}


bool UMythicRegionalPressureSubsystem::IsPacingRestPhase() const {
    if (const UWorld *World = GetWorld()) {
        if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
            return Pacing->GetPhase() == EMythicDirectorPhase::Rest;
        }
    }
    return false;
}

APawn *UMythicRegionalPressureSubsystem::NearestPlayerPawn(const FVector &Location, float MaxRadius) const {
    const UWorld *World = GetWorld();
    if (!World) {
        return nullptr;
    }
    APawn *Best = nullptr;
    float BestDistSq = FMath::Square(MaxRadius);
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        const APlayerController *PC = It->Get();
        APawn *Pawn = PC ? PC->GetPawn() : nullptr;
        if (!Pawn) {
            continue;
        }
        const float DistSq = FVector::DistSquared(Location, Pawn->GetActorLocation());
        if (DistSq <= BestDistSq) {
            BestDistSq = DistSq;
            Best = Pawn;
        }
    }
    return Best;
}
