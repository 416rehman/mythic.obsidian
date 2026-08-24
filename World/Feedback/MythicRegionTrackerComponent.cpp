
#include "MythicRegionTrackerComponent.h"

#include "Net/UnrealNetwork.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "GameFramework/Pawn.h"
#include "GameplayEffectTypes.h"

#include "Player/MythicPlayerState.h"
#include "Player/MythicPlayerController.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "Settings/MythicDeveloperSettings.h"
#include "World/LivingWorld/LivingWorldSubsystem.h"
#include "World/LivingWorld/Territory/TerritoryGrid.h"
#include "World/LivingWorld/Settlements/MythicSettlement.h"

#define LOCTEXT_NAMESPACE "MythicRegionTracker"

UMythicRegionTrackerComponent::UMythicRegionTrackerComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicRegionTrackerComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicRegionTrackerComponent, CurrentDangerTier, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicRegionTrackerComponent, CurrentRegionName, COND_OwnerOnly);
}


bool UMythicRegionTrackerComponent::ShouldAnnounce(EMythicDangerTier NewTier, int32 NewSettlementId,
                                                   EMythicDangerTier LastTier, int32 LastSettlementId) {
    return NewTier != LastTier || NewSettlementId != LastSettlementId;
}

bool UMythicRegionTrackerComponent::IsDangerIncrease(EMythicDangerTier NewTier, EMythicDangerTier LastTier) {
    if (LastTier == EMythicDangerTier::COUNT) {
        return false;
    }
    return static_cast<uint8>(NewTier) > static_cast<uint8>(LastTier);
}

FMythicHudNotice UMythicRegionTrackerComponent::BuildRegionNotice(const FText &Region, EMythicDangerTier Tier) {
    FMythicHudNotice Notice;
    Notice.Kind = EMythicNoticeKind::Progression;
    Notice.Text = Region;
    if (Tier > EMythicDangerTier::Safe && Tier < EMythicDangerTier::COUNT) {
        Notice.Detail = FText::Format(LOCTEXT("RegionDangerDetail", "Danger: {0}"), UEnum::GetDisplayValueAsText(Tier));
    }
    // Border flapping merges into one banner instead of queueing four.
    Notice.StackKey = FName(TEXT("RegionEntry"));
    return Notice;
}

FText UMythicRegionTrackerComponent::ResolveRegionName(bool bInSettlement, const FText &SettlementName, EMythicBiome Biome) {
    if (bInSettlement && !SettlementName.IsEmpty()) {
        return SettlementName;
    }
    switch (Biome) {
    case EMythicBiome::Plains:    return LOCTEXT("BiomePlains", "Plains");
    case EMythicBiome::Forest:    return LOCTEXT("BiomeForest", "Forest");
    case EMythicBiome::Mountain:  return LOCTEXT("BiomeMountain", "Mountains");
    case EMythicBiome::Wetland:   return LOCTEXT("BiomeWetland", "Wetlands");
    case EMythicBiome::Wasteland: return LOCTEXT("BiomeWasteland", "Wasteland");
    case EMythicBiome::Desert:    return LOCTEXT("BiomeDesert", "Desert");
    default:                      return LOCTEXT("BiomeWilderness", "Wilderness");
    }
}


void UMythicRegionTrackerComponent::BeginPlay() {
    Super::BeginPlay();

    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bRegionDangerTrackerEnabled) {
        return;
    }
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().SetTimer(SampleTimerHandle, this, &UMythicRegionTrackerComponent::Sample, 1.0f,true);
    }
}

void UMythicRegionTrackerComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (UWorld *World = GetWorld()) {
        World->GetTimerManager().ClearTimer(SampleTimerHandle);
    }
    Super::EndPlay(EndPlayReason);
}

void UMythicRegionTrackerComponent::Sample() {
    AMythicPlayerState *PS = GetOwner<AMythicPlayerState>();
    if (!PS || !PS->HasAuthority()) {
        return;
    }
    const APawn *AvatarPawn = PS->GetPawn();
    if (!AvatarPawn) {
        return;
    }

    UGameInstance *GI = GetWorld() ? GetWorld()->GetGameInstance() : nullptr;
    UMythicLivingWorldSubsystem *LW = GI ? GI->GetSubsystem<UMythicLivingWorldSubsystem>() : nullptr;
    const UMythicTerritoryGrid *Grid = LW ? LW->GetTerritoryGrid() : nullptr;
    if (!Grid) {
        return;
    }

    const FVector PawnLoc = AvatarPawn->GetActorLocation();
    const FMythicCellCoord Cell = Grid->WorldToCell(PawnLoc);
    const EMythicDangerTier NewTier = Grid->GetCellDangerTier(Cell);

    FMythicSettlementData Data;
    const bool bInSettlement = LW->CopySettlementAtCell(Cell, Data);
    const int32 NewSettlementId = bInSettlement ? Data.SettlementId : INDEX_NONE;

    if (!ShouldAnnounce(NewTier, NewSettlementId, LastDangerTier, LastSettlementId)) {
        return;
    }

    const bool bDangerIncreased = IsDangerIncrease(NewTier, LastDangerTier);

    CurrentDangerTier = NewTier;
    CurrentRegionName = ResolveRegionName(bInSettlement, Data.DisplayName, Grid->GetBiomeAtWorld(PawnLoc));
    LastDangerTier = NewTier;
    LastSettlementId = NewSettlementId;

    const bool bServerSeed = !bServerSeeded;
    bServerSeeded = true;
    BroadcastIfChanged(bServerSeed);

    if (bDangerIncreased) {
        if (UMythicAbilitySystemComponent *MythicASC = PS->GetMythicAbilitySystemComponent()) {
            FGameplayCueParameters CueParams;
            CueParams.Location = PawnLoc;
            CueParams.Instigator = PS->GetPawn();
            MythicASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_World_EnteringDanger, CueParams);
        }
    }
}

void UMythicRegionTrackerComponent::OnRep_RegionDanger() {
    const bool bClientSeed = !bClientSeeded;
    bClientSeeded = true;
    BroadcastIfChanged(bClientSeed);
}

void UMythicRegionTrackerComponent::BroadcastIfChanged(bool bIsInitialSeed) {
    if (!bIsInitialSeed && LastBroadcastTier == CurrentDangerTier && LastBroadcastRegion.EqualTo(CurrentRegionName)) {
        return;
    }
    LastBroadcastTier = CurrentDangerTier;
    LastBroadcastRegion = CurrentRegionName;
    // The seed still reaches passive readouts (a widget may have bound before the first sample); it only
    // skips the banner, so spawning in a region never plays as entering it.
    OnRegionDangerChanged.Broadcast(CurrentRegionName, CurrentDangerTier);
    if (bIsInitialSeed) {
        LastBannerRegion = CurrentRegionName;
        return;
    }

    if (LastBannerRegion.EqualTo(CurrentRegionName)) {
        return;
    }
    LastBannerRegion = CurrentRegionName;
    const AMythicPlayerState *PS = GetOwner<AMythicPlayerState>();
    AMythicPlayerController *PC = PS ? Cast<AMythicPlayerController>(PS->GetPlayerController()) : nullptr;
    if (PC && PC->IsLocalController()) {
        PC->RaiseHudNotice(BuildRegionNotice(CurrentRegionName, CurrentDangerTier));
    }
}

#undef LOCTEXT_NAMESPACE
