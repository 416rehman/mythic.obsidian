// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Logging/LogMacros.h"

class UObject;

MYTHIC_API DECLARE_LOG_CATEGORY_EXTERN(LogLyra, Log, All);
MYTHIC_API DECLARE_LOG_CATEGORY_EXTERN(LogLyraExperience, Log, All);
MYTHIC_API DECLARE_LOG_CATEGORY_EXTERN(LogLyraAbilitySystem, Log, All);
MYTHIC_API DECLARE_LOG_CATEGORY_EXTERN(LogLyraTeams, Log, All);

MYTHIC_API FString GetClientServerContextString(UObject* ContextObject = nullptr);
