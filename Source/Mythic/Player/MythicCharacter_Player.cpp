// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicCharacter_Player.h"

#include "AbilitySystemComponent.h"
#include "MythicPlayerState.h"
#include "UObject/ConstructorHelpers.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Materials/Material.h"
#include "Engine/World.h"

AMythicCharacter_Player::AMythicCharacter_Player()
{
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

	// Create a camera boom...
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true); // Don't want arm to rotate when character does
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false; // Don't want to pull camera in when it collides with level

	// Create a camera...
	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));
	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false; // Camera does not rotate relative to arm

	// Activate ticking in order to update the cursor every frame.
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = true;
}

void AMythicCharacter_Player::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
}

void AMythicCharacter_Player::InitPlayer() {
	if (AMythicPlayerState* PS = GetPlayerState<AMythicPlayerState>()) {
	    auto ASC = PS->GetAbilitySystemComponent();
	    if (ASC != nullptr) {
	        ASC->InitAbilityActorInfo(PS, this);
	        UE_LOG(LogTemp, Warning, TEXT("PlayerState %s ability system component initialized"), *PS->GetName());
	    }
        else {
            UE_LOG(LogTemp, Warning, TEXT("PlayerState %s has no AbilitySystemComponent"), *PS->GetName());
        }
	} else {
        UE_LOG(LogTemp, Warning, TEXT("Player has no PlayerState"));
    }
}

void AMythicCharacter_Player::PossessedBy(AController* NewController) {
	Super::PossessedBy(NewController);
	this->InitPlayer();
}

void AMythicCharacter_Player::OnRep_PlayerState() {
	Super::OnRep_PlayerState();
	this->InitPlayer();
}