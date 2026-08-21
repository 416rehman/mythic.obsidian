#include "MythicConversionStation.h"

#include "AbilitySystemComponent.h"
#include "ConversionStationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/IConsoleManager.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"

AMythicConversionStation::AMythicConversionStation() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    bReplicateUsingRegisteredSubObjectList = true;
    SetNetCullDistanceSquared(FMath::Square(4000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);

    ConversionComponent = CreateDefaultSubobject<UConversionStationComponent>(TEXT("ConversionComponent"));
    ConversionComponent->SetIsReplicated(true);

    StationInventory = CreateDefaultSubobject<UMythicInventoryComponent>(TEXT("StationInventory"));
    StationInventory->SetIsReplicated(true);
}

void AMythicConversionStation::SetupLocalViewModel() {
    auto World = GetWorld();
    if (World && World->GetNetMode() == NM_DedicatedServer) {
        return;
    }

    if (!IsValid(StationViewModel)) {
        StationViewModel = NewObject<UConversionStationVM>(this);
    }
}

void AMythicConversionStation::BeginPlay() {
    Super::BeginPlay();

    SetupLocalViewModel();
    if (StationViewModel) {
        StationViewModel->InitializeForStation(ConversionComponent, nullptr);
    }
}

AController *AMythicConversionStation::ResolveController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *P = Cast<APawn>(Interactor)) {
        return P->GetController();
    }
    return nullptr;
}

TArray<UMythicInventoryComponent *> AMythicConversionStation::GetAllInventoryComponents() const {
    return {StationInventory};
}

UAbilitySystemComponent *AMythicConversionStation::GetSchematicsASC() const {
    return nullptr;
}

void AMythicConversionStation::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AController *C = ResolveController(Interactor);
    AMythicPlayerController *PC = Cast<AMythicPlayerController>(C);
    if (!PC) {
        return;
    }

    if (PC->IsLocalController()) {
        PC->ServerOpenConversionStation(this);
        this->StationViewModel->RefreshForInteractor(PC);
        OnStationOpened(PC);
    }
}

void AMythicConversionStation::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicConversionStation::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicConversionStation::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;

    if (ConversionComponent && !ConversionComponent->GetStationUseRequirement().IsEmpty()) {
        FGameplayTagContainer Owned;
        if (IInventoryProviderInterface *Prov = Cast<IInventoryProviderInterface>(ResolveController(Interactor))) {
            if (UAbilitySystemComponent *ASC = Prov->GetSchematicsASC()) {
                ASC->GetOwnedGameplayTags(Owned);
            }
        }
        if (!ConversionComponent->GetStationUseRequirement().Matches(Owned)) {
            return false;
        }
    }

    return true;
}

void AMythicConversionStation::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicConversionStation::OnUnfocused_Implementation(AActor *Interactor) {
}

static void MythicOpenNearestStation(const TArray<FString> &Args, UWorld *World) {
    if (!World) {
        UE_LOG(LogTemp, Warning, TEXT("Mythic.OpenNearestStation: no world"));
        return;
    }

    APlayerController *PC = World->GetFirstPlayerController();
    APawn *Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn) {
        UE_LOG(LogTemp, Warning, TEXT("Mythic.OpenNearestStation: no local pawn - is PIE running?"));
        return;
    }

    const FVector From = Pawn->GetActorLocation();
    AMythicConversionStation *Best = nullptr;
    double BestDistSq = TNumericLimits<double>::Max();
    int32 Seen = 0;

    for (TActorIterator<AMythicConversionStation> It(World); It; ++It) {
        AMythicConversionStation *Station = *It;
        if (!IsValid(Station)) {
            continue;
        }
        ++Seen;
        const double DistSq = FVector::DistSquared(From, Station->GetActorLocation());
        if (DistSq < BestDistSq) {
            BestDistSq = DistSq;
            Best = Station;
        }
    }

    if (!Best) {
        UE_LOG(LogTemp, Warning, TEXT("Mythic.OpenNearestStation: no conversion station is loaded (world partition may not have streamed one in)"));
        return;
    }

    UE_LOG(LogTemp, Log, TEXT("Mythic.OpenNearestStation: opening %s at %.0f uu (%d station(s) loaded)"),
           *Best->GetName(), FMath::Sqrt(BestDistSq), Seen);

    IMythicInteractable::Execute_OnPrimaryInteract(Best, Pawn);
}

static FAutoConsoleCommandWithWorldAndArgs GMythicOpenNearestStationCmd(
    TEXT("Mythic.OpenNearestStation"),
    TEXT("Open the crafting station nearest the local pawn, without having to aim at it."),
    FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&MythicOpenNearestStation));
