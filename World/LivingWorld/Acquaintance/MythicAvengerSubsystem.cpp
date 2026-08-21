
#include "World/LivingWorld/Acquaintance/MythicAvengerSubsystem.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "World/Death/MythicCemeterySubsystem.h"
#include "World/Death/MythicCemeteryRules.h"
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
#include "Engine/GameInstance.h"
#include "TimerManager.h"


bool UMythicAvengerSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    if (World->GetNetMode() == NM_Client) {
        return false;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    return Settings && Settings->bEnableAvengers;
}

void UMythicAvengerSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }

    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        Config = Settings->Avengers;
    }
    if (UGameInstance *GI = InWorld.GetGameInstance()) {
        LivingWorld = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }

    const float Interval = FMath::Max(5.0f, Config.CheckIntervalSeconds);
    InWorld.GetTimerManager().SetTimer(CheckTimerHandle, this, &UMythicAvengerSubsystem::HandleAvengerCheck, Interval,
true,Interval);

    UE_LOG(LogMythLivingWorld, Log,
           TEXT("AvengerSubsystem: live (check=%.0fs, base chance=%.2f, cooldown=%.0fs, telegraph=%.0fs, cap=%d)"),
           Interval, Config.BaseChance, Config.CooldownSeconds, Config.TelegraphDelaySeconds, Config.MaxSimultaneousAvengers);
}

void UMythicAvengerSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(CheckTimerHandle);
    }
    PendingDispatches.Reset();
    LastDispatchTime.Reset();
    AvengersByPlayer.Reset();
    ActiveAvengerObjectives.Reset();
    LivingWorld = nullptr;
    Super::Deinitialize();
}

bool UMythicAvengerSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

int32 UMythicAvengerSubsystem::GetTrackedAvengerCount() const {
    int32 Count = 0;
    for (const TPair<FString, TArray<TWeakObjectPtr<AMythicNPCCharacter>>> &Pair : AvengersByPlayer) {
        for (const TWeakObjectPtr<AMythicNPCCharacter> &Avenger : Pair.Value) {
            if (Avenger.IsValid()) {
                ++Count;
            }
        }
    }
    return Count;
}


void UMythicAvengerSubsystem::NotifyNpcKilledByPlayer(uint32 VictimNameHash, const FText &VictimName,
                                                      FGameplayTag VictimRole, float Significance,
                                                      FMythicFactionId VictimFaction, AMythicPlayerController *KillerPC) {
    UWorld *World = GetWorld();
    if (!World || !IsAuthority() || !KillerPC || VictimNameHash == 0) {
        return;
    }
    const AMythicPlayerState *KillerPS = KillerPC->GetPlayerState<AMythicPlayerState>();
    if (!KillerPS) {
        return;
    }
    const FString PlayerKey = KillerPS->GetCanonicalPlayerKey();
    if (PlayerKey.IsEmpty() || PendingDispatches.Contains(PlayerKey)) {
        return;
    }

    const UMythicCemeterySubsystem *Cemetery = World->GetSubsystem<UMythicCemeterySubsystem>();
    static const FMythicCemeteryConfig DefaultCemeteryConfig;
    const FMythicCemeteryConfig &CemeteryConfig = Cemetery ? Cemetery->GetConfig() : DefaultCemeteryConfig;
    const bool bNotable = FMythicCemeteryRules::IsNotableDeath(VictimRole, Significance, CemeteryConfig);

    if (bNotable && !Config.AvengerNPCType.IsValid()) {
        if (!bWarnedMissingContent) {
            bWarnedMissingContent = true;
            UE_LOG(LogMythLivingWorld, Warning,
                   TEXT("Avenger: bEnableAvengers is ON but Avengers.AvengerNPCType is unset — no vengeance will "
                        "dispatch. Author an avenger NPC type tag (CONTENT) to activate the system."));
        }
        return;
    }

    float Notoriety = 0.0f;
    if (const UMythicFactionStandingComponent *Standing = KillerPS->GetFactionStanding()) {
        for (const FMythicFactionStandingEntry &Entry : Standing->GetStandings()) {
            if (Entry.Faction.IsValid() && VictimFaction.IsValid() && Entry.Faction.Index == VictimFaction.Index) {
                Notoriety = FMath::Max(0.0f, -Entry.Value);
                break;
            }
        }
    }

    const double Now = World->GetTimeSeconds();
    const double *Last = LastDispatchTime.Find(PlayerKey);
    const double TimeSinceLast = Last ? (Now - *Last) : TNumericLimits<double>::Max();
    const int32 LiveAvengers = CountLiveAvengers(PlayerKey);

    if (!MythicMourning::ShouldSpawnAvenger(bNotable, Notoriety, TimeSinceLast, LiveAvengers, FMath::FRand(), Config)) {
        return;
    }

    if (UObjectiveTracker *Tracker = KillerPC->GetObjectiveTracker()) {
        if (UObjectiveDefinition *Obj = BuildAvengerObjective(VictimName)) {
            Tracker->ServerAddObjective(Obj);
            ActiveAvengerObjectives.Add(Obj);
            while (ActiveAvengerObjectives.Num() > MaxRootedObjectives) {
                ActiveAvengerObjectives.RemoveAt(0, 1, EAllowShrinking::No);
            }
        }
    }

    SubmitAvengerChronicle(VictimFaction, KillerPC->GetPawn(),false);

    FPendingAvengerDispatch Pending;
    Pending.DueTime = Now + FMath::Max(0.0f, Config.TelegraphDelaySeconds);
    Pending.VictimName = VictimName;
    Pending.Faction = VictimFaction;
    PendingDispatches.Add(PlayerKey, Pending);

    UE_LOG(LogMythLivingWorld, Log, TEXT("Avenger: vengeance for '%s' telegraphed against '%s' (spawns in %.0fs)"),
           *VictimName.ToString(), *PlayerKey, Config.TelegraphDelaySeconds);
}


void UMythicAvengerSubsystem::HandleAvengerCheck() {
    if (!IsAuthority()) {
        return;
    }
    const UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    RefreshAvengerPursuit();
    ProcessDueDispatches(World->GetTimeSeconds());
}

void UMythicAvengerSubsystem::ProcessDueDispatches(double Now) {
    if (PendingDispatches.Num() == 0) {
        return;
    }
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();

    TArray<FString> Completed;
    for (const TPair<FString, FPendingAvengerDispatch> &Pair : PendingDispatches) {
        if (Pair.Value.DueTime > Now) {
            continue;
        }
        APawn *TargetPawn = Registry ? Registry->GetPawnForKey(Pair.Key) : nullptr;
        if (!TargetPawn) {
            Completed.Add(Pair.Key);
            UE_LOG(LogMythLivingWorld, Log, TEXT("Avenger: target '%s' gone at dispatch time — vengeance dissolved"), *Pair.Key);
            continue;
        }

        const bool bSpawned = SpawnAvenger(Pair.Key, TargetPawn);
        LastDispatchTime.Add(Pair.Key, Now);
        SubmitAvengerChronicle(Pair.Value.Faction, TargetPawn,true);
        Completed.Add(Pair.Key);

        UE_LOG(LogMythLivingWorld, Log, TEXT("Avenger: %s after '%s' (for the death of '%s')"),
               bSpawned ? TEXT("an avenger now hunts") : TEXT("no valid ground for an avenger"),
               *Pair.Key, *Pair.Value.VictimName.ToString());
    }
    for (const FString &Key : Completed) {
        PendingDispatches.Remove(Key);
    }
}

bool UMythicAvengerSubsystem::SpawnAvenger(const FString &PlayerKey, APawn *TargetPawn) {
    UWorld *World = GetWorld();
    if (!World || !IsValid(TargetPawn)) {
        return false;
    }
    UGameInstance *GI = World->GetGameInstance();
    UMythicNPCManager *NPCManager = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr;
    if (!NPCManager || !Config.AvengerNPCType.IsValid()) {
        return false;
    }
    if (CountLiveAvengers(PlayerKey) >= FMath::Max(1, Config.MaxSimultaneousAvengers)) {
        return false;
    }

    const FVector TargetLoc = TargetPawn->GetActorLocation();
    const float MinDist = FMath::Max(500.0f, Config.MinSpawnDistance);
    const float MaxDist = FMath::Max(MinDist, Config.MaxSpawnDistance);
    const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
    const float Dist = FMath::FRandRange(MinDist, MaxDist);

    FMythicPlacementParams Params;
    Params.CellCenterXY = TargetLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
    Params.ScatterRadius = 400.0f;

    FTransform SpawnXf;
    if (!MythicPlacement::FindValidSpawn(World, Params, SpawnXf)) {
        return false;
    }

    AMythicNPCCharacter *Avenger =
        NPCManager->SpawnRandomNPC(Config.AvengerNPCType, SpawnXf.GetLocation(), SpawnXf.GetRotation().Rotator());
    if (!Avenger) {
        UE_LOG(LogMythLivingWorld, Warning,
               TEXT("Avenger: SpawnRandomNPC failed for type %s — check the NPC type data table (CONTENT)"),
               *Config.AvengerNPCType.ToString());
        return false;
    }

    if (UMythicCognitiveBrainComponent *Brain = Avenger->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
        Brain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK, Brain->GetHomeCell());
    }
    if (AMythicAIController *AI = Cast<AMythicAIController>(Avenger->GetController())) {
        AI->ForceEngageTarget(TargetPawn);
    }

    AvengersByPlayer.FindOrAdd(PlayerKey).Add(Avenger);
    return true;
}

void UMythicAvengerSubsystem::RefreshAvengerPursuit() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();

    for (TMap<FString, TArray<TWeakObjectPtr<AMythicNPCCharacter>>>::TIterator It = AvengersByPlayer.CreateIterator(); It; ++It) {
        CountLiveAvengers(It.Key());
        if (It.Value().Num() == 0) {
            It.RemoveCurrent();
            continue;
        }
        APawn *Target = Registry ? Registry->GetPawnForKey(It.Key()) : nullptr;
        if (!Target) {
            continue;
        }
        for (const TWeakObjectPtr<AMythicNPCCharacter> &AvengerPtr : It.Value()) {
            if (AMythicNPCCharacter *Avenger = AvengerPtr.Get()) {
                if (AMythicAIController *AI = Cast<AMythicAIController>(Avenger->GetController())) {
                    AI->ForceEngageTarget(Target);
                }
            }
        }
    }
}

int32 UMythicAvengerSubsystem::CountLiveAvengers(const FString &PlayerKey) {
    TArray<TWeakObjectPtr<AMythicNPCCharacter>> *Pack = AvengersByPlayer.Find(PlayerKey);
    if (!Pack) {
        return 0;
    }
    Pack->RemoveAll([](const TWeakObjectPtr<AMythicNPCCharacter> &Ptr) {
        const AMythicNPCCharacter *Avenger = Ptr.Get();
        if (!Avenger || Avenger->IsHidden()) {
            return true;
        }
        if (const UAbilitySystemComponent *ASC = Avenger->GetAbilitySystemComponent()) {
            if (ASC->HasMatchingGameplayTag(GAS_STATE_DEAD)) {
                return true;
            }
        }
        return false;
    });
    return Pack->Num();
}


UObjectiveDefinition *UMythicAvengerSubsystem::BuildAvengerObjective(const FText &VictimName) {
    UObjectiveDefinition *Obj = NewObject<UObjectiveDefinition>(this, NAME_None, RF_Transient);
    if (!Obj) {
        return nullptr;
    }

    Obj->TriggerEventTag = GAS_EVENT_KILL;
    Obj->RequiredCount = 1;
    Obj->bCountByEventMagnitude = false;

    FFormatNamedArguments Args;
    Args.Add(TEXT("Victim"), VictimName);
    Obj->DisplayText = FText::Format(
        FTextFormat(NSLOCTEXT("Mythic", "AvengerTelegraph",
                              "Kin of {Victim} swear vengeance — an avenger seeks you. Face them when they come.")),
        Args);
    Obj->CompletedText = NSLOCTEXT("Mythic", "AvengerAnswered", "The vengeance is answered; the blood-debt is settled.");
    Obj->QuestName = NSLOCTEXT("Mythic", "AvengerGroup", "Vengeance");

    Obj->Rewards.LootReward = NewObject<ULootReward>(Obj);
    Obj->bRepeatable = false;
    return Obj;
}

void UMythicAvengerSubsystem::SubmitAvengerChronicle(FMythicFactionId Faction, APawn *NearPawn, bool bDispatched) {
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
    Event.Significance = bDispatched ? 0.7f : 0.55f;
    Event.CategoryFlags = EMythicEventCategory::Scheme;
    LWS->SubmitWorldEvent(Event);
}
