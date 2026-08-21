
#include "GAS/Mounts/MythicMountRosterComponent.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Engine/World.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/Pawn.h"
#include "NavigationSystem.h"
#include "Net/UnrealNetwork.h"

#include "GAS/Mounts/MythicMount.h"
#include "GAS/Mounts/MythicTags_Mounts.h"
#include "GAS/MythicAbilitySystemComponent.h"
#include "GAS/Feedback/MythicTags_FeedbackCues.h"
#include "GAS/MythicTags_GAS.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "Player/MythicPlayerState.h"
#include "Progression/MythicStatLedgerComponent.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Mythic.h"

UMythicMountRosterComponent::UMythicMountRosterComponent() {
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UMythicMountRosterComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(UMythicMountRosterComponent, Roster, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(UMythicMountRosterComponent, ActiveMountId, COND_OwnerOnly);
}

void UMythicMountRosterComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
    if (GetOwner() && GetOwner()->HasAuthority()) {
        DespawnSummonedMount();
    }
    Super::EndPlay(EndPlayReason);
}

double UMythicMountRosterComponent::ServerNow() const {
    if (const UWorld *W = GetWorld()) {
        if (const AGameStateBase *GS = W->GetGameState()) {
            return GS->GetServerWorldTimeSeconds();
        }
    }
    return 0.0;
}

const FMythicMountRecord *UMythicMountRosterComponent::GetActiveRecord() const {
    if (!ActiveMountId.IsValid()) {
        return nullptr;
    }
    return Roster.FindByPredicate([this](const FMythicMountRecord &R) { return R.MountId == ActiveMountId; });
}

void UMythicMountRosterComponent::SyncActiveFlags() {
    for (FMythicMountRecord &R : Roster) {
        R.bActive = R.MountId == ActiveMountId;
    }
}

FGuid UMythicMountRosterComponent::ServerAddMount(uint8 SpeciesId, FName CustomName) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return FGuid();
    }
    if (MaxRosterSize > 0 && Roster.Num() >= MaxRosterSize) {
        UE_LOG(Myth, Log, TEXT("MountRoster: %s roster full (%d) — tame refused"), *GetNameSafe(GetOwner()), Roster.Num());
        return FGuid();
    }

    FMythicMountRecord &NewRecord = Roster.AddDefaulted_GetRef();
    NewRecord.MountId = FGuid::NewGuid();
    NewRecord.CustomName = !CustomName.IsNone() ? CustomName : FName(*FString::Printf(TEXT("Mount %d"), Roster.Num()));
    NewRecord.SpeciesId = SpeciesId;
    NewRecord.BondXP = 0;

    if (!ActiveMountId.IsValid()) {
        ActiveMountId = NewRecord.MountId;
    }
    SyncActiveFlags();

    if (const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner())) {
        if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
            Ledger->RecordStat(TAG_Stat_Mount_Tamed);
        }
    }

    UE_LOG(Myth, Log, TEXT("MountRoster: %s tamed species %d → %s (%d total)"),
           *GetNameSafe(GetOwner()), SpeciesId, *NewRecord.MountId.ToString(EGuidFormats::Short), Roster.Num());
    OnRosterChanged.Broadcast();
    return NewRecord.MountId;
}

void UMythicMountRosterComponent::ServerGrantBondXP(const FGuid &MountId, int32 XP) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || XP <= 0) {
        return;
    }
    FMythicMountRecord *Record = Roster.FindByPredicate([&MountId](const FMythicMountRecord &R) { return R.MountId == MountId; });
    if (!Record) {
        return;
    }
    Record->BondXP += XP;

    if (AMythicMount *Live = SummonedMount.Get()) {
        if (Live->OwnerMountId == MountId) {
            Live->BondLevel = MythicMountStatics::BondLevelFromXP(Record->BondXP);
        }
    }
    OnRosterChanged.Broadcast();
}

void UMythicMountRosterComponent::RestoreRoster(const TArray<FMythicMountRecord> &InRoster, const FGuid &InActiveId) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    if (InRoster.Num() == 0) {
        return;
    }
    Roster = InRoster;
    ActiveMountId = InActiveId;
    if (!GetActiveRecord()) {
        const FMythicMountRecord *Flagged = Roster.FindByPredicate([](const FMythicMountRecord &R) { return R.bActive; });
        ActiveMountId = Flagged ? Flagged->MountId : Roster[0].MountId;
    }
    SyncActiveFlags();
    UE_LOG(Myth, Log, TEXT("MountRoster: restored %d mounts for %s"), Roster.Num(), *GetNameSafe(GetOwner()));
    OnRosterChanged.Broadcast();
}

void UMythicMountRosterComponent::ServerSetActiveMount_Implementation(FGuid MountId) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    const FMythicMountRecord *Record = Roster.FindByPredicate([&MountId](const FMythicMountRecord &R) { return R.MountId == MountId; });
    if (!Record || ActiveMountId == MountId) {
        return;
    }
    ActiveMountId = MountId;
    SyncActiveFlags();
    OnRosterChanged.Broadcast();
}

void UMythicMountRosterComponent::ServerRenameMount_Implementation(FGuid MountId, FName NewName) {
    if (!GetOwner() || !GetOwner()->HasAuthority() || NewName.IsNone()) {
        return;
    }
    FMythicMountRecord *Record = Roster.FindByPredicate([&MountId](const FMythicMountRecord &R) { return R.MountId == MountId; });
    if (!Record) {
        return;
    }
    Record->CustomName = NewName;
    OnRosterChanged.Broadcast();
}

void UMythicMountRosterComponent::ServerSummonMount_Implementation() {
    const AMythicPlayerState *PS = Cast<AMythicPlayerState>(GetOwner());
    APawn *OwnerPawn = PS ? PS->GetPawn() : nullptr;
    if (!OwnerPawn) {
        return;
    }
    SummonMountNear(OwnerPawn->GetActorLocation());
}

void UMythicMountRosterComponent::ServerStashMount_Implementation() {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    DespawnSummonedMount();
    OnRosterChanged.Broadcast();
}

void UMythicMountRosterComponent::SummonMountNear(const FVector &NearLocation) {
    AActor *OwnerActor = GetOwner();
    if (!OwnerActor || !OwnerActor->HasAuthority()) {
        return;
    }
    AMythicPlayerState *PS = Cast<AMythicPlayerState>(OwnerActor);
    APawn *OwnerPawn = PS ? PS->GetPawn() : nullptr;
    UWorld *World = GetWorld();
    if (!PS || !OwnerPawn || !World) {
        return;
    }

    const FMythicMountRecord *Record = GetActiveRecord();
    bool bInCombat = false;
    if (const IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(PS)) {
        if (const UAbilitySystemComponent *ASC = ASI->GetAbilitySystemComponent()) {
            bInCombat = ASC->HasMatchingGameplayTag(GAS_STATE_INCOMBAT);
        }
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    const double Cooldown = Settings ? Settings->MountSummonCooldown : 0.0;
    if (!MythicMountStatics::CanSummon(Record != nullptr, bInCombat, ServerNow() - LastSummonServerTime, Cooldown)) {
        UE_LOG(Myth, Log, TEXT("MountRoster: summon refused for %s (record=%d combat=%d cd=%.1fs since last)"),
               *GetNameSafe(OwnerActor), Record != nullptr, bInCombat, ServerNow() - LastSummonServerTime);
        return;
    }

    UClass *MountClass = ResolveMountClass();
    if (!MountClass) {
        UE_LOG(Myth, Warning, TEXT("MountRoster: no mount class configured (component override or MythicDeveloperSettings::DefaultMountClass) — summon skipped"));
        return;
    }

    DespawnSummonedMount();

    FVector SpawnLocation = NearLocation + OwnerPawn->GetActorRightVector() * 250.0f;
    if (UNavigationSystemV1 *Nav = UNavigationSystemV1::GetCurrent(World)) {
        FNavLocation Projected;
        if (Nav->ProjectPointToNavigation(SpawnLocation, Projected, FVector(300.0f, 300.0f, 500.0f))) {
            SpawnLocation = Projected.Location;
        }
    }
    if (const AMythicMount *CDO = MountClass->GetDefaultObject<AMythicMount>()) {
        SpawnLocation.Z += CDO->GetDefaultHalfHeight() + 2.0f;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
    Params.Instigator = OwnerPawn;
    AMythicMount *Mount = World->SpawnActor<AMythicMount>(MountClass, SpawnLocation, FRotator(0.0f, OwnerPawn->GetActorRotation().Yaw, 0.0f), Params);
    if (!Mount) {
        return;
    }
    Mount->ConfigureFromRecord(*Record, PS->GetCanonicalPlayerKey(), PS);

    SummonedMount = Mount;
    LastSummonServerTime = ServerNow();

    if (IAbilitySystemInterface *ASI = Cast<IAbilitySystemInterface>(PS)) {
        if (UMythicAbilitySystemComponent *SummonerASC = Cast<UMythicAbilitySystemComponent>(ASI->GetAbilitySystemComponent())) {
            FGameplayCueParameters CueParams;
            CueParams.Location = SpawnLocation;
            CueParams.Instigator = OwnerPawn;
            SummonerASC->ExecuteGameplayCueMulticast(TAG_GameplayCue_Mount_Summon, CueParams);
        }
    }

    if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
        Ledger->RecordStat(TAG_Stat_Mount_Summoned);
    }

    if (UMythicLifeComponent *OwnerLife = UMythicLifeComponent::FindHealthComponent(OwnerPawn)) {
        OwnerLife->OnDeath.AddUniqueDynamic(this, &UMythicMountRosterComponent::HandleOwnerPawnDeath);
    }

    UE_LOG(Myth, Log, TEXT("MountRoster: summoned %s for %s"), *GetNameSafe(Mount), *GetNameSafe(OwnerActor));
    OnRosterChanged.Broadcast();
}

void UMythicMountRosterComponent::DespawnSummonedMount() {
    AMythicMount *Mount = SummonedMount.Get();
    SummonedMount = nullptr;
    if (!Mount) {
        return;
    }
    if (Mount->IsRidden()) {
        Mount->ServerDismount();
    }
    Mount->Destroy();
}

void UMythicMountRosterComponent::HandleOwnerPawnDeath(AActor *) {
    if (!GetOwner() || !GetOwner()->HasAuthority()) {
        return;
    }
    DespawnSummonedMount();
}

UClass *UMythicMountRosterComponent::ResolveMountClass() const {
    if (MountClassOverride) {
        return MountClassOverride;
    }
    if (const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>()) {
        if (!Settings->DefaultMountClass.IsNull()) {
            return Settings->DefaultMountClass.LoadSynchronous();
        }
    }
    return nullptr;
}

void UMythicMountRosterComponent::OnRep_Roster() {
    OnRosterChanged.Broadcast();
}
