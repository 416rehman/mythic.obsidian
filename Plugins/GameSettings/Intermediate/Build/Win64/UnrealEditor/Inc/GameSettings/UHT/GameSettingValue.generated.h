// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GameSettingValue.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef GAMESETTINGS_GameSettingValue_generated_h
#error "GameSettingValue.generated.h already included, missing '#pragma once' in GameSettingValue.h"
#endif
#define GAMESETTINGS_GameSettingValue_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_22_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSettingValue(); \
	friend struct Z_Construct_UClass_UGameSettingValue_Statics; \
public: \
	DECLARE_CLASS(UGameSettingValue, UGameSetting, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSettingValue)


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_22_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameSettingValue(UGameSettingValue&&); \
	UGameSettingValue(const UGameSettingValue&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSettingValue); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSettingValue); \
	DEFINE_ABSTRACT_DEFAULT_CONSTRUCTOR_CALL(UGameSettingValue) \
	NO_API virtual ~UGameSettingValue();


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_19_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_22_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_22_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_22_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSettingValue>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
