// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "CommonPlayerController.h"
#include "Player/LyraPlayerController.h"
#include "MythicPlayerController.generated.h"

UCLASS()
class AMythicPlayerController : public ALyraPlayerController
{
	GENERATED_BODY()

public:
	AMythicPlayerController();

protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay();
};


