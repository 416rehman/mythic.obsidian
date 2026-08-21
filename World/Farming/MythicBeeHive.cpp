
#include "MythicBeeHive.h"

#include "MythicApiaryRules.h"
#include "MythicCropDefinition.h"
#include "MythicFarmPlot.h"
#include "MythicFarmingRewardUtil.h"
#include "MythicTags_Farming.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Mythic.h"
#include "Net/UnrealNetwork.h"
#include "Player/MythicPlayerController.h"
#include "Player/MythicPlayerState.h"
#include "Player/Proficiency/ProficiencyComponent.h"
#include "Player/Proficiency/ProficiencyDefinition.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "TimerManager.h"
#include "World/Camping/MythicInfluenceSourceComponent.h"
#include "World/EnvironmentController/EnvironmentTags.h"
#include "World/EnvironmentController/MythicEnvironmentSubsystem.h"
#include "Progression/MythicStatLedgerComponent.h"

namespace {
constexpr uint8 HiveSaveVersion = 1;
}

AMythicBeeHive::AMythicBeeHive() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    NetDormancy = DORM_DormantAll;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);

    PollinationAura = CreateDefaultSubobject<UMythicPollinationAuraComponent>(TEXT("PollinationAura"));
    PollinationAura->SetupAttachment(SceneRoot);
}

void AMythicBeeHive::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AMythicBeeHive, StoredUnits);
}

void AMythicBeeHive::BeginPlay() {
    Super::BeginPlay();
    LastSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    if (HasAuthority()) {
        RearmProductionTimer();
    }
    OnHiveVisualChanged(StoredUnits, MaxStoredUnits);
}


float AMythicBeeHive::ProductionMultiplierNow() const {
    if (!bPauseProductionInSnow) {
        return 1.0f;
    }
    const UWorld *World = GetWorld();
    const UGameInstance *GI = World ? World->GetGameInstance() : nullptr;
    if (UMythicEnvironmentSubsystem *Env = GI ? GI->GetSubsystem<UMythicEnvironmentSubsystem>() : nullptr) {
        if (Env->GetWeather() == Environment_Weather_Snow) {
            return 0.0f;
        }
    }
    return 1.0f;
}

void AMythicBeeHive::SampleProduction(double Now) {
    if (!HasAuthority()) {
        return;
    }
    const float Window = static_cast<float>(FMath::Max(0.0, Now - LastSampleTime));
    LastSampleTime = Now;
    if (Window <= 0.0f) {
        return;
    }
    const FMythicProductionAccrual Accrual = FMythicApiaryRules::AccrueUnits(
        CarryoverSeconds, Window, ProductionMultiplierNow(), SecondsPerHoneyUnit, StoredUnits, MaxStoredUnits);
    CarryoverSeconds = Accrual.CarryoverSeconds;
    SetStoredUnits(Accrual.StoredUnits);
}

void AMythicBeeHive::RearmProductionTimer() {
    UWorld *World = GetWorld();
    if (!World || !HasAuthority()) {
        return;
    }
    FTimerManager &Timers = World->GetTimerManager();
    const float Next = FMythicApiaryRules::SecondsToNextUnit(CarryoverSeconds, SecondsPerHoneyUnit, StoredUnits, MaxStoredUnits);
    if (Next <= 0.0f) {
        Timers.ClearTimer(ProductionTimerHandle);
        return;
    }
    TWeakObjectPtr<AMythicBeeHive> WeakThis(this);
    Timers.SetTimer(ProductionTimerHandle, FTimerDelegate::CreateLambda([WeakThis]() {
                        if (AMythicBeeHive *Hive = WeakThis.Get()) {
                            Hive->SampleProduction(Hive->GetWorld()->GetTimeSeconds());
                            Hive->RearmProductionTimer();
                        }
                    }),
                    FMath::Max(1.0f, Next), false);
}

void AMythicBeeHive::SetStoredUnits(int32 NewUnits) {
    const int32 Clamped = FMath::Clamp(NewUnits, 0, FMath::Max(0, MaxStoredUnits));
    if (Clamped == StoredUnits) {
        return;
    }
    StoredUnits = Clamped;
    if (HasAuthority()) {
        FlushNetDormancy();
    }
    OnHiveVisualChanged(StoredUnits, MaxStoredUnits);
}


int32 AMythicBeeHive::CountDistinctCropsInRadius() const {
    const UWorld *World = GetWorld();
    if (!World || !PollinationAura) {
        return 0;
    }
    const float RadiusSq = FMath::Square(PollinationAura->InfluenceRadius);
    const FVector HiveLoc = GetActorLocation();
    TArray<FGameplayTag> CropTypes;
    for (TActorIterator<AMythicFarmPlot> It(const_cast<UWorld *>(World)); It; ++It) {
        const AMythicFarmPlot *Plot = *It;
        if (!IsValid(Plot) || Plot->IsEmpty() || Plot->IsWithered()) {
            continue;
        }
        if (FVector::DistSquared(HiveLoc, Plot->GetActorLocation()) > RadiusSq) {
            continue;
        }
        CropTypes.Add(Plot->GetPlantedCropTypeTag());
    }
    return FMythicApiaryRules::CountDistinctCropTypes(CropTypes);
}

void AMythicBeeHive::ServerHandleCollect(AActor *Interactor) {
    if (!HasAuthority() || !GetWorld()) {
        return;
    }
    SampleProduction(GetWorld()->GetTimeSeconds());
    if (StoredUnits <= 0) {
        return;
    }
    APlayerController *PC = Cast<APlayerController>(AMythicFarmPlot::ResolveController(Interactor));
    if (!PC) {
        return;
    }

    const int32 DistinctCrops = CountDistinctCropsInRadius();
    TArray<int32, TInlineAllocator<8>> RowMins;
    for (const FMythicHoneyVarietyRow &Row : VarietyRows) {
        RowMins.Add(Row.MinDistinctCropTypes);
    }
    const int32 RowIndex = FMythicApiaryRules::ResolveHoneyVariety(DistinctCrops, RowMins);
    if (RowIndex == INDEX_NONE) {
        if (!bWarnedMissingVarietyContent) {
            bWarnedMissingVarietyContent = true;
            UE_LOG(Myth, Warning, TEXT("AMythicBeeHive: no VarietyRows authored — collection yields nothing. Author at "
                                       "least a MinDistinctCropTypes=0 row (CONTENT) to activate honey."));
        }
        return;
    }

    const int32 Units = StoredUnits;
    const FVector HiveLoc = GetActorLocation();
    const FMythicHoneyVarietyRow &Row = VarietyRows[RowIndex];
    for (int32 i = 0; i < Units; ++i) {
        Row.Rewards.Give(PC, true, 0, HiveLoc);
    }
    if (MythicFarmingRewards::HasAny(WaxRewards)) {
        WaxRewards.Give(PC, true, 0, HiveLoc);
    }

    const int32 Level = AMythicFarmPlot::ResolveProficiencyLevel(Interactor, BeekeepingProficiency);
    float Xp = BeekeepingXPPerUnit * Units;
    if (XpNoGainAtOrAboveLevel > 0 && Level >= XpNoGainAtOrAboveLevel) {
        Xp = 0.0f;
    }
    if (Xp > 0.0f && BeekeepingProficiency) {
        if (AMythicPlayerController *MythicPC = Cast<AMythicPlayerController>(AMythicFarmPlot::ResolveController(Interactor))) {
            if (UProficiencyComponent *ProfComp = const_cast<UProficiencyComponent *>(MythicPC->GetProficiencyComponent())) {
                ProfComp->GrantProficiencyXP(BeekeepingProficiency, Xp);
            }
        }
    }
    if (const AMythicPlayerState *PS = PC->GetPlayerState<AMythicPlayerState>()) {
        if (UMythicStatLedgerComponent *Ledger = PS->GetStatLedgerComponent()) {
            Ledger->RecordStat(TAG_Stat_Beekeeping_HoneyCollected, Units);
        }
    }

    UE_LOG(Myth, Log, TEXT("AMythicBeeHive: collected %d honey (variety row %d, %d distinct crops)"), Units, RowIndex, DistinctCrops);
    CarryoverSeconds = 0.0f;
    SetStoredUnits(0);
    RearmProductionTimer();
}


void AMythicBeeHive::OnPrimaryInteract_Implementation(AActor *Interactor) {
    if (HasAuthority()) {
        ServerHandleCollect(Interactor);
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor)) {
        if (PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
    }
}

void AMythicBeeHive::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicBeeHive::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicBeeHive::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = CollectInteractionName;
    return true;
}

void AMythicBeeHive::OnFocused_Implementation(AActor *Interactor) {}
void AMythicBeeHive::OnUnfocused_Implementation(AActor *Interactor) {}

void AMythicBeeHive::OnRep_StoredUnits() {
    OnHiveVisualChanged(StoredUnits, MaxStoredUnits);
}


void AMythicBeeHive::SerializeCustomData(TArray<uint8> &OutCustomData) {
    SampleProduction(GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0);

    FMemoryWriter Writer(OutCustomData);
    uint8 Version = HiveSaveVersion;
    int32 Units = StoredUnits;
    float Carry = CarryoverSeconds;
    Writer << Version;
    Writer << Units;
    Writer << Carry;
}

void AMythicBeeHive::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    if (InCustomData.Num() == 0) {
        OnHiveVisualChanged(StoredUnits, MaxStoredUnits);
        return;
    }
    FMemoryReader Reader(InCustomData);
    uint8 Version = 0;
    int32 Units = 0;
    float Carry = 0.0f;
    Reader << Version;
    Reader << Units;
    Reader << Carry;
    if (Reader.IsError() || Version < 1) {
        UE_LOG(MythSaveLoad, Warning, TEXT("AMythicBeeHive: unreadable saved payload (%d bytes); restoring empty"), InCustomData.Num());
        Units = 0;
        Carry = 0.0f;
    }

    CarryoverSeconds = FMath::Max(0.0f, Carry);
    LastSampleTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0;
    SetStoredUnits(Units);
    if (HasAuthority()) {
        FlushNetDormancy();
        RearmProductionTimer();
    }
    OnHiveVisualChanged(StoredUnits, MaxStoredUnits);
}
