// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "CommonLocalPlayer.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONGAME_CommonLocalPlayer_generated_h
#error "CommonLocalPlayer.generated.h already included, missing '#pragma once' in CommonLocalPlayer.h"
#endif
#define COMMONGAME_CommonLocalPlayer_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_20_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonLocalPlayer(); \
	friend struct Z_Construct_UClass_UCommonLocalPlayer_Statics; \
public: \
	DECLARE_CLASS(UCommonLocalPlayer, ULocalPlayer, COMPILED_IN_FLAGS(0 | CLASS_Transient | CLASS_Config), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UCommonLocalPlayer)


#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_20_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UCommonLocalPlayer(UCommonLocalPlayer&&); \
	UCommonLocalPlayer(const UCommonLocalPlayer&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonLocalPlayer); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonLocalPlayer); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UCommonLocalPlayer) \
	NO_API virtual ~UCommonLocalPlayer();


#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_17_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_20_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_20_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h_20_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UCommonLocalPlayer>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_CommonLocalPlayer_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
