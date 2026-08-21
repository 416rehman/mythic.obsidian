
#include "World/LivingWorld/EmergentQuests/MythicApexHuntSubsystem.h"

#include "Objectives/ObjectiveDefinition.h"
#include "Objectives/ObjectiveTracker.h"
#include "Rewards/LootReward.h"
#include "Rewards/ItemReward.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Knowledge/MythicCodexComponent.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"
#include "World/LivingWorld/Pressure/MythicTags_Pressure.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "AI/NPCs/MythicNPCManager.h"
#include "AI/NPCs/MythicNPCCharacter.h"
#include "World/Hunting/MythicSpoorTrail.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "GameFramework/Pawn.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Mythic.h"


bool UMythicApexHuntSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicApexHuntSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        Config = Settings->ApexHunts;
        SpoorConfig = Settings->Spoor;
        bEnabled = Settings->bEnableApexHunts;
    }
    if (bEnabled && Config.Species.Num() > 0) {
        const float Interval = FMath::Max(10.0f, Config.CheckIntervalSeconds);
        InWorld.GetTimerManager().SetTimer(CheckTimerHandle, this, &UMythicApexHuntSubsystem::HandleCheck, Interval,
 true, Interval);
        UE_LOG(Myth, Log, TEXT("ApexHunt: live (%d species row(s), check every %.0fs)"), Config.Species.Num(), Interval);
    }
}

void UMythicApexHuntSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(CheckTimerHandle);
    }
    ActiveOffers.Reset();
    ActiveObjectiveRoots.Reset();
    Super::Deinitialize();
}

bool UMythicApexHuntSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

double UMythicApexHuntSubsystem::Now() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}

bool UMythicApexHuntSubsystem::IsRainingNow() const {
    const UWorld *World = GetWorld();
    const UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr;
    if (!Env) {
        return false;
    }
    const FGameplayTag Weather = Env->GetWeather();
    return Weather.IsValid() && Weather.ToString().Contains(TEXT("Rain"));
}


void UMythicApexHuntSubsystem::HandleCheck() {
    if (!IsAuthority() || !bEnabled) {
        return;
    }
    const double CurrentTime = Now();

    for (int32 i = ActiveOffers.Num() - 1; i >= 0; --i) {
        if (CurrentTime >= ActiveOffers[i].ExpireTimeSeconds || (!ActiveOffers[i].Apex.IsValid())) {
            RetireOffer(i, false);
        }
    }

    for (int32 SpeciesIdx = 0; SpeciesIdx < Config.Species.Num(); ++SpeciesIdx) {
        bool bActive = false;
        for (const FMythicActiveApexOffer &Offer : ActiveOffers) {
            if (Offer.SpeciesIndex == SpeciesIdx) {
                bActive = true;
                break;
            }
        }
        if (!bActive) {
            TryOfferSpecies(SpeciesIdx);
        }
    }
}

bool UMythicApexHuntSubsystem::TryOfferSpecies(int32 SpeciesIndex) {
    UWorld *World = GetWorld();
    if (!World || !Config.Species.IsValidIndex(SpeciesIndex)) {
        return false;
    }
    const FMythicApexHuntSpecies &Species = Config.Species[SpeciesIndex];
    if (!Species.BestiaryKey.IsValid() || !Species.ApexNPCType.IsValid()) {
        if (!bWarnedMissingContent) {
            bWarnedMissingContent = true;
            UE_LOG(Myth, Warning, TEXT("ApexHunt: a species row lacks BestiaryKey/ApexNPCType (CONTENT) — it will never offer."));
        }
        return false;
    }

    const double CurrentTime = Now();
    const double *LastEnd = LastOfferEndTimeBySpecies.Find(SpeciesIndex);
    const double SecondsSinceLast = LastEnd ? (CurrentTime - *LastEnd) : -1.0;
    const int32 FullThreshold = Config.KillThresholdFullOverride > 0 ? Config.KillThresholdFullOverride : 10;
    const FGameplayTag PressureChannel = Species.HuntPressureChannel.IsValid() ? Species.HuntPressureChannel : TAG_Pressure_Hunt;
    UMythicRegionalPressureSubsystem *Pressure = World->GetSubsystem<UMythicRegionalPressureSubsystem>();

    TArray<AMythicPlayerController *> Recipients;
    APawn *AnchorPawn = nullptr;
    for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(It->Get());
        if (!PC || !PC->HasAuthority()) {
            continue;
        }
        APawn *Pawn = PC->GetPawn();
        const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>();
        const UMythicCodexComponent *Codex = PS ? PS->GetCodexComponent() : nullptr;
        if (!Pawn || !Codex) {
            continue;
        }
        const bool bKnowledgeFull = Codex->HasFullBestiaryTier(Species.BestiaryKey, FullThreshold);
        const float HuntPressure = Pressure ? Pressure->QueryPressure(Pawn->GetActorLocation(), PressureChannel) : 0.0f;
        const bool bOffer = FMythicApexHuntRules::ShouldOfferApexHunt(bEnabled, bKnowledgeFull, HuntPressure,
                                                                      Config.PopulationPressureThreshold,
 false,
                                                                      SecondsSinceLast, Config.OfferCooldownSeconds);
        if (bOffer) {
            Recipients.Add(PC);
            if (!AnchorPawn) {
                AnchorPawn = Pawn;
            }
        }
    }
    if (Recipients.Num() == 0 || !AnchorPawn) {
        return false;
    }

    UGameInstance *GI = World->GetGameInstance();
    UMythicNPCManager *NPCManager = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr;
    if (!NPCManager) {
        return false;
    }
    const float MinDist = FMath::Max(1000.0f, Config.SpawnMinDistance);
    const float MaxDist = FMath::Max(MinDist, Config.SpawnMaxDistance);
    AMythicNPCCharacter *Apex = nullptr;
    FVector HuntSite = FVector::ZeroVector;
    for (int32 Attempt = 0; Attempt < 4 && !Apex; ++Attempt) {
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        const float Dist = FMath::FRandRange(MinDist, MaxDist);
        FMythicPlacementParams Params;
        Params.CellCenterXY = AnchorPawn->GetActorLocation() + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
        Params.ScatterRadius = 600.0f;
        FTransform SpawnXf;
        if (!MythicPlacement::FindValidSpawn(World, Params, SpawnXf)) {
            continue;
        }
        Apex = NPCManager->SpawnRandomNPC(Species.ApexNPCType, SpawnXf.GetLocation(), SpawnXf.GetRotation().Rotator());
        HuntSite = SpawnXf.GetLocation();
    }
    if (!Apex) {
        UE_LOG(Myth, Warning, TEXT("ApexHunt: SpawnRandomNPC failed for %s — check the NPC type data table (CONTENT)"),
               *Species.ApexNPCType.ToString());
        return false;
    }

    if (UMythicLifeComponent *Life = Apex->FindComponentByClass<UMythicLifeComponent>()) {
        Life->OnDeath.AddDynamic(this, &UMythicApexHuntSubsystem::HandleApexDeath);
    }

    UObjectiveDefinition *Obj = BuildApexObjective(Species, HuntSite);
    if (!Obj) {
        return false;
    }
    for (AMythicPlayerController *PC : Recipients) {
        if (UObjectiveTracker *Tracker = PC->GetObjectiveTracker()) {
            Tracker->ServerAddObjective(Obj);
        }
    }

    if (Config.TrailNodeCount > 0) {
        SpawnTrailStart(AnchorPawn->GetActorLocation(), HuntSite);
    }

    FMythicActiveApexOffer Offer;
    Offer.SpeciesIndex = SpeciesIndex;
    Offer.Objective = Obj;
    Offer.Apex = Apex;
    Offer.ExpireTimeSeconds = Now() + FMath::Max(60.0f, Config.OfferLifetimeSeconds);
    ActiveObjectiveRoots.Add(Obj);
    ActiveOffers.Add(MoveTemp(Offer));

    SubmitApexChronicle(HuntSite, false);
    UE_LOG(Myth, Log, TEXT("ApexHunt: offered '%s' to %d player(s), apex %s at %s"),
           *Obj->DisplayText.ToString(), Recipients.Num(), *GetNameSafe(Apex), *HuntSite.ToCompactString());
    return true;
}

UObjectiveDefinition *UMythicApexHuntSubsystem::BuildApexObjective(const FMythicApexHuntSpecies &Species, const FVector &HuntSite) {
    UObjectiveDefinition *Obj = NewObject<UObjectiveDefinition>(this, NAME_None, RF_Transient);
    if (!Obj) {
        return nullptr;
    }
    Obj->TriggerEventTag = GAS_EVENT_KILL;
    Obj->RequiredCount = 1;
    Obj->RequiredPayloadTag = Species.BestiaryKey;

    const FText SpeciesName = Species.SpeciesName.IsEmpty() ? FText::FromString(TEXT("beast")) : Species.SpeciesName;
    Obj->DisplayText = FText::Format(NSLOCTEXT("Mythic", "ApexHuntHeadline", "Apex hunt: track and slay the great {0}"), SpeciesName);
    Obj->CompletedText = FText::Format(NSLOCTEXT("Mythic", "ApexHuntDone", "The great {0} has fallen"), SpeciesName);
    Obj->QuestName = NSLOCTEXT("Mythic", "ApexHuntGroup", "Apex Hunt");

    Obj->bShowOnMap = true;
    Obj->WorldMarkerLocation = HuntSite;

    Obj->Rewards.LootReward = NewObject<ULootReward>(Obj);
    if (!Species.TrophyItem.IsNull()) {
        if (UItemDefinition *TrophyDef = Species.TrophyItem.LoadSynchronous()) {
            UItemReward *Trophy = NewObject<UItemReward>(Obj);
            Trophy->Item = TrophyDef;
            Trophy->Quantity = 1;
            Trophy->bCelebrate = true;
            Obj->Rewards.ItemReward = Trophy;
        }
    }
    Obj->bRepeatable = false;
    return Obj;
}

void UMythicApexHuntSubsystem::SpawnTrailStart(const FVector &HunterLocation, const FVector &HuntSite) {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (AMythicSpoorTrail::CountNodesNear(World, HunterLocation, 15000.0f) >= FMath::Max(1, SpoorConfig.MaxNodesPerRegion)) {
        return;
    }
    const FVector FirstLoc = FMythicSpoorRules::NextStepLocation(HunterLocation, HuntSite, FMath::Max(600.0f, SpoorConfig.StepDistanceCm * 0.5f),
                                                                 SpoorConfig.StepJitterDegrees, FMath::FRand());
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    if (AMythicSpoorTrail *Node = World->SpawnActor<AMythicSpoorTrail>(AMythicSpoorTrail::StaticClass(), FTransform(FirstLoc), Params)) {
        Node->ServerInitTrailNode(HuntSite, FMath::Max(0, Config.TrailNodeCount - 1), SpoorConfig, IsRainingNow());
    }
}

void UMythicApexHuntSubsystem::HandleApexDeath(AActor *DeadActor) {
    for (int32 i = ActiveOffers.Num() - 1; i >= 0; --i) {
        if (ActiveOffers[i].Apex.Get() == DeadActor) {
            SubmitApexChronicle(DeadActor ? DeadActor->GetActorLocation() : FVector::ZeroVector, true);
            UE_LOG(Myth, Log, TEXT("ApexHunt: the apex %s has fallen — offer retired (rewards ride the killer's tracker)"),
                   *GetNameSafe(DeadActor));
            RetireOffer(i, true);
        }
    }
}

void UMythicApexHuntSubsystem::RetireOffer(int32 OfferIndex, bool) {
    if (!ActiveOffers.IsValidIndex(OfferIndex)) {
        return;
    }
    LastOfferEndTimeBySpecies.Add(ActiveOffers[OfferIndex].SpeciesIndex, Now());
    ActiveObjectiveRoots.Remove(ActiveOffers[OfferIndex].Objective);
    ActiveOffers.RemoveAt(OfferIndex);
}

void UMythicApexHuntSubsystem::SubmitApexChronicle(const FVector &NearLocation, bool bCompleted) const {
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
    Event.EventTag = bCompleted ? TAG_LIVINGWORLD_EVENT_SCHEME_COMPLETED : TAG_LIVINGWORLD_EVENT_SCHEME_DISCOVERED;
    Event.Cell = Cell;
    Event.WorldTime = World ? World->GetTimeSeconds() : 0.0;
    Event.Significance = bCompleted ? 0.65f : 0.55f;
    Event.CategoryFlags = EMythicEventCategory::Scheme;
    LWS->SubmitWorldEvent(Event);
}
