// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "MythicPlayerController.generated.h"

UCLASS()
class AMythicPlayerController : public APlayerController, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AMythicPlayerController();

    virtual UAbilitySystemComponent *GetAbilitySystemComponent() const override;
protected:
	virtual void SetupInputComponent() override;
	virtual void BeginPlay();
    
};


