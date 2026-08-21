
#include "World/Secrets/MythicSecretInteractable.h"

#include "World/Secrets/MythicSecretReveal.h"
#include "Settings/MythicDeveloperSettings.h"
#include "Components/StaticMeshComponent.h"
#include "World/Placement/MythicProxyRegistrationComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Player/MythicPlayerController.h"

AMythicSecretInteractable::AMythicSecretInteractable() {
    PrimaryActorTick.bCanEverTick = false;
    bReplicates = true;
    NetDormancy = DORM_DormantAll;
    SetNetCullDistanceSquared(FMath::Square(6000.f));

    SceneRoot = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
    SetRootComponent(SceneRoot);

    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    Mesh->SetupAttachment(SceneRoot);

    ProxyRegistration = CreateDefaultSubobject<UMythicProxyRegistrationComponent>(TEXT("ProxyRegistration"));
    ProxyRegistration->ProxyType = FGameplayTag::RequestGameplayTag(FName("Interactable.Secret"), false);
    ProxyRegistration->bEnabled = true;
}

int32 AMythicSecretInteractable::GetProxyStateFlags_Implementation() const {
    return bGlobalConsumed ? 1 : 0;
}

void AMythicSecretInteractable::ApplyProxyStateFlags_Implementation(int32 StateFlags) {
    bGlobalConsumed = (StateFlags & 1) != 0;
}

AController *AMythicSecretInteractable::ResolveInteractorController(AActor *Interactor) {
    if (AController *C = Cast<AController>(Interactor)) {
        return C;
    }
    if (const APawn *Pawn = Cast<APawn>(Interactor)) {
        return Pawn->GetController();
    }
    return nullptr;
}

void AMythicSecretInteractable::OnPrimaryInteract_Implementation(AActor *Interactor) {
    if (HasAuthority()) {
        if (!GetDefault<UMythicDeveloperSettings>()->bSecretsEnabled) {
            return;
        }
        if (bGlobalOneShot && bGlobalConsumed) {
            return;
        }
        APlayerController *PC = Cast<APlayerController>(Interactor);
        if (!PC) {
            return;
        }
        const TWeakObjectPtr<AController> WeakController(PC);
        if (RevealedControllers.Contains(WeakController)) {
            return;
        }
        const bool bRevealed = FMythicSecretReveal::TryRevealSecret(PC, Def, GetActorLocation());
        if (bRevealed) {
            RevealedControllers.Add(WeakController);
            if (bGlobalOneShot) {
                bGlobalConsumed = true;
            }
        }
        return;
    }
    if (AMythicPlayerController *PC = Cast<AMythicPlayerController>(Interactor)) {
        if (PC->IsLocalController()) {
            PC->ServerInteractPrimary(this);
        }
    }
}

void AMythicSecretInteractable::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicSecretInteractable::GetWidgetAttachmentComponent_Implementation() const {
    return SceneRoot;
}

bool AMythicSecretInteractable::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    if (bGlobalOneShot && bGlobalConsumed) {
        return false;
    }
    if (const AController *Controller = ResolveInteractorController(Interactor)) {
        if (RevealedControllers.Contains(TWeakObjectPtr<AController>(const_cast<AController *>(Controller)))) {
            return false;
        }
    }
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = PrimaryInteractionName;
    return true;
}

void AMythicSecretInteractable::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicSecretInteractable::OnUnfocused_Implementation(AActor *Interactor) {
}
