// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicCharacter_Player.h"

#include "MythicPlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Materials/Material.h"
#include "Engine/World.h"
#include "Net/UnrealNetwork.h"
#include "GAS/AttributeSets/Shared/MythicLifeComponent.h"
#include "MythicPlayerController.h" // revive interaction routes via ServerInteractPrimary

AMythicCharacter_Player::AMythicCharacter_Player() {
    // Set size for player capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    LifeComponent = CreateDefaultSubobject<UMythicLifeComponent>(TEXT("LifeComponent"));

    // Don't rotate character to camera direction
    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    // Configure character movement
    GetCharacterMovement()->bOrientRotationToMovement = true; // Rotate character to moving direction
    GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;

    // // Create a camera boom...
    // CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
    // CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
    // CameraBoom->TargetArmLength = 800.f;
    // CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
    // CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level
    // CameraBoom->SetupAttachment(GetMesh(), SprintArmAttachToSocket);
    //
    // // Create a camera...
    // TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
    // TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
    // TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm
}


void AMythicCharacter_Player::InitializeASC() {
    // Cast the controller so we get the ability system component
    auto OwningController = GetController();
    auto ASCInterface = Cast<IAbilitySystemInterface>(OwningController);
    if (ASCInterface) {
        this->ASC_Ref = ASCInterface->GetAbilitySystemComponent();
        if (ASC_Ref && ASC_Ref->HasBeenInitialized()) {
            // Owner = PlayerState (the ASC's real owning subobject), NOT the PlayerController — controllers don't
            // replicate to non-owning clients, so using the controller here made the ASC's OwnerActor machine-dependent
            // (PC on server/owner, PlayerState on proxies) and stamped player-originated effects with the PC as
            // Instigator. Match the PlayerState branch below (canonical owner=PlayerState, avatar=Pawn).
            ASC_Ref->InitAbilityActorInfo(GetPlayerState<AMythicPlayerState>(), this);
            if (LifeComponent && !LifeComponent->IsInitialized()) {
                LifeComponent->InitializeWithAbilitySystem(ASC_Ref);
            }
        }
        return;
    }

    // Otherwise, cast the player state
    ASCInterface = Cast<IAbilitySystemInterface>(GetPlayerState());
    if (ASCInterface) {
        this->ASC_Ref = ASCInterface->GetAbilitySystemComponent();
        if (ASC_Ref && ASC_Ref->HasBeenInitialized()) {
            ASC_Ref->InitAbilityActorInfo(GetPlayerState<AMythicPlayerState>(), this);
            if (LifeComponent && !LifeComponent->IsInitialized()) {
                LifeComponent->InitializeWithAbilitySystem(ASC_Ref);
            }
        }
    }
}

UAbilitySystemComponent *AMythicCharacter_Player::GetAbilitySystemComponent() const {
    return this->ASC_Ref;
}

void AMythicCharacter_Player::GetLifetimeReplicatedProps(TArray<FLifetimeProperty> &OutLifetimeProps) const {
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);

    DOREPLIFETIME(AMythicCharacter_Player, ASC_Ref);
}

// ─── IMythicInteractable: revive a downed teammate ───
// A player pawn is interactable ONLY while downed. The prompt widget calls OnPrimaryInteract on the REVIVER's client
// (Interactor = the reviver's controller); we route to the server via ServerInteractPrimary, then on the server
// validate (target still downed, reviver alive) and revive.
void AMythicCharacter_Player::OnPrimaryInteract_Implementation(AActor *Interactor) {
    AMythicPlayerController *ReviverPC = Cast<AMythicPlayerController>(Interactor);
    if (!ReviverPC) {
        return;
    }
    if (HasAuthority()) {
        if (!LifeComponent) {
            return;
        }
        bool bReviverDowned = false;
        if (const APawn *ReviverPawn = ReviverPC->GetPawn()) {
            if (const UMythicLifeComponent *ReviverLife = UMythicLifeComponent::FindHealthComponent(ReviverPawn)) {
                bReviverDowned = ReviverLife->IsDowned();
            }
        }
        if (UMythicLifeComponent::CanReviveTarget(LifeComponent->IsDowned(), bReviverDowned)) {
            LifeComponent->ServerReviveFromDowned();
        }
        return;
    }
    if (ReviverPC->IsLocalController()) {
        ReviverPC->ServerInteractPrimary(this);
    }
}

void AMythicCharacter_Player::OnSecondaryInteract_Implementation(AActor *Interactor) {
    // No secondary action.
}

USceneComponent *AMythicCharacter_Player::GetWidgetAttachmentComponent_Implementation() const {
    return GetRootComponent();
}

bool AMythicCharacter_Player::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    // Offer the "Revive" interaction ONLY while downed; a healthy player exposes no prompt (the focus sweep still
    // sees the pawn, but a false return means "not interactable right now").
    if (!LifeComponent || !LifeComponent->IsDowned()) {
        return false;
    }
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = ReviveInteractionName;
    return true;
}

void AMythicCharacter_Player::OnFocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}

void AMythicCharacter_Player::OnUnfocused_Implementation(AActor *Interactor) {
    // Visual feedback handled in Blueprint.
}
