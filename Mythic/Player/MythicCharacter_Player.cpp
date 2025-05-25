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

AMythicCharacter_Player::AMythicCharacter_Player() {
    // Set size for player capsule
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

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
            ASC_Ref->InitAbilityActorInfo(OwningController, this);
        }
        return;
    }

    // Otherwise, cast the player state
    ASCInterface = Cast<IAbilitySystemInterface>(GetPlayerState());
    if (ASCInterface) {
        this->ASC_Ref = ASCInterface->GetAbilitySystemComponent();
        if (ASC_Ref && ASC_Ref->HasBeenInitialized()) {
            ASC_Ref->InitAbilityActorInfo(GetPlayerState<AMythicPlayerState>(), this);
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
