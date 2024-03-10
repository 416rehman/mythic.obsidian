// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "AsyncAction_CommonUserInitialize.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UAsyncAction_CommonUserInitialize;
class UCommonUserInfo;
class UCommonUserSubsystem;
enum class ECommonUserOnlineContext : uint8;
enum class ECommonUserPrivilege : uint8;
struct FInputDeviceId;
#ifdef COMMONUSER_AsyncAction_CommonUserInitialize_generated_h
#error "AsyncAction_CommonUserInitialize.generated.h already included, missing '#pragma once' in AsyncAction_CommonUserInitialize.h"
#endif
#define COMMONUSER_AsyncAction_CommonUserInitialize_generated_h

#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_SPARSE_DATA
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execHandleInitializationComplete); \
	DECLARE_FUNCTION(execLoginForOnlinePlay); \
	DECLARE_FUNCTION(execInitializeForLocalPlay);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUAsyncAction_CommonUserInitialize(); \
	friend struct Z_Construct_UClass_UAsyncAction_CommonUserInitialize_Statics; \
public: \
	DECLARE_CLASS(UAsyncAction_CommonUserInitialize, UCancellableAsyncAction, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonUser"), NO_API) \
	DECLARE_SERIALIZER(UAsyncAction_CommonUserInitialize)


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UAsyncAction_CommonUserInitialize(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UAsyncAction_CommonUserInitialize(UAsyncAction_CommonUserInitialize&&); \
	NO_API UAsyncAction_CommonUserInitialize(const UAsyncAction_CommonUserInitialize&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UAsyncAction_CommonUserInitialize); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UAsyncAction_CommonUserInitialize); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UAsyncAction_CommonUserInitialize) \
	NO_API virtual ~UAsyncAction_CommonUserInitialize();


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_21_PROLOG
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_SPARSE_DATA \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h_24_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONUSER_API UClass* StaticClass<class UAsyncAction_CommonUserInitialize>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_AsyncAction_CommonUserInitialize_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
