// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace LyraGameplayTags
{
	MYTHIC_API	FGameplayTag FindTagByString(const FString& TagString, bool bMatchPartialString = false);

	// Declare all of the custom native tags that Lyra will use
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_IsDead);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_Cooldown);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_Cost);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_TagsBlocked);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_TagsMissing);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_Networking);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_ActivateFail_ActivationGroup);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Ability_Behavior_SurvivesDeath);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Stick);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_AutoRun);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Death);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_Reset);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(GameplayEvent_RequestReset);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Damage);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(SetByCaller_Heal);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cheat_GodMode);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Cheat_UnlimitedHealth);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Crouching);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_AutoRunning);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dying);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Status_Death_Dead);

	// These are mappings from MovementMode enums to GameplayTags associated with those enums (below)
	MYTHIC_API	extern const TMap<uint8, FGameplayTag> MovementModeTagMap;
	MYTHIC_API	extern const TMap<uint8, FGameplayTag> CustomMovementModeTagMap;

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Walking);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_NavWalking);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Falling);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Swimming);
	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Flying);

	MYTHIC_API	UE_DECLARE_GAMEPLAY_TAG_EXTERN(Movement_Mode_Custom);
};
