// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicPlayerController.h"

#include "MythicPlayerState.h"

AMythicPlayerController::AMythicPlayerController() {
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	bReplicateUsingRegisteredSubObjectList = true;
}

UAbilitySystemComponent * AMythicPlayerController::GetAbilitySystemComponent() const {
    auto PS = GetPlayerState<AMythicPlayerState>();
    return PS ? PS->GetAbilitySystemComponent() : nullptr;
}

void AMythicPlayerController::BeginPlay() {
	// Call the base class  
	Super::BeginPlay();
}

void AMythicPlayerController::SetupInputComponent() {
	// set up gameplay key bindings
	Super::SetupInputComponent();
}