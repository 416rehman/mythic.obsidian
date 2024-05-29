// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicPlayerController.h"

AMythicPlayerController::AMythicPlayerController() {
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;
	bReplicateUsingRegisteredSubObjectList = true;
}

void AMythicPlayerController::BeginPlay() {
	// Call the base class  
	Super::BeginPlay();
}

void AMythicPlayerController::SetupInputComponent() {
	// set up gameplay key bindings
	Super::SetupInputComponent();
}