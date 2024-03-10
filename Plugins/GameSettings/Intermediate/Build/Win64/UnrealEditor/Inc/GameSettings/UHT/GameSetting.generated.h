// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GameSetting.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
enum class ESlateVisibility : uint8;
struct FGameplayTagContainer;
#ifdef GAMESETTINGS_GameSetting_generated_h
#error "GameSetting.generated.h already included, missing '#pragma once' in GameSetting.h"
#endif
#define GAMESETTINGS_GameSetting_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_SPARSE_DATA
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execGetWarningRichText); \
	DECLARE_FUNCTION(execGetDynamicDetails); \
	DECLARE_FUNCTION(execGetTags); \
	DECLARE_FUNCTION(execGetDescriptionRichText); \
	DECLARE_FUNCTION(execGetDisplayNameVisibility); \
	DECLARE_FUNCTION(execGetDisplayName); \
	DECLARE_FUNCTION(execGetDevName);


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_ACCESSORS
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSetting(); \
	friend struct Z_Construct_UClass_UGameSetting_Statics; \
public: \
	DECLARE_CLASS(UGameSetting, UObject, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSetting)


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UGameSetting(UGameSetting&&); \
	NO_API UGameSetting(const UGameSetting&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSetting); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSetting); \
	DEFINE_ABSTRACT_DEFAULT_CONSTRUCTOR_CALL(UGameSetting) \
	NO_API virtual ~UGameSetting();


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_24_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_SPARSE_DATA \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_ACCESSORS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h_27_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSetting>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Public_GameSetting_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
