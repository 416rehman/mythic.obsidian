// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "GameSettingCollection.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef GAMESETTINGS_GameSettingCollection_generated_h
#error "GameSettingCollection.generated.h already included, missing '#pragma once' in GameSettingCollection.h"
#endif
#define GAMESETTINGS_GameSettingCollection_generated_h

#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_18_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSettingCollection(); \
	friend struct Z_Construct_UClass_UGameSettingCollection_Statics; \
public: \
	DECLARE_CLASS(UGameSettingCollection, UGameSetting, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSettingCollection)


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_18_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameSettingCollection(UGameSettingCollection&&); \
	UGameSettingCollection(const UGameSettingCollection&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSettingCollection); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSettingCollection); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UGameSettingCollection) \
	NO_API virtual ~UGameSettingCollection();


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_15_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_18_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_18_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_18_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSettingCollection>();

#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_44_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUGameSettingCollectionPage(); \
	friend struct Z_Construct_UClass_UGameSettingCollectionPage_Statics; \
public: \
	DECLARE_CLASS(UGameSettingCollectionPage, UGameSettingCollection, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/GameSettings"), NO_API) \
	DECLARE_SERIALIZER(UGameSettingCollectionPage)


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_44_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UGameSettingCollectionPage(UGameSettingCollectionPage&&); \
	UGameSettingCollectionPage(const UGameSettingCollectionPage&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UGameSettingCollectionPage); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UGameSettingCollectionPage); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UGameSettingCollectionPage) \
	NO_API virtual ~UGameSettingCollectionPage();


#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_41_PROLOG
#define FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_44_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_44_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_44_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> GAMESETTINGS_API UClass* StaticClass<class UGameSettingCollectionPage>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
