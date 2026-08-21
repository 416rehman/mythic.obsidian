
#include "World/Fishing/MythicFishingSpot.h"

#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Pawn.h"
#include "Engine/World.h"

#include "World/Fishing/MythicTags_Fishing.h"
#include "World/Fishing/MythicFishStockRules.h"
#include "World/LivingWorld/Pressure/MythicRegionalPressureSubsystem.h"
#include "World/LivingWorld/Pressure/MythicTags_Pressure.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Player/MythicPlayerController.h"
#include "Mythic.h"

AMythicFishingSpot::AMythicFishingSpot() {
    PrimaryActorTick.bCanEverTick = false;

    bReplicates = false;

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);
}

void AMythicFishingSpot::BeginPlay() {
    Super::BeginPlay();
}

void AMythicFishingSpot::OnPrimaryInteract_Implementation(AActor *Interactor) {
    if (!Interactor) {
        return;
    }
    if (GetNetMode() == NM_Client) {
        AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor);
        if (!PC) {
            if (const APawn *InteractorPawn = Cast<APawn>(Interactor)) {
                PC = Cast<AMythicPlayerController>(InteractorPawn->GetController());
            }
        }
        if (PC && PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
        return;
    }
}

USceneComponent *AMythicFishingSpot::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicFishingSpot::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}


int32 AMythicFishingSpot::EffectiveMaxStock() const {
    if (MaxStockOverride > 0) {
        return MaxStockOverride;
    }
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    return Settings ? FMath::Max(1, Settings->FishStocks.DefaultMaxStock) : 10;
}

double AMythicFishingSpot::NowSeconds() const {
    const UWorld *World = GetWorld();
    return World ? World->GetTimeSeconds() : 0.0;
}

int32 AMythicFishingSpot::ResolveStockNow() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!bStockInitialized) {
        StockUnits = EffectiveMaxStock();
        RegenAnchorTime = NowSeconds();
        bStockInitialized = true;
    }
    const float RegenPerUnit = Settings ? Settings->FishStocks.RegenSecondsPerUnit : 300.0f;
    return FMythicFishStockRules::Resolve(StockUnits, RegenAnchorTime, NowSeconds(), RegenPerUnit, EffectiveMaxStock());
}

int32 AMythicFishingSpot::GetCurrentStock() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bFishStocksEnabled) {
        return MAX_int32;
    }
    return ResolveStockNow();
}

bool AMythicFishingSpot::IsCurrentlyExhausted() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!Settings || !Settings->bFishStocksEnabled) {
        return false;
    }
    return ResolveStockNow() <= 0;
}

void AMythicFishingSpot::ServerNotifyCatch() {
    const UMythicDeveloperSettings *Settings = GetDefault<UMythicDeveloperSettings>();
    if (!HasAuthority() || GetNetMode() == NM_Client || !Settings || !Settings->bFishStocksEnabled) {
        return;
    }
    ResolveStockNow();

    bool bExhaustedNow = false;
    const bool bConsumed = FMythicFishStockRules::ConsumeOne(StockUnits, RegenAnchorTime, NowSeconds(),
                                                             Settings->FishStocks.RegenSecondsPerUnit, EffectiveMaxStock(),
                                                             bExhaustedNow);
    if (UMythicRegionalPressureSubsystem *Pressure = GetWorld() ? GetWorld()->GetSubsystem<UMythicRegionalPressureSubsystem>() : nullptr) {
        if (bConsumed && Settings->FishStocks.PressurePerCatch > 0.0f) {
            Pressure->AddPressure(GetActorLocation(), TAG_Pressure_Fish, Settings->FishStocks.PressurePerCatch);
        }
        if (bExhaustedNow && Settings->FishStocks.PressureOnExhausted > 0.0f) {
            Pressure->AddPressure(GetActorLocation(), TAG_Pressure_Fish, Settings->FishStocks.PressureOnExhausted);
        }
    }
    if (bExhaustedNow) {
        UE_LOG(Myth, Log, TEXT("FishingSpot %s: EXHAUSTED (catch table degrades to trash until the stock regenerates)"),
               *GetNameSafe(this));
    }
}


void AMythicFishingSpot::SerializeCustomData(TArray<uint8> &OutCustomData) {
    const int32 Units = ResolveStockNow();
    const float TowardNext = FMythicFishStockRules::SecondsTowardNextUnit(Units, RegenAnchorTime, NowSeconds(), EffectiveMaxStock());
    FMythicFishStockRules::EncodeStockSave(OutCustomData, Units, TowardNext);
}

void AMythicFishingSpot::DeserializeCustomData(const TArray<uint8> &InCustomData) {
    int32 Units = 0;
    float TowardNext = 0.0f;
    if (FMythicFishStockRules::DecodeStockSave(InCustomData, Units, TowardNext)) {
        StockUnits = FMath::Clamp(Units, 0, EffectiveMaxStock());
        RegenAnchorTime = NowSeconds() - TowardNext;
        bStockInitialized = true;
    }
    else {
        bStockInitialized = false;
    }
}
