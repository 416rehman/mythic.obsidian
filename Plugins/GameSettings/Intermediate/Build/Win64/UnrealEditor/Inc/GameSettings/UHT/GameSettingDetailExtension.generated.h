// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "Widgets/GameSettingDetailExtension.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UGameSetting;
#ifdef GAMESETTINGS_GameSettingDetailExtension_generated_h
#error "GameSettingDetailExtension.generated.h already included, missing '#pragma once' in GameSettingDetailExtension.h"
#endif
#define GAMESETTINGS_GameSettingDetailExtension_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_CALLBACK_WRAPPERS
#define FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSettingDetailExtension(); \
	friend struct Z_Construct_UClass_UGameSettingDetailExtension_Statics; \
public: \
	DECLARE_CLASS(UGameSettingDetailExtension, UUserWidget, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSettingDetailExtension)


#define FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UGameSettingDetailExtension(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameSettingDetailExtension(UGameSettingDetailExtension&&); \
	UGameSettingDetailExtension(const UGameSettingDetailExtension&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSettingDetailExtension); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSettingDetailExtension); \
	DEFINE_ABSTRACT_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UGameSettingDetailExtension) \
	NO_API virtual ~UGameSettingDetailExtension();


#define FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_17_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_CALLBACK_WRAPPERS \
	FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSettingDetailExtension>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
