
#include "World/Camping/MythicCampsiteSubsystem.h"

#include "World/Camping/MythicCampComponent.h"
#include "World/Camping/MythicCampfireComponent.h"
#include "World/Camping/MythicTags_Camping.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/GameDirector/MythicPacingDirectorSubsystem.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "World/EnvironmentController/EnvironmentTags.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/CausalFabric/CausalFabric.h"
#include "World/LivingWorld/MythicTags_LivingWorld.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Spawn/MythicPlacement.h"
#include "World/LivingWorld/Encounters/EncounterTemplate.h"
#include "Itemization/Loot/MythicLootManagerSubsystem.h"
#include "Itemization/Inventory/ItemDefinition.h"
#include "Player/MythicPlayerRegistrySubsystem.h"
#include "Player/MythicPlayerState.h"
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


bool UMythicCampsiteSubsystem::ShouldCreateSubsystem(UObject *Outer) const {
    const UWorld *World = Cast<UWorld>(Outer);
    if (!World || !World->IsGameWorld()) {
        return false;
    }
    return World->GetNetMode() != NM_Client;
}

void UMythicCampsiteSubsystem::OnWorldBeginPlay(UWorld &InWorld) {
    Super::OnWorldBeginPlay(InWorld);
    if (InWorld.GetNetMode() == NM_Client) {
        return;
    }
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        Config = Settings->Camping;
        bEventsEnabled = Settings->bEnableCampEvents;
    }
}

void UMythicCampsiteSubsystem::Deinitialize() {
    if (const UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(EventTimerHandle);
    }
    Pieces.Reset();
    ResolvedCamps.Reset();
    PendingEvents.Reset();
    LastEventTimeByAnchor.Reset();
    Super::Deinitialize();
}

bool UMythicCampsiteSubsystem::IsAuthority() const {
    const UWorld *World = GetWorld();
    return World && World->IsGameWorld() && World->GetNetMode() != NM_Client;
}

double UMythicCampsiteSubsystem::Now() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}


void UMythicCampsiteSubsystem::RegisterPiece(UMythicCampComponent *Piece) {
    if (!IsAuthority() || !Piece || Pieces.Contains(Piece)) {
        return;
    }
    Piece->SetRegisteredAtServerTime(Now());
    Pieces.Add(Piece);
    MarkClustersDirty();

    EnforcePieceCap(Piece->GetOwnerPlayerKey());

    UpdateEventTimer();
}

void UMythicCampsiteSubsystem::UnregisterPiece(UMythicCampComponent *Piece) {
    if (!Piece) {
        return;
    }
    if (Pieces.Remove(Piece) > 0) {
        MarkClustersDirty();
        UpdateEventTimer();
    }
}

void UMythicCampsiteSubsystem::PrunePieces() {
    const int32 Removed = Pieces.RemoveAll([](const TWeakObjectPtr<UMythicCampComponent> &P) { return !P.IsValid(); });
    if (Removed > 0) {
        MarkClustersDirty();
    }
}

int32 UMythicCampsiteSubsystem::GetLivePieceCount() const {
    int32 Count = 0;
    for (const TWeakObjectPtr<UMythicCampComponent> &P : Pieces) {
        if (P.IsValid()) {
            ++Count;
        }
    }
    return Count;
}

void UMythicCampsiteSubsystem::EnforcePieceCap(const FString &PlayerKey) {
    if (Config.MaxCampPiecesPerPlayer <= 0 || PlayerKey.IsEmpty()) {
        return;
    }
    PrunePieces();

    TArray<UMythicCampComponent *> Owned;
    for (const TWeakObjectPtr<UMythicCampComponent> &P : Pieces) {
        UMythicCampComponent *Piece = P.Get();
        if (Piece && Piece->GetOwnerPlayerKey() == PlayerKey) {
            Owned.Add(Piece);
        }
    }
    if (Owned.Num() <= Config.MaxCampPiecesPerPlayer) {
        return;
    }
    Owned.Sort([](const UMythicCampComponent &A, const UMythicCampComponent &B) {
        return A.GetRegisteredAtServerTime() < B.GetRegisteredAtServerTime();
    });
    const int32 ToCollapse = Owned.Num() - Config.MaxCampPiecesPerPlayer;
    for (int32 i = 0; i < ToCollapse; ++i) {
        CollapsePiece(Owned[i]);
    }
}

bool UMythicCampsiteSubsystem::WouldExceedPieceCap(const APlayerState *PlayerState) const {
    if (Config.MaxCampPiecesPerPlayer <= 0) {
        return false;
    }
    const AMythicPlayerState *PS = Cast<AMythicPlayerState>(PlayerState);
    const FString PlayerKey = PS ? PS->GetCanonicalPlayerKey() : FString();
    if (PlayerKey.IsEmpty()) {
        return false;
    }
    int32 Count = 0;
    for (const TWeakObjectPtr<UMythicCampComponent> &P : Pieces) {
        const UMythicCampComponent *Piece = P.Get();
        if (Piece && Piece->GetOwnerPlayerKey() == PlayerKey) {
            ++Count;
        }
    }
    return Count >= Config.MaxCampPiecesPerPlayer;
}

void UMythicCampsiteSubsystem::CollapsePiece(UMythicCampComponent *Piece) {
    AActor *Owner = Piece ? Piece->GetOwner() : nullptr;
    if (!Owner) {
        return;
    }

    if (UWorld *World = GetWorld()) {
        if (UItemDefinition *ItemDef = Piece->SourceItemDefinition.LoadSynchronous()) {
            UGameInstance *GI = World->GetGameInstance();
            if (UMythicLootManagerSubsystem *Loot = GI ? GI->GetSubsystem<UMythicLootManagerSubsystem>() : nullptr) {
                AController *Recipient = nullptr;
                if (UMythicPlayerRegistrySubsystem *Registry = World->GetSubsystem<UMythicPlayerRegistrySubsystem>()) {
                    if (const APawn *OwnerPawn = Registry->GetPawnForKey(Piece->GetOwnerPlayerKey())) {
                        Recipient = OwnerPawn->GetController();
                    }
                }
                Loot->CreateAndSpawn(ItemDef, Owner->GetActorLocation(), Recipient, 1, 1, 100.0f);
            }
        }
    }

    UE_LOG(Myth, Log, TEXT("Campsite: per-player cap reached — oldest piece %s collapsed (owner key '%s')"),
           *GetNameSafe(Owner), *Piece->GetOwnerPlayerKey());
    Owner->Destroy();
}


void UMythicCampsiteSubsystem::ResolveClustersIfDirty() {
    if (!bClustersDirty) {
        return;
    }
    bClustersDirty = false;
    ResolvedCamps.Reset();
    PrunePieces();
    bClustersDirty = false;

    const float RadiusSq = FMath::Square(Config.CampRadius);
    for (const TWeakObjectPtr<UMythicCampComponent> &AnchorPtr : Pieces) {
        UMythicCampComponent *Anchor = AnchorPtr.Get();
        if (!Anchor || !Anchor->bCampAnchor || !Anchor->GetOwner()) {
            continue;
        }
        FMythicResolvedCamp Camp;
        Camp.Anchor = Anchor;
        Camp.Center = Anchor->GetOwner()->GetActorLocation();

        TArray<FMythicComfortSource> Sources;
        for (const TWeakObjectPtr<UMythicCampComponent> &PiecePtr : Pieces) {
            UMythicCampComponent *Piece = PiecePtr.Get();
            const AActor *PieceOwner = Piece ? Piece->GetOwner() : nullptr;
            if (!PieceOwner) {
                continue;
            }
            if (FVector::DistSquared(Camp.Center, PieceOwner->GetActorLocation()) > RadiusSq) {
                continue;
            }
            Camp.Pieces.Add(Piece);

            if (const UMythicCampfireComponent *Fire = PieceOwner->FindComponentByClass<UMythicCampfireComponent>()) {
                if (!Fire->IsLit()) {
                    continue;
                }
            }
            if (Piece->ComfortCategoryTag.IsValid()) {
                Sources.Emplace(Piece->ComfortCategoryTag, Piece->ComfortPoints);
            }
        }

        Camp.ComfortTier = MythicCampsite::ComputeComfortTier(Sources, FMythicComfortScale::Camp());
        ResolvedCamps.Add(MoveTemp(Camp));
    }
}

bool UMythicCampsiteSubsystem::GetCampAt(const FVector &Location, FMythicResolvedCamp &OutCamp) {
    ResolveClustersIfDirty();
    const float RadiusSq = FMath::Square(Config.CampRadius);
    float BestDistSq = TNumericLimits<float>::Max();
    const FMythicResolvedCamp *Best = nullptr;
    for (const FMythicResolvedCamp &Camp : ResolvedCamps) {
        const float DistSq = FVector::DistSquared(Location, Camp.Center);
        if (DistSq <= RadiusSq && DistSq < BestDistSq) {
            BestDistSq = DistSq;
            Best = &Camp;
        }
    }
    if (!Best) {
        return false;
    }
    OutCamp = *Best;
    return true;
}

int32 UMythicCampsiteSubsystem::GetComfortTierAt(const FVector &Location) {
    FMythicResolvedCamp Camp;
    return GetCampAt(Location, Camp) ? Camp.ComfortTier : -1;
}

int32 UMythicCampsiteSubsystem::GetResolvedCampCount() {
    ResolveClustersIfDirty();
    return ResolvedCamps.Num();
}

void UMythicCampsiteSubsystem::CollectComfortSources(const FVector &Center, float Radius, TArray<FMythicComfortSource> &OutSources) {
    PrunePieces();
    const float RadiusSq = FMath::Square(FMath::Max(0.0f, Radius));
    for (const TWeakObjectPtr<UMythicCampComponent> &PiecePtr : Pieces) {
        const UMythicCampComponent *Piece = PiecePtr.Get();
        const AActor *PieceOwner = Piece ? Piece->GetOwner() : nullptr;
        if (!PieceOwner || FVector::DistSquared(Center, PieceOwner->GetActorLocation()) > RadiusSq) {
            continue;
        }
        if (const UMythicCampfireComponent *Fire = PieceOwner->FindComponentByClass<UMythicCampfireComponent>()) {
            if (!Fire->IsLit()) {
                continue;
            }
        }
        if (Piece->ComfortCategoryTag.IsValid()) {
            OutSources.Emplace(Piece->ComfortCategoryTag, Piece->ComfortPoints);
        }
    }
}


void UMythicCampsiteSubsystem::UpdateEventTimer() {
    UWorld *World = GetWorld();
    if (!World) {
        return;
    }
    if (!bEventsEnabled) {
        return;
    }

    bool bAnyAnchor = false;
    for (const TWeakObjectPtr<UMythicCampComponent> &P : Pieces) {
        const UMythicCampComponent *Piece = P.Get();
        if (Piece && Piece->bCampAnchor) {
            bAnyAnchor = true;
            break;
        }
    }

    FTimerManager &Timers = World->GetTimerManager();
    if (bAnyAnchor && !Timers.IsTimerActive(EventTimerHandle)) {
        const float Interval = FMath::Max(10.0f, Config.Events.CheckIntervalSeconds);
        Timers.SetTimer(EventTimerHandle, this, &UMythicCampsiteSubsystem::HandleEventCheck, Interval, true,
 Interval);
    }
    else if (!bAnyAnchor && Timers.IsTimerActive(EventTimerHandle)) {
        Timers.ClearTimer(EventTimerHandle);
        PendingEvents.Reset();
    }
}

void UMythicCampsiteSubsystem::HandleEventCheck() {
    if (!IsAuthority() || !bEventsEnabled) {
        return;
    }
    const double CurrentTime = Now();

    ProcessDueEvents(CurrentTime);

    ResolveClustersIfDirty();
    for (const FMythicResolvedCamp &Camp : ResolvedCamps) {
        EvaluateCampForEvents(Camp, CurrentTime);
    }
}

void UMythicCampsiteSubsystem::EvaluateCampForEvents(const FMythicResolvedCamp &Camp, double CurrentTime) {
    const UMythicCampComponent *Anchor = Camp.Anchor.Get();
    if (!Anchor) {
        return;
    }
    const FMythicCampEventConfig &Events = Config.Events;

    const FObjectKey AnchorKey(Anchor);
    if (const double *Last = LastEventTimeByAnchor.Find(AnchorKey)) {
        if (CurrentTime - *Last < Events.PerCampCooldownSeconds) {
            return;
        }
    }
    for (const FPendingCampEvent &Pending : PendingEvents) {
        if (Pending.Anchor == Camp.Anchor) {
            return;
        }
    }

    const bool bPlayerNear = AnyPlayerNear(Camp.Center, Events.PlayerNearRadius);
    const int32 DangerTier = DangerTierAt(Camp.Center);

    const bool bNightOk = !Events.bAmbushNightOnly || IsNight();
    if (bNightOk && Events.AmbushNPCType.IsValid()) {
        const float Chance = MythicCampsite::ComputeAmbushChance(DangerTier, Events);
        if (MythicCampsite::CanFireHostileCampEvent(IsPacingRestPhase(), bPlayerNear, true,
                                                    bEventsEnabled, FMath::FRand(), Chance)) {
            TelegraphEvent(Camp, DangerTier, true, CurrentTime);
            return;
        }
    }
    else if (bNightOk && !Events.AmbushNPCType.IsValid() && !bWarnedMissingAmbushContent &&
             MythicCampsite::ComputeAmbushChance(DangerTier, Events) > 0.0f) {
        bWarnedMissingAmbushContent = true;
        UE_LOG(Myth, Warning,
               TEXT("Campsite: bEnableCampEvents is ON but Camping.Events.AmbushNPCType is unset — no ambush will "
                    "spawn. Author an NPC type tag (CONTENT) to activate night ambushes."));
    }

    if (bPlayerNear && Events.MerchantChancePerCheck > 0.0f) {
        if (!Events.MerchantNPCType.IsValid()) {
            if (!bWarnedMissingMerchantContent) {
                bWarnedMissingMerchantContent = true;
                UE_LOG(Myth, Warning,
                       TEXT("Campsite: Camping.Events.MerchantChancePerCheck > 0 but MerchantNPCType is unset — no "
                            "merchant will visit. Author a vendor-flavored NPC type tag (CONTENT)."));
            }
        }
        else if (MythicCombat::RollSucceeds(Events.MerchantChancePerCheck, FMath::FRand())) {
            TelegraphEvent(Camp, DangerTier, false, CurrentTime);
        }
    }
}

void UMythicCampsiteSubsystem::TelegraphEvent(const FMythicResolvedCamp &Camp, int32 DangerTier, bool bHostile, double CurrentTime) {
    if (bHostile) {
        SubmitCampChronicle(Camp.Center, false);
    }

    FPendingCampEvent Pending;
    Pending.Anchor = Camp.Anchor;
    Pending.DueTime = CurrentTime + FMath::Max(0.0f, Config.Events.TelegraphDelaySeconds);
    Pending.DangerTier = DangerTier;
    Pending.bHostile = bHostile;
    PendingEvents.Add(Pending);

    UE_LOG(Myth, Log, TEXT("Campsite: %s telegraphed at camp %s (danger %d, fires in %.0fs)"),
           bHostile ? TEXT("AMBUSH") : TEXT("merchant visit"), *GetNameSafe(Camp.Anchor->GetOwner()), DangerTier,
           Config.Events.TelegraphDelaySeconds);
}

void UMythicCampsiteSubsystem::ProcessDueEvents(double CurrentTime) {
    if (PendingEvents.Num() == 0) {
        return;
    }
    ResolveClustersIfDirty();

    for (int32 i = PendingEvents.Num() - 1; i >= 0; --i) {
        const FPendingCampEvent &Pending = PendingEvents[i];
        if (Pending.DueTime > CurrentTime) {
            continue;
        }
        if (Pending.bHostile && IsPacingRestPhase()) {
            continue;
        }

        const UMythicCampComponent *Anchor = Pending.Anchor.Get();
        FMythicResolvedCamp Camp;
        const bool bCampAlive = Anchor && Anchor->GetOwner() && GetCampAt(Anchor->GetOwner()->GetActorLocation(), Camp);
        if (bCampAlive && AnyPlayerNear(Camp.Center, Config.Events.PlayerNearRadius)) {
            LastEventTimeByAnchor.Add(FObjectKey(Anchor), CurrentTime);
            if (Pending.bHostile) {
                DispatchAmbush(Camp, Pending.DangerTier);
                SubmitCampChronicle(Camp.Center, true);
            }
            else {
                DispatchMerchant(Camp);
            }
        }
        else {
            UE_LOG(Myth, Log, TEXT("Campsite: pending %s dissolved (camp gone or party left)"),
                   Pending.bHostile ? TEXT("ambush") : TEXT("merchant visit"));
        }
        PendingEvents.RemoveAt(i);
    }
}

void UMythicCampsiteSubsystem::DispatchAmbush(const FMythicResolvedCamp &Camp, int32 DangerTier) {
    UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicNPCManager *NPCManager = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr;
    if (!World || !NPCManager || !Config.Events.AmbushNPCType.IsValid()) {
        return;
    }
    APawn *TargetPawn = NearestPlayerPawn(Camp.Center, Config.Events.PlayerNearRadius);
    if (!TargetPawn) {
        return;
    }

    const int32 PackCount = MythicEncounterDefaults::DangerScaledEntityCount(
        FMath::Max(1, Config.Events.AmbushBaseCount), static_cast<EMythicDangerTier>(FMath::Clamp(DangerTier, 0, 4)),
        FMath::Max(1, Config.Events.AmbushMaxCount));

    const float MinDist = FMath::Max(500.0f, Config.Events.MinSpawnDistance);
    const float MaxDist = FMath::Max(MinDist, Config.Events.MaxSpawnDistance);
    int32 Spawned = 0;
    for (int32 i = 0; i < PackCount; ++i) {
        const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
        const float Dist = FMath::FRandRange(MinDist, MaxDist);

        FMythicPlacementParams Params;
        Params.CellCenterXY = Camp.Center + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.0f);
        Params.ScatterRadius = 400.0f;

        FTransform SpawnXf;
        if (!MythicPlacement::FindValidSpawn(World, Params, SpawnXf)) {
            continue;
        }
        AMythicNPCCharacter *Ambusher =
            NPCManager->SpawnRandomNPC(Config.Events.AmbushNPCType, SpawnXf.GetLocation(), SpawnXf.GetRotation().Rotator());
        if (!Ambusher) {
            UE_LOG(Myth, Warning, TEXT("Campsite: SpawnRandomNPC failed for ambush type %s — check the NPC type data table (CONTENT)"),
                   *Config.Events.AmbushNPCType.ToString());
            break;
        }
        if (UMythicCognitiveBrainComponent *Brain = Ambusher->FindComponentByClass<UMythicCognitiveBrainComponent>()) {
            Brain->OnSignificantEvent(TAG_LIVINGWORLD_ACTION_VIOLENCE_ATTACK, Brain->GetHomeCell());
        }
        if (AMythicAIController *AI = Cast<AMythicAIController>(Ambusher->GetController())) {
            AI->ForceEngageTarget(TargetPawn);
        }
        ++Spawned;
    }
    UE_LOG(Myth, Log, TEXT("Campsite: night ambush — %d/%d attacker(s) on the camp at %s (danger %d)"), Spawned,
           PackCount, *Camp.Center.ToCompactString(), DangerTier);
}

void UMythicCampsiteSubsystem::DispatchMerchant(const FMythicResolvedCamp &Camp) {
    UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicNPCManager *NPCManager = GI ? GI->GetSubsystem<UMythicNPCManager>() : nullptr;
    if (!World || !NPCManager || !Config.Events.MerchantNPCType.IsValid()) {
        return;
    }
    const float Angle = FMath::FRandRange(0.0f, 2.0f * PI);
    FMythicPlacementParams Params;
    Params.CellCenterXY = Camp.Center + FVector(FMath::Cos(Angle) * 700.0f, FMath::Sin(Angle) * 700.0f, 0.0f);
    Params.ScatterRadius = 250.0f;

    FTransform SpawnXf;
    if (!MythicPlacement::FindValidSpawn(World, Params, SpawnXf)) {
        return;
    }
    const FRotator FaceFire = (Camp.Center - SpawnXf.GetLocation()).Rotation();
    AMythicNPCCharacter *Merchant =
        NPCManager->SpawnRandomNPC(Config.Events.MerchantNPCType, SpawnXf.GetLocation(), FRotator(0.0f, FaceFire.Yaw, 0.0f));
    UE_LOG(Myth, Log, TEXT("Campsite: traveling merchant %s at camp %s"),
           Merchant ? TEXT("arrived") : TEXT("FAILED to spawn (check NPC type CONTENT)"), *Camp.Center.ToCompactString());
}

void UMythicCampsiteSubsystem::SubmitCampChronicle(const FVector &NearLocation, bool bDispatched) const {
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


bool UMythicCampsiteSubsystem::IsPacingRestPhase() const {
    if (const UWorld *World = GetWorld()) {
        if (const UMythicPacingDirectorSubsystem *Pacing = World->GetSubsystem<UMythicPacingDirectorSubsystem>()) {
            return Pacing->GetPhase() == EMythicDirectorPhase::Rest;
        }
    }
    return false;
}

bool UMythicCampsiteSubsystem::IsNight() const {
    const UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    if (UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr) {
        return Env->GetDayTimeTag() == Environment_Time_Night;
    }
    return false;
}

int32 UMythicCampsiteSubsystem::DangerTierAt(const FVector &Location) const {
    const UWorld *World = GetWorld();
    UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LWS = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    if (LWS && LWS->IsSystemActive()) {
        if (UMythicTerritoryGrid *Grid = LWS->GetTerritoryGrid()) {
            return static_cast<int32>(Grid->GetCellDangerTier(Grid->WorldToCell(Location)));
        }
    }
    return 0;
}

APawn *UMythicCampsiteSubsystem::NearestPlayerPawn(const FVector &Location, float MaxRadius) const {
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

bool UMythicCampsiteSubsystem::AnyPlayerNear(const FVector &Location, float Radius) const {
    return NearestPlayerPawn(Location, Radius) != nullptr;
}
