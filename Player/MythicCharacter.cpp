#include "MythicCharacter.h"
#include "MythicPlayerController.h"
#include "MythicPlayerState.h"
#include "GAS/AttributeSets/Shared/MythicAttributeSet_Life.h"
#include "Net/UnrealNetwork.h"

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
