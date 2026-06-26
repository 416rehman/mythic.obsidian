#include "MythicConversionStation.h"

#include "AbilitySystemComponent.h"
#include "ConversionStationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"

AMythicConversionStation::AMythicConversionStation() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    // Item-instance subobjects replicate through the registered-subobject list (matches PlayerController /
    // GameState / StorageContainer). Required so UMythicInventoryComponent::SetItemInSlotInternal ->
    // UMythicReplicatedObject::SetOwner(UActorComponent*) doesn't trip its IsUsingRegisteredSubObjectList ensure.
    bReplicateUsingRegisteredSubObjectList = true;
    // Far players should not receive job / fuel deltas.
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
    // Only create VMs where they can be used (not on dedicated server)
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
        // Initialize early so world-space UI (like fire particles/progress bars) works without needing to interact first
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

    // Runs on the interacting client: open the station server-side (registers the instigator) and let the
    // Blueprint push the station widget.
    if (PC->IsLocalController()) {
        PC->ServerOpenConversionStation(this);
        // Re-evaluate recipes using the specific player's inventory capabilities
        this->StationViewModel->RefreshForInteractor(PC);
        OnStationOpened(PC);
    }
}

void AMythicConversionStation::OnSecondaryInteract_Implementation(AActor *Interactor) {
    // No default secondary action; Blueprints may override.
}

USceneComponent *AMythicConversionStation::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicConversionStation::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;

    // Advisory prompt gate: hide the prompt if the player can't satisfy the station-use requirement.
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
    // Visual feedback handled in Blueprint.
}

void AMythicConversionStation::OnUnfocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}
