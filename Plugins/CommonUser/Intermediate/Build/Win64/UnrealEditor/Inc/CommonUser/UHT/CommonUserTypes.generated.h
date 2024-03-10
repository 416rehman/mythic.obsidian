// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

// IWYU pragma: private, include "CommonUserTypes.h"
#include "UObject/ObjectMacros.h"
#include "UObject/ScriptMacros.h"

PRAGMA_DISABLE_DEPRECATION_WARNINGS
#ifdef COMMONUSER_CommonUserTypes_generated_h
#error "CommonUserTypes.generated.h already included, missing '#pragma once' in CommonUserTypes.h"
#endif
#define COMMONUSER_CommonUserTypes_generated_h

#define FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserTypes_h_199_GENERATED_BODY \
	friend struct Z_Construct_UScriptStruct_FOnlineResultInformation_Statics; \
	COMMONUSER_API static class UScriptStruct* StaticStruct();


template<> COMMONUSER_API UScriptStruct* StaticStruct<struct FOnlineResultInformation>();

#undef CURRENT_FILE_ID
#define CURRENT_FILE_ID FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserTypes_h


#define FOREACH_ENUM_ECOMMONUSERONLINECONTEXT(op) \
	op(ECommonUserOnlineContext::Game) \
	op(ECommonUserOnlineContext::Default) \
	op(ECommonUserOnlineContext::Service) \
	op(ECommonUserOnlineContext::ServiceOrDefault) \
	op(ECommonUserOnlineContext::Platform) \
	op(ECommonUserOnlineContext::PlatformOrDefault) \
	op(ECommonUserOnlineContext::Invalid) 

enum class ECommonUserOnlineContext : uint8;
template<> struct TIsUEnumClass<ECommonUserOnlineContext> { enum { Value = true }; };
template<> COMMONUSER_API UEnum* StaticEnum<ECommonUserOnlineContext>();

#define FOREACH_ENUM_ECOMMONUSERINITIALIZATIONSTATE(op) \
	op(ECommonUserInitializationState::Unknown) \
	op(ECommonUserInitializationState::DoingInitialLogin) \
	op(ECommonUserInitializationState::DoingNetworkLogin) \
	op(ECommonUserInitializationState::FailedtoLogin) \
	op(ECommonUserInitializationState::LoggedInOnline) \
	op(ECommonUserInitializationState::LoggedInLocalOnly) \
	op(ECommonUserInitializationState::Invalid) 

enum class ECommonUserInitializationState : uint8;
template<> struct TIsUEnumClass<ECommonUserInitializationState> { enum { Value = true }; };
template<> COMMONUSER_API UEnum* StaticEnum<ECommonUserInitializationState>();

#define FOREACH_ENUM_ECOMMONUSERPRIVILEGE(op) \
	op(ECommonUserPrivilege::CanPlay) \
	op(ECommonUserPrivilege::CanPlayOnline) \
	op(ECommonUserPrivilege::CanCommunicateViaTextOnline) \
	op(ECommonUserPrivilege::CanCommunicateViaVoiceOnline) \
	op(ECommonUserPrivilege::CanUseUserGeneratedContent) \
	op(ECommonUserPrivilege::CanUseCrossPlay) \
	op(ECommonUserPrivilege::Invalid_Count) 

enum class ECommonUserPrivilege : uint8;
template<> struct TIsUEnumClass<ECommonUserPrivilege> { enum { Value = true }; };
template<> COMMONUSER_API UEnum* StaticEnum<ECommonUserPrivilege>();

#define FOREACH_ENUM_ECOMMONUSERAVAILABILITY(op) \
	op(ECommonUserAvailability::Unknown) \
	op(ECommonUserAvailability::NowAvailable) \
	op(ECommonUserAvailability::PossiblyAvailable) \
	op(ECommonUserAvailability::CurrentlyUnavailable) \
	op(ECommonUserAvailability::AlwaysUnavailable) \
	op(ECommonUserAvailability::Invalid) 

enum class ECommonUserAvailability : uint8;
template<> struct TIsUEnumClass<ECommonUserAvailability> { enum { Value = true }; };
template<> COMMONUSER_API UEnum* StaticEnum<ECommonUserAvailability>();

#define FOREACH_ENUM_ECOMMONUSERPRIVILEGERESULT(op) \
	op(ECommonUserPrivilegeResult::Unknown) \
	op(ECommonUserPrivilegeResult::Available) \
	op(ECommonUserPrivilegeResult::UserNotLoggedIn) \
	op(ECommonUserPrivilegeResult::LicenseInvalid) \
	op(ECommonUserPrivilegeResult::VersionOutdated) \
	op(ECommonUserPrivilegeResult::NetworkConnectionUnavailable) \
	op(ECommonUserPrivilegeResult::AgeRestricted) \
	op(ECommonUserPrivilegeResult::AccountTypeRestricted) \
	op(ECommonUserPrivilegeResult::AccountUseRestricted) \
	op(ECommonUserPrivilegeResult::PlatformFailure) 

enum class ECommonUserPrivilegeResult : uint8;
template<> struct TIsUEnumClass<ECommonUserPrivilegeResult> { enum { Value = true }; };
template<> COMMONUSER_API UEnum* StaticEnum<ECommonUserPrivilegeResult>();

PRAGMA_ENABLE_DEPRECATION_WARNINGS
