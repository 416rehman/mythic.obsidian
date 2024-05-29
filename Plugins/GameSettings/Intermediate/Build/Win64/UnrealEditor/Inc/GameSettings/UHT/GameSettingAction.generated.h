// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GameSettingAction.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef GAMESETTINGS_GameSettingAction_generated_h
#error "GameSettingAction.generated.h already included, missing '#pragma once' in GameSettingAction.h"
#endif
#define GAMESETTINGS_GameSettingAction_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_23_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSettingAction(); \
	friend struct Z_Construct_UClass_UGameSettingAction_Statics; \
public: \
	DECLARE_CLASS(UGameSettingAction, UGameSetting, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSettingAction)


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_23_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameSettingAction(UGameSettingAction&&); \
	UGameSettingAction(const UGameSettingAction&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSettingAction); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSettingAction); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UGameSettingAction) \
	NO_API virtual ~UGameSettingAction();


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_20_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_23_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_23_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_23_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSettingAction>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
