// Copyright Epic Games, Inc. All Rights Reserved.

#include "MythicGameInstance.h"

#include "CommonLocalPlayer.h"
#include "CommonSessionSubsystem.h"
#include "GameplayTagContainer.h"
#include "GameUIManagerSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MythicGameInstance)

UMythicGameInstance::UMythicGameInstance(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
}

void UMythicGameInstance::Init()
{
	Super::Init();
	
}

void UMythicGameInstance::Shutdown()
{
	Super::Shutdown();
}


bool UMythicGameInstance::CanJoinRequestedSession() const
{
	// Temporary first pass:  Always return true
	// This will be fleshed out to check the player's state
	if (!Super::CanJoinRequestedSession())
	{
		return false;
	}
	return true;
}
