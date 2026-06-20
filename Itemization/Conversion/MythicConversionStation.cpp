#include "MythicConversionStation.h"

#include "AbilitySystemComponent.h"
#include "ConversionStationComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "Itemization/Inventory/MythicInventoryComponent.h"
#include "Player/MythicPlayerController.h"

AMythicConversionStation::AMythicConversionStation() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
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
