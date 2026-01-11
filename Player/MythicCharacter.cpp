#include "MythicCharacter.h"
#include "Mythic/Mythic.h"
#include "MythicPlayerController.h"
#include "MythicPlayerState.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Net/UnrealNetwork.h"
#include "Input/MythicInputComponent.h"
#include "Input/MythicInputConfig.h"
#include "GAS/MythicAbilitySystemComponent.h"

AMythicCharacter::AMythicCharacter() {
    // Replication
    bReplicates = true;
}

void AMythicCharacter::BeginPlay() {
    Super::BeginPlay();

    InitializeASC();
}

void AMythicCharacter::PossessedBy(AController *NewController) {
    Super::PossessedBy(NewController);

    InitializeASC();
}

void AMythicCharacter::OnRep_Controller() {
    Super::OnRep_Controller();

    InitializeASC();
}

void AMythicCharacter::OnRep_PlayerState() {
    Super::OnRep_PlayerState();

    InitializeASC();
}

void AMythicCharacter::OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode) {
    OnMythicMovementModeChange.Broadcast(PrevMovementMode, PreviousCustomMode);
}

void AMythicCharacter::SetupPlayerInputComponent(UInputComponent *PlayerInputComponent) {
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    UMythicInputComponent *MythicIC = Cast<UMythicInputComponent>(PlayerInputComponent);
    if (!MythicIC) {
        UE_LOG(Myth, Error, TEXT("AMythicCharacter::SetupPlayerInputComponent: Input Component is not UMythicInputComponent. Check Project Settings."));
        return;
    }

    if (!InputConfig) {
        UE_LOG(Myth, Warning, TEXT("AMythicCharacter::SetupPlayerInputComponent: InputConfig is null."));
        return;
    }

    // Bind Ability Actions (Gas Input)
    TArray<uint32> BindHandles;
    MythicIC->BindAbilityActions(InputConfig, this, &ThisClass::Input_AbilityInputTagPressed, &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

    UE_LOG(Myth, Log, TEXT("SetupPlayerInputComponent: Bound %d Ability Actions"), BindHandles.Num());
}

void AMythicCharacter::Input_AbilityInputTagPressed(FGameplayTag InputTag) {
    UE_LOG(Myth, Log, TEXT("AMythicCharacter::Input_AbilityInputTagPressed: Tag=%s, Character=%s"),
           *InputTag.ToString(),
           *GetName());
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        UE_LOG(Myth, Log, TEXT("  -> Forwarding to ASC: %s"), *GetNameSafe(MythicASC));
        MythicASC->AbilityInputTagPressed(InputTag);
    }
    else {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: No ASC found!"));
    }
}

void AMythicCharacter::Input_AbilityInputTagReleased(FGameplayTag InputTag) {
    UE_LOG(Myth, Log, TEXT("AMythicCharacter::Input_AbilityInputTagReleased: Tag=%s, Character=%s"),
           *InputTag.ToString(),
           *GetName());
    if (UMythicAbilitySystemComponent *MythicASC = Cast<UMythicAbilitySystemComponent>(GetAbilitySystemComponent())) {
        UE_LOG(Myth, Log, TEXT("  -> Forwarding to ASC: %s"), *GetNameSafe(MythicASC));
        MythicASC->AbilityInputTagReleased(InputTag);
    }
    else {
        UE_LOG(Myth, Error, TEXT("  -> FAILED: No ASC found!"));
    }
}
