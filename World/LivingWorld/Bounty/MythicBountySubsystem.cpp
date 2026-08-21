
#include "World/LivingWorld/Bounty/MythicBountySubsystem.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Rewards/LootReward.h"
#include "GAS/MythicTags_GAS.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicAIController.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"
#include "TimerManager.h"


bool UMythicBountySubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    if (World->GetNetMode() == NM_Client) {
        return false;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    return Settings && Settings->bEnableBountyHunters;
}

void UMythicBountySubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }

    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        Config = Settings->BountyHunters;
    }

    if (UGameInstance *GI = InWorld.GetGameInstance()) {
        LivingWorld = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }

    const float Interval = FMath::Max(5.0f, Config.CheckIntervalSeconds);
    InWorld.GetTimerManager().SetTimer(CheckTimerHandle, this, &UMythicBountySubsystem::HandleBountyCheck, Interval,
true,Interval);

    UE_LOG(LogMythLivingWorld, Log,
           TEXT("BountySubsystem: live (check=%.0fs, tiers=%d, chance=%.2f, cooldown=%.0fs, telegraph=%.0fs, cap=%d)"),
           Interval, Config.TierThresholds.Num(), Config.SpawnChancePerCheck, Config.CooldownSeconds,
           Config.TelegraphDelaySeconds, Config.MaxSimultaneousHunters);
}

void UMythicBountySubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(CheckTimerHandle);
    }
    PendingDispatches.Reset();
    LastDispatchTime.Reset();
    HuntersByPlayer.Reset();
    ActiveBountyObjectives.Reset();
    LivingWorld = nullptr;
    Super::Deinitialize();
}

bool UMythicBountySubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

int32 UMythicBountySubsystem::GetTrackedHunterCount() const {
    int32 Count = 0;
    for (const TPair<FString, TArray<TWeakObjectPtr<AMythicNPCCharacter>>> &Pair : HuntersByPlayer) {
        for (const TWeakObjectPtr<AMythicNPCCharacter> &Hunter : Pair.Value) {
            if (Hunter.IsValid()) {
                ++Count;
            }
        }
    }
    return Count;
}


void UMythicBountySubsystem::HandleBountyCheck() {
    if (!IsAuthority()) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    const double Now = World->GetTimeSeconds();

    RefreshHunterPursuit();
    ProcessDueDispatches(Now);
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
        if (PC && PC->HasAuthority()) {
            EvaluatePlayer(PC, Now);
        }
    }
}


void UMythicBountySubsystem::EvaluatePlayer(AMythicPlayerController *PC, double Now) {
    UWorld *World = GetWorld();
    if (!World || !PC) {
        return;
    }
    const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
    if (!PS) {
        return;
    }
    const FString PlayerKey = PS->GetCanonicalPlayerKey();
    if (PlayerKey.IsEmpty()) {
        return;
    }
    const UMythicFactionStandingComponent *Standing = PS->GetFactionStanding();
    if (!Standing) {
        return;
    }

    float Notoriety = 0.0f;
    FMythicFactionId WorstFaction;
    for (const FMythicFactionStandingEntry &Entry : Standing->GetStandings()) {
        if (!Entry.Faction.IsValid()) {
            continue;
        }
        const float Heat = -Entry.Value;
        if (Heat > Notoriety) {
            Notoriety = Heat;
            WorstFaction = Entry.Faction;
        }
    }

    const int32 Tier = MythicBounty::ResolveBountyTier(Notoriety, Config.TierThresholds);

    if (Tier < 0) {
        if (PendingDispatches.Remove(PlayerKey) > 0) {
            UE_LOG(LogMythLivingWorld, Log, TEXT("Bounty: the trail on '%s' went cold — pending hunt called off"), *PlayerKey);
        }
        return;
    }

    if (PendingDispatches.Contains(PlayerKey)) {
        return;
    }

    if (!Config.HunterNPCType.IsValid()) {
        if (!bWarnedMissingHunterContent) {
            bWarnedMissingHunterContent = true;
            UE_LOG(LogMythLivingWorld, Warning,
                   TEXT("Bounty: bEnableBountyHunters is ON but BountyHunters.HunterNPCType is unset — no hunts will "
                        "dispatch. Author a hunter NPC type tag (CONTENT) to activate the system."));
        }
        return;
    }

    if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
        if (Pacing->GetPhase() == EMythicDirectorPhase::Rest) {
            return;
        }
    }

    const double *Last = LastDispatchTime.Find(PlayerKey);
    const double TimeSinceLast = Last ? (Now - *Last) : TNumericLimits<double>::Max();
    const int32 LiveHunters = CountLiveHunters(PlayerKey);

    if (!MythicBounty::ShouldDispatchHunters(Tier, TimeSinceLast, Config.CooldownSeconds, LiveHunters,
                                             Config.MaxSimultaneousHunters, FMath::FRand(), Config.SpawnChancePerCheck)) {
        return;
    }

    TelegraphHunt(PlayerKey, PC, Tier, WorstFaction, Now);
}

void UMythicBountySubsystem::TelegraphHunt(const FString &PlayerKey, AMythicPlayerController *PC, int32 Tier,
                                           FMythicFactionId Faction, double Now) {
    const int32 PackCount =
        MythicBounty::HunterCountForTier(Tier, Config.BaseHunters, Config.HuntersPerTier, Config.MaxSimultaneousHunters);

    if (UObjectiveTracker *Tracker = PC->GetObjectiveTracker()) {
        if (UObjectiveDefinition *Obj = BuildBountyObjective(Tier, PackCount)) {
            Tracker->ServerAddObjective(Obj);
            ActiveBountyObjectives.Add(Obj);
            while (ActiveBountyObjectives.Num() > MaxRootedObjectives) {
                ActiveBountyObjectives.RemoveAt(0, 1, EAllowShrinking::No);
            }
        }
    }

    SubmitBountyChronicle(Faction, PC->GetPawn(),false);

    FPendingBountyDispatch Pending;
    Pending.Tier = Tier;
    Pending.DueTime = Now + FMath::Max(0.0f, Config.TelegraphDelaySeconds);
    Pending.Faction = Faction;
    PendingDispatches.Add(PlayerKey, Pending);

    UE_LOG(LogMythLivingWorld, Log,
           TEXT("Bounty: hunt telegraphed on '%s' (tier %d, pack %d, spawns in %.0fs)"),
           *PlayerKey, Tier, PackCount, Config.TelegraphDelaySeconds);
}


void UMythicBountySubsystem::ProcessDueDispatches(double Now) {
    if (PendingDispatches.Num() == 0) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
        if (Pacing->GetPhase() == EMythicDirectorPhase::Rest) {
            return;
        }
    }

    UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();

    TArray<FString> Completed;
    for (const TPair<FString, FPendingBountyDispatch> &Pair : PendingDispatches) {
        if (Pair.Value.DueTime > Now) {
            continue;
        }
        APawn *TargetPawn = Registry ? Registry->GetPawnForKey(Pair.Key) : nullptr;
        if (!TargetPawn) {
            Completed.Add(Pair.Key);
            UE_LOG(LogMythLivingWorld, Log, TEXT("Bounty: target '%s' gone at dispatch time — hunt dissolved"), *Pair.Key);
            continue;
        }

        const int32 Spawned = SpawnHunters(Pair.Key, TargetPawn, Pair.Value.Tier);
        LastDispatchTime.Add(Pair.Key, Now);
        SubmitBountyChronicle(Pair.Value.Faction, TargetPawn,true);
        Completed.Add(Pair.Key);

        UE_LOG(LogMythLivingWorld, Log, TEXT("Bounty: dispatched %d hunter(s) after '%s' (tier %d)"),
               Spawned, *Pair.Key, Pair.Value.Tier);
    }
    for (const FString &Key : Completed) {
        PendingDispatches.Remove(Key);
    }
}

int32 UMythicBountySubsystem::SpawnHunters(const FString &PlayerKey, APawn *TargetPawn, int32 Tier) {
    UWorld *World = GetWorld();
    if (!World || !IsValid(TargetPawn)) {
        return 0;
    }
    UGameInstance *GI = World->GetGameInstance();
    UMythicNPCManager *NPCManager = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr;
    if (!NPCManager || !Config.HunterNPCType.IsValid()) {
        return 0;
    }

    const int32 PackCount =
        MythicBounty::HunterCountForTier(Tier, Config.BaseHunters, Config.HuntersPerTier, Config.MaxSimultaneousHunters);
    const int32 ToSpawn = FMath::Min(PackCount, Config.MaxSimultaneousHunters - CountLiveHunters(PlayerKey));
    if (ToSpawn <= 0) {
        return 0;
    }

    TArray<TWeakObjectPtr<AMythicNPCCharacter>> &Pack = HuntersByPlayer.FindOrAdd(PlayerKey);
    const FVector TargetLoc = TargetPawn->GetActorLocation();
    const float MinDist = FMath::Max(500.0f, Config.MinSpawnDistance);
    const float MaxDist = FMath::Max(MinDist, Config.MaxSpawnDistance);
    int32 Spawned = 0;

    for (int32 i = 0; i < ToSpawn; ++i) {
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        const float Dist = FMath::FRandRange(MinDist, MaxDist);

        FMythicPlacementParams Params;
        Params.CellCenterXY = TargetLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
        Params.ScatterRadius = 400.0f;

        FTransform SpawnXf;
        if (!MythicPlacement::FindValidSpawn(World, Params, SpawnXf)) {
            continue;
        }

        AMythicNPCCharacter *Hunter =
            NPCManager->SpawnRandomNPC(Config.HunterNPCType, SpawnXf.GetLocation(), SpawnXf.GetRotation().Rotator());
        if (!Hunter) {
            UE_LOG(LogMythLivingWorld, Warning,
                   TEXT("Bounty: SpawnRandomNPC failed for hunter type %s — check the NPC type data table (CONTENT)"),
                   *Config.HunterNPCType.ToString());
            break;
        }

        if (UMythicCognitiveBrainComponent *Brain = Hunter->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
            Brain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK, Brain->GetHomeCell());
        }
        if (AMythicAIController *AI = Cast<AMythicAIController>(Hunter->GetController())) {
            AI->ForceEngageTarget(TargetPawn);
        }

        Pack.Add(Hunter);
        ++Spawned;
    }

    return Spawned;
}


void UMythicBountySubsystem::RefreshHunterPursuit() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();

    for (TMap<FString, TArray<TWeakObjectPtr<AMythicNPCCharacter>>>::TIterator It = HuntersByPlayer.CreateIterator(); It; ++It) {
        CountLiveHunters(It.Key());
        if (It.Value().Num() == 0) {
            It.RemoveCurrent();
            continue;
        }
        APawn *Target = Registry ? Registry->GetPawnForKey(It.Key()) : nullptr;
        if (!Target) {
            continue;
        }
        for (const TWeakObjectPtr<AMythicNPCCharacter> &HunterPtr : It.Value()) {
            if (AMythicNPCCharacter *Hunter = HunterPtr.Get()) {
                if (AMythicAIController *AI = Cast<AMythicAIController>(Hunter->GetController())) {
                    AI->ForceEngageTarget(Target);
                }
            }
        }
    }
}

int32 UMythicBountySubsystem::CountLiveHunters(const FString &PlayerKey) {
    TArray<TWeakObjectPtr<AMythicNPCCharacter>> *Pack = HuntersByPlayer.Find(PlayerKey);
    if (!Pack) {
        return 0;
    }
    Pack->RemoveAll([](const TWeakObjectPtr<AMythicNPCCharacter> &Ptr) {
        const AMythicNPCCharacter *Hunter = Ptr.Get();
        if (!Hunter || Hunter->IsHidden()) {
            return true;
        }
        if (const UAbilitySystemComponent *ASC = Hunter->GetAbilitySystemComponent()) {
            if (ASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
                return true;
            }
        }
        return false;
    });
    return Pack->Num();
}


UObjectiveDefinition *UMythicBountySubsystem::BuildBountyObjective(int32 Tier, int32 HunterCount) {
    UObjectiveDefinition *Obj = NewObject<UObjectiveDefinition>(this, NAME_None, RF_Transient);
    if (!Obj) {
        return nullptr;
    }

    Obj->TriggerEventTag = GAS_EVENT_KILL;
    Obj->RequiredCount = FMath::Max(1, HunterCount);
    Obj->bCountByEventMagnitude = false;

    FFormatNamedArguments Args;
    Args.Add(TEXT("Count"), FText::AsNumber(FMath::Max(1, HunterCount)));
    Args.Add(TEXT("Tier"), FText::AsNumber(Tier + 1));
    Obj->DisplayText = FText::Format(
        FTextFormat(NSLOCTEXT("Mythic", "BountyTelegraph",
                              "Your crimes have a price — hunters are asking about you. Defeat {Count} when they come (wanted tier {Tier}).")),
        Args);
    Obj->CompletedText = NSLOCTEXT("Mythic", "BountyCleared", "The hunters lie dead — for now, no one collects.");
    Obj->QuestName = NSLOCTEXT("Mythic", "BountyGroup", "Bounty");

    Obj->Rewards.LootReward = NewObject<ULootReward>(Obj);
    Obj->bRepeatable = false;
    return Obj;
}

void UMythicBountySubsystem::SubmitBountyChronicle(FMythicFactionId Faction, APawn *NearPawn, bool bDispatched) {
    UMythicLivingWorldSubsystem *LWS = LivingWorld;
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    UWorld *World = GetWorld();

    FMythicCellCoord Cell;
    if (IsValid(NearPawn)) {
        if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
            Cell = Grid->WorldToCell(NearPawn->GetActorLocation());
        }
    }

    FMythicWorldEvent Event;
    Event.EventTag = bDispatched ? TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED : TAG_LIVINGWORLD_EVENT_SCHEME_DISCOVERED;
    Event.PrimaryFaction = Faction;
    Event.Cell = Cell;
    Event.WorldTime = World ? World->GetTimeSeconds() : 0.0;
    Event.Significance = bDispatched ? 0.75f : 0.6f;
    Event.CategoryFlags = EMythicEventCategory::Scheme;
    LWS->SubmitWorldEvent(Event);
}
