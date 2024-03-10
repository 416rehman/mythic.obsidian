// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonGameInstance.h"
#include "UObject/UObjectGlobals.h"

#include "MythicGameInstance.generated.h"

class UObject;

UCLASS(Config = Game)
class MYTHIC_API UMythicGameInstance : public UCommonGameInstance
{
	GENERATED_BODY()

public:

	UMythicGameInstance(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());	
	virtual bool CanJoinRequestedSession() const override;

protected:

	virtual void Init() override;
	virtual void Shutdown() override;
};
