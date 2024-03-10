// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "CommonUserSubsystem.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
class UCommonUserInfo;
enum class ECommonUserAvailability : uint8;
enum class ECommonUserInitializationState : uint8;
enum class ECommonUserOnlineContext : uint8;
enum class ECommonUserPrivilege : uint8;
enum class ECommonUserPrivilegeResult : uint8;
struct FCommonUserInitializeParams;
struct FGameplayTag;
struct FInputDeviceId;
struct FKey;
struct FPlatformUserId;
struct FUniqueNetIdRepl;
#ifdef COMMONUSER_CommonUserSubsystem_generated_h
#error "CommonUserSubsystem.generated.h already included, missing '#pragma once' in CommonUserSubsystem.h"
#endif
#define COMMONUSER_CommonUserSubsystem_generated_h

#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_SPARSE_DATA
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execGetDebugString); \
	DECLARE_FUNCTION(execGetNickname); \
	DECLARE_FUNCTION(execGetNetId); \
	DECLARE_FUNCTION(execGetPrivilegeAvailability); \
	DECLARE_FUNCTION(execGetCachedPrivilegeResult); \
	DECLARE_FUNCTION(execIsDoingLogin); \
	DECLARE_FUNCTION(execIsLoggedIn);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonUserInfo(); \
	friend struct Z_Construct_UClass_UCommonUserInfo_Statics; \
public: \
	DECLARE_CLASS(UCommonUserInfo, UObject, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonUser"), NO_API) \
	DECLARE_SERIALIZER(UCommonUserInfo)


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_ENHANCED_CONSTRUCTORS \
	/** Standard constructor, called after all reflected properties have been initialized */ \
	NO_API UCommonUserInfo(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get()); \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UCommonUserInfo(UCommonUserInfo&&); \
	NO_API UCommonUserInfo(const UCommonUserInfo&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonUserInfo); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonUserInfo); \
	DEFINE_DEFAULT_OBJECT_INITIALIZER_CONSTRUCTOR_CALL(UCommonUserInfo) \
	NO_API virtual ~UCommonUserInfo();


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_46_PROLOG
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_SPARSE_DATA \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_49_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONUSER_API UClass* StaticClass<class UCommonUserInfo>();

#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_141_DELEGATE \
COMMONUSER_API void FCommonUserOnInitializeCompleteMulticast_DelegateWrapper(const FMulticastScriptDelegate& CommonUserOnInitializeCompleteMulticast, const UCommonUserInfo* UserInfo, bool bSuccess, const FText& Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_142_DELEGATE \
COMMONUSER_API void FCommonUserOnInitializeComplete_DelegateWrapper(const FScriptDelegate& CommonUserOnInitializeComplete, const UCommonUserInfo* UserInfo, bool bSuccess, const FText& Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_145_DELEGATE \
COMMONUSER_API void FCommonUserHandleSystemMessageDelegate_DelegateWrapper(const FMulticastScriptDelegate& CommonUserHandleSystemMessageDelegate, FGameplayTag MessageType, const FText& TitleText, const FText& BodyText);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_148_DELEGATE \
COMMONUSER_API void FCommonUserAvailabilityChangedDelegate_DelegateWrapper(const FMulticastScriptDelegate& CommonUserAvailabilityChangedDelegate, const UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserAvailability OldAvailability, ECommonUserAvailability NewAvailability);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_155_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics; \
	static class UScriptStruct* StaticStruct();


template<> COMMONUSER_API UScriptStruct* StaticStruct<struct FCommonUserInitializeParams>();

#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_SPARSE_DATA
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_RPC_WRAPPERS_NO_PURE_DECLS \
 \
	DECLARE_FUNCTION(execShouldWaitForStartInput); \
	DECLARE_FUNCTION(execHasTraitTag); \
	DECLARE_FUNCTION(execResetUserState); \
	DECLARE_FUNCTION(execTryToLogOutUser); \
	DECLARE_FUNCTION(execCancelUserInitialization); \
	DECLARE_FUNCTION(execListenForLoginKeyInput); \
	DECLARE_FUNCTION(execTryToInitializeUser); \
	DECLARE_FUNCTION(execTryToLoginForOnlinePlay); \
	DECLARE_FUNCTION(execTryToInitializeForLocalPlay); \
	DECLARE_FUNCTION(execGetUserInfoForInputDevice); \
	DECLARE_FUNCTION(execGetUserInfoForControllerId); \
	DECLARE_FUNCTION(execGetUserInfoForUniqueNetId); \
	DECLARE_FUNCTION(execGetUserInfoForPlatformUser); \
	DECLARE_FUNCTION(execGetUserInfoForPlatformUserIndex); \
	DECLARE_FUNCTION(execGetUserInfoForLocalPlayerIndex); \
	DECLARE_FUNCTION(execGetLocalPlayerInitializationState); \
	DECLARE_FUNCTION(execGetNumLocalPlayers); \
	DECLARE_FUNCTION(execGetMaxLocalPlayers); \
	DECLARE_FUNCTION(execSetMaxLocalPlayers); \
	DECLARE_FUNCTION(execSendSystemMessage);


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_ACCESSORS
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_INCLASS_NO_PURE_DECLS \
private: \
	static void StaticRegisterNativesUCommonUserSubsystem(); \
	friend struct Z_Construct_UClass_UCommonUserSubsystem_Statics; \
public: \
	DECLARE_CLASS(UCommonUserSubsystem, UGameInstanceSubsystem, COMPILED_IN_FLAGS(0), CASTCLASS_None, TEXT("/Script/CommonUser"), NO_API) \
	DECLARE_SERIALIZER(UCommonUserSubsystem) \
	static const TCHAR* StaticConfigName() {return TEXT("Game");} \



#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_ENHANCED_CONSTRUCTORS \
private: \
	/** Private move- and copy-constructors, should never be used */ \
	NO_API UCommonUserSubsystem(UCommonUserSubsystem&&); \
	NO_API UCommonUserSubsystem(const UCommonUserSubsystem&); \
public: \
	DECLARE_VTABLE_PTR_HELPER_CTOR(NO_API, UCommonUserSubsystem); \
	DEFINE_VTABLE_PTR_HELPER_CTOR_CALLER(UCommonUserSubsystem); \
	DEFINE_DEFAULT_CONSTRUCTOR_CALL(UCommonUserSubsystem) \
	NO_API virtual ~UCommonUserSubsystem();


#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_203_PROLOG
#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_GENERATED_BODY \
PRAGMA_DISABLE_DEPRECATION_WARNINGS \
public: \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_SPARSE_DATA \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_EDITOR_ONLY_SPARSE_DATA_PROPERTY_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_RPC_WRAPPERS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_ACCESSORS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_INCLASS_NO_PURE_DECLS \
	FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_206_ENHANCED_CONSTRUCTORS \
private: \
PRAGMA_ENABLE_DEPRECATION_WARNINGS


template<> COMMONUSER_API UClass* StaticClass<class UCommonUserSubsystem>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h


PRAGMA_ENABLE_DEPRECATION_WARNINGS
