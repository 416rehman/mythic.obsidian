// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "CommonUIExtensions.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class APlayerController;
class UCommonActivatableWidget;
class ULocalPlayer;
class UUserWidget;
enum class ECommonInputType : uint8;
struct FGameplayTag;
#ifdef COMMONGAME_CommonUIExtensions_generated_h
#error "CommonUIExtensions.generated.h already included, missing '#pragma once' in CommonUIExtensions.h"
#endif
#define COMMONGAME_CommonUIExtensions_generated_h

#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_RPC_WRAPPERS_NO_PURE_DECLS \
	DECLARE_FUNCTION(execResumeInputForPlayer); \
	DECLARE_FUNCTION(execSuspendInputForPlayer); \
	DECLARE_FUNCTION(execGetLocalPlayerFromController); \
	DECLARE_FUNCTION(execPopContentFromLayer); \
	DECLARE_FUNCTION(execPushStreamedContentToLayer_ForPlayer); \
	DECLARE_FUNCTION(execPushContentToLayer_ForPlayer); \
	DECLARE_FUNCTION(execIsOwningPlayerUsingGamepad); \
	DECLARE_FUNCTION(execIsOwningPlayerUsingTouch); \
	DECLARE_FUNCTION(execGetOwningPlayerInputType);


#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonUIExtensions(); \
	friend struct Z_Construct_UClass_UCommonUIExtensions_Statics; \
public: \
	DECLARE_CLASS(UCommonUIExtensions, UBlueprintFunctionLibrary, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonGame"), NO_API) \
	DECLARE_SERIALIZER(UCommonUIExtensions)


#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	UCommonUIExtensions(UCommonUIExtensions&&); \
	UCommonUIExtensions(const UCommonUIExtensions&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonUIExtensions); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonUIExtensions); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UCommonUIExtensions) \
	NO_API virtual ~UCommonUIExtensions();


#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_21_PROLOG
#define FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_24_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONGAME_API UClass* StaticClass<class UCommonUIExtensions>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
