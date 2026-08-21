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
#include "Settings/MythicDeveloperSettings.h"
#include "MythicPlayerController.h"

AMythicCharacter_Player::AMythicCharacter_Player() {
    GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

    LifeComponent = CreateDefaultSubobject<UMythicLifeComponent>(TEXT("LifeComponent"));

    bUseControllerRotationPitch = false;
    bUseControllerRotationYaw = false;
    bUseControllerRotationRoll = false;

    GetCharacterMovement()->bOrientRotationToMovement = true;
    GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
    GetCharacterMovement()->bConstrainToPlane = true;
    GetCharacterMovement()->bSnapToPlaneAtStart = true;
}


void AMythicCharacter_Player::InitializeASC() {
    auto OwningController = GetController();
    auto ASCInterface = Cast<IAbilitySystemInterface>(OwningController);
    if (ASCInterface) {
        this->ASC_Ref = ASCInterface->GetAbilitySystemComponent();
        if (ASC_Ref && ASC_Ref->HasBeenInitialized()) {
            ASC_Ref->InitAbilityActorInfo(GetPlayerState<AMythicPlayerState>(), this);
            if (LifeComponent && !LifeComponent->IsInitialized()) {
                LifeComponent->InitializeWithAbilitySystem(ASC_Ref);
            }
        }
        return;
    }

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
            LifeComponent->ServerBeginReviveChannel(ReviverPC->GetPawn());
        }
        return;
    }
    if (ReviverPC->IsLocalController()) {
        ReviverPC->ServerInteractPrimary(this);
    }
}

void AMythicCharacter_Player::OnSecondaryInteract_Implementation(AActor *Interactor) {
}

USceneComponent *AMythicCharacter_Player::GetWidgetAttachmentComponent_Implementation() const {
    return GetRootComponent();
}

bool AMythicCharacter_Player::GetInteractionData_Implementation(AActor *Interactor, FMythicInteractionData &OutInteractionData) const {
    if (!LifeComponent || !LifeComponent->IsDowned()) {
        return false;
    }
    OutInteractionData.InputActionDataTable = InputActionDataTable;
    OutInteractionData.PrimaryInteractionName = ReviveInteractionName;
    return true;
}

void AMythicCharacter_Player::OnFocused_Implementation(AActor *Interactor) {
}

void AMythicCharacter_Player::OnUnfocused_Implementation(AActor *Interactor) {
}
