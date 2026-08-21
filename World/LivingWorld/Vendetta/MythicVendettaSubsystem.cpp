
#include "World/LivingWorld/Vendetta/MythicVendettaSubsystem.h"

#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/Factions/FactionDatabase.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Rewards/LootReward.h"
#include "GAS/MythicTags_GAS.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/MythicFactionStandingComponent.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "Settings/MythicDeveloperSettings.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "AI/NPCs/MythicAIController.h"
#include "AI/Cognition/CognitiveBrainComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Mythic.h"


bool UMythicVendettaSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicVendettaSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }

    if (UGameInstance *GI = InWorld.GetGameInstance()) {
        LivingWorld = GI->GetSubsystem<UMythicLivingWorldSubsystem>();
    }

    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
    }

    if (TickIntervalSeconds > 0.0f) {
        InWorld.GetTimerManager().SetTimer(TickTimerHandle, this, &UMythicVendettaSubsystem::HandleVendettaTick,
                                           TickIntervalSeconds,true,TickIntervalSeconds);
    }

    UE_LOG(Myth, Log, TEXT("Vendetta: subsystem live (tick=%.1fs, bounty@%.0f assassin@%.0f raid@%.0f, cooldown=%.0fs)"),
           TickIntervalSeconds, Thresholds.BountyAt, Thresholds.AssassinAt, Thresholds.RaidAt, Thresholds.CooldownSeconds);
}

void UMythicVendettaSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(TickTimerHandle);
    }
    Ledger.Reset();
    StandingSnapshot.Reset();
    ActiveVendettaObjectives.Reset();
    LivingWorld = nullptr;
    Super::Deinitialize();
}

bool UMythicVendettaSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

float UMythicVendettaSubsystem::GetMaxThreatForPlayer(const FString &PlayerKey) const {
    if (PlayerKey.IsEmpty()) {
        return 0.0f;
    }
    float MaxThreat = 0.0f;
    for (const TPair<FMythicThreatKey, FMythicVendettaLedgerEntry> &Pair : Ledger.GetEntries()) {
        if (Pair.Key.PlayerKey == PlayerKey) {
            MaxThreat = FMath::Max(MaxThreat, Pair.Value.Threat);
        }
    }
    return MaxThreat;
}


void UMythicVendettaSubsystem::HandleVendettaTick() {
    if (!IsAuthority()) {
        return;
    }
    Ledger.TickDecay(TickIntervalSeconds, ThreatDecayRatePerSec);
    ObserveStandingSignals();
    EvaluateAndExecuteVendettas();
}


void UMythicVendettaSubsystem::ObserveStandingSignals() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
        if (!PC || !PC->HasAuthority()) {
            continue;
        }
        const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
        if (!PS) {
            continue;
        }
        const FString PlayerKey = PS->GetCanonicalPlayerKey();
        if (PlayerKey.IsEmpty()) {
            continue;
        }
        const UMythicFactionStandingComponent *Standing = PS->GetFactionStanding();
        if (!Standing) {
            continue;
        }

        for (const FMythicFactionStandingEntry &Entry : Standing->GetStandings()) {
            if (!Entry.Faction.IsValid()) {
                continue;
            }
            const FMythicThreatKey Key(Entry.Faction, PlayerKey);
            const float Prev = StandingSnapshot.FindRef(Key);
            const float Delta = Entry.Value - Prev;
            if (Delta < -KINDA_SMALL_NUMBER) {
                Ledger.AddThreat(Entry.Faction, PlayerKey, -Delta, ThreatWeightPerStandingLost);
            }
            StandingSnapshot.Add(Key, Entry.Value);
        }
    }
}


void UMythicVendettaSubsystem::EvaluateAndExecuteVendettas() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    UMythicFactionDatabase *FactionDB = LWS ? LWS->GetFactionDatabase() : nullptr;

    const double Now = World->GetTimeSeconds();

    struct FPendingVendetta {
        FString PlayerKey;
        FMythicFactionId Faction;
        EMythicVendettaType Type = EMythicVendettaType::None;
        FText FactionName;
    };
    TArray<FPendingVendetta> Pending;

    for (const TPair<FMythicThreatKey, FMythicVendettaLedgerEntry> &Pair : Ledger.GetEntries()) {
        const FMythicVendettaLedgerEntry &Entry = Pair.Value;
        if (Entry.Threat < Thresholds.BountyAt) {
            continue;
        }

        FMythicFactionId Faction;
        Faction.Index = Pair.Key.FactionIndex;

        float MilitaryStrength = 0.0f;
        FText FactionName = FText::GetEmpty();
        if (FactionDB && Faction.IsValid()) {
            FMythicFactionData Data;
            if (FactionDB->GetFaction(Faction, Data)) {
                MilitaryStrength = Data.MilitaryStrength;
                FactionName = Data.DisplayName;
            }
        }

        const float SecondsSince = static_cast<float>(Now - Entry.LastVendettaTime);
        const EMythicVendettaType Type =
            MythicVendetta::SelectVendetta(Entry.Threat, MilitaryStrength, SecondsSince, Thresholds);
        if (Type == EMythicVendettaType::None) {
            continue;
        }

        Pending.Add({Pair.Key.PlayerKey, Faction, Type, FactionName});
    }

    for (const FPendingVendetta &V : Pending) {
        ExecuteVendetta(V.PlayerKey, V.Faction, V.Type, V.FactionName);
        Ledger.StampVendetta(V.Faction, V.PlayerKey, Now, V.Type);
    }
}

void UMythicVendettaSubsystem::ExecuteVendetta(const FString &PlayerKey, FMythicFactionId Faction,
                                               EMythicVendettaType Type, const FText &FactionName) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }

    UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>();
    AMythicPlayerController *TargetPC = Registry ? Registry->GetPlayerControllerForKey(PlayerKey) : nullptr;

    FMythicCellCoord Cell;
    if (TargetPC) {
        if (const APawn *Pawn = TargetPC->GetPawn()) {
            if (UMythicLivingWorldSubsystem *LWS = LivingWorld.Get()) {
                if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
                    Cell = Grid->WorldToCell(Pawn->GetActorLocation());
                }
            }
        }
    }

    if (TargetPC) {
        if (UObjectiveTracker *Tracker = TargetPC->GetObjectiveTracker()) {
            if (UObjectiveDefinition *Obj = BuildVendettaObjective(Type, FactionName)) {
                Tracker->ServerAddObjective(Obj);

                ActiveVendettaObjectives.Add(Obj);
                while (ActiveVendettaObjectives.Num() > MaxRootedObjectives) {
                    ActiveVendettaObjectives.RemoveAt(0, 1, EAllowShrinking::No);
                }
            }
        }
    }

    SubmitVendettaChronicle(Faction, Type, Cell);


    const TCHAR *TypeName = Type == EMythicVendettaType::RetaliationRaid  ? TEXT("RetaliationRaid")
                            : Type == EMythicVendettaType::AssassinDispatch ? TEXT("AssassinDispatch")
                                                                            : TEXT("BountyPosting");
    UE_LOG(Myth, Log, TEXT("Vendetta: faction %d mounts %s against player '%s'%s"), static_cast<int32>(Faction.Index),
           TypeName, *PlayerKey, TargetPC ? TEXT("") : TEXT(" (target offline — chronicle only)"));
}


UObjectiveDefinition *UMythicVendettaSubsystem::BuildVendettaObjective(EMythicVendettaType Type, const FText &FactionName) {
    UObjectiveDefinition *Obj = NewObject<UObjectiveDefinition>(this, NAME_None, RF_Transient);
    if (!Obj) {
        return nullptr;
    }

    int32 Count = 1;
    FText Headline;
    switch (Type) {
    case EMythicVendettaType::RetaliationRaid:
        Count = 6;
        Headline = FText::FromString(TEXT("{Faction} mounts a retaliation raid against you — repel {Count} attackers."));
        break;
    case EMythicVendettaType::AssassinDispatch:
        Count = 1;
        Headline = FText::FromString(TEXT("{Faction} has dispatched an assassin to kill you — strike them down first."));
        break;
    case EMythicVendettaType::BountyPosting:
    default:
        Count = 3;
        Headline = FText::FromString(TEXT("Wanted by {Faction}: a bounty marks you — defeat {Count} of their hunters to clear it."));
        break;
    }

    Obj->TriggerEventTag = GAS_EVENT_KILL;
    Obj->RequiredCount = Count;
    Obj->bCountByEventMagnitude = false;

    FFormatNamedArguments Args;
    Args.Add(TEXT("Faction"), FactionName.IsEmpty() ? FText::FromString(TEXT("a vengeful faction")) : FactionName);
    Args.Add(TEXT("Count"), FText::AsNumber(Count));
    Obj->DisplayText = FText::Format(FTextFormat(Headline), Args);
    Obj->CompletedText = FText::Format(NSLOCTEXT("Mythic", "VendettaCleared", "{0} — the vendetta is settled."), FactionName.IsEmpty() ? FText::FromString(TEXT("Their grudge")) : FactionName);
    Obj->QuestName = NSLOCTEXT("Mythic", "VendettaGroup", "Vendetta");

    Obj->Rewards.LootReward = NewObject<ULootReward>(Obj);
    Obj->bRepeatable = false;
    return Obj;
}

bool UMythicVendettaSubsystem::IsPacingRestPhase() const {
    if (const UWorld *World = GetWorld()) {
        if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
            return Pacing->GetPhase() == EMythicDirectorPhase::Rest;
        }
    }
    return false;
}


void UMythicVendettaSubsystem::SubmitVendettaChronicle(FMythicFactionId Faction, EMythicVendettaType Type,
                                                       FMythicCellCoord Cell) {
    UMythicLivingWorldSubsystem *LWS = LivingWorld.Get();
    if (!LWS || !LWS->IsSystemActive()) {
        return;
    }
    UWorld *World = GetWorld();

    FMythicWorldEvent Event;
    Event.EventTag = TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED;
    Event.PrimaryFaction = Faction;
    Event.Cell = Cell;
    Event.WorldTime = World ? World->GetTimeSeconds() : 0.0;
    Event.Significance = Type == EMythicVendettaType::RetaliationRaid  ? 0.9f
                         : Type == EMythicVendettaType::AssassinDispatch ? 0.8f
                                                                         : 0.7f;
    Event.CategoryFlags = EMythicEventCategory::Scheme;
    LWS->SubmitWorldEvent(Event);
}
