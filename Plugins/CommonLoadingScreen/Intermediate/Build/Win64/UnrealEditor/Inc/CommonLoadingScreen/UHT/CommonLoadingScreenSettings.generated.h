// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "CommonLoadingScreenSettings.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONLOADINGSCREEN_CommonLoadingScreenSettings_generated_h
#error "CommonLoadingScreenSettings.generated.h already included, missing '#pragma once' in CommonLoadingScreenSettings.h"
#endif
#define COMMONLOADINGSCREEN_CommonLoadingScreenSettings_generated_h

#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h_18_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonLoadingScreenSettings(); \
	friend struct Z_Construct_UClass_UCommonLoadingScreenSettings_Statics; \
public: \
	DECLARE_CLASS(UCommonLoadingScreenSettings, UDeveloperSettingsBackedByCVars, COMPILED_IN_FLAGS(0 | CLASS_DefaultConfig | CLASS_Config), CASTCLASS_None, TEXT("/Script/CommonLoadingScreen"), NO_API) \
	DECLARE_SERIALIZER(UCommonLoadingScreenSettings) \
	static const TCHAR* StaticConfigName() {return TEXT("Game");} \



#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h_18_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UCommonLoadingScreenSettings(UCommonLoadingScreenSettings&&); \
	UCommonLoadingScreenSettings(const UCommonLoadingScreenSettings&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonLoadingScreenSettings); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonLoadingScreenSettings); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UCommonLoadingScreenSettings) \
	NO_API virtual ~UCommonLoadingScreenSettings();


#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h_15_PROLOG
#define FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h_18_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h_18_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h_18_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONLOADINGSCREEN_API UClass* StaticClass<class UCommonLoadingScreenSettings>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Private_CommonLoadingScreenSettings_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
