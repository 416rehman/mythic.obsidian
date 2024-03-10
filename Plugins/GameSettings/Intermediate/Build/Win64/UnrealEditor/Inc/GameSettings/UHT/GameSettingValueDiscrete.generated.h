// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GameSettingValueDiscrete.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef GAMESETTINGS_GameSettingValueDiscrete_generated_h
#error "GameSettingValueDiscrete.generated.h already included, missing '#pragma once' in GameSettingValueDiscrete.h"
#endif
#define GAMESETTINGS_GameSettingValueDiscrete_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_SPARSE_DATA
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execGetDiscreteOptions); \
	DECLARE_FUNCTION(execGetDiscreteOptionDefaultIndex); \
	DECLARE_FUNCTION(execGetDiscreteOptionIndex);


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_ACCESSORS
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSettingValueDiscrete(); \
	friend struct Z_Construct_UClass_UGameSettingValueDiscrete_Statics; \
public: \
	DECLARE_CLASS(UGameSettingValueDiscrete, UGameSettingValue, COMPILED_IN_FLAGS(CLASS_Abstract), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSettingValueDiscrete)


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UGameSettingValueDiscrete(UGameSettingValueDiscrete&&); \
	NO_API UGameSettingValueDiscrete(const UGameSettingValueDiscrete&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSettingValueDiscrete); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSettingValueDiscrete); \
	DEFINE_ABSTRACT_DEFAULT_CONSTRUCTOR_CALL(UGameSettingValueDiscrete) \
	NO_API virtual ~UGameSettingValueDiscrete();


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_12_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_SPARSE_DATA \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_ACCESSORS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h_15_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSettingValueDiscrete>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueDiscrete_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
