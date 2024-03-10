// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CommonUser/Public/CommonUserSubsystem.h"
#include "../../Source/Runtime/Engine/Classes/Engine/GameInstance.h"
#include "../../Source/Runtime/Engine/Classes/GameFramework/OnlineReplStructs.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonUserSubsystem() {}
// Cross Module References
	COMMONUSER_API UClass* Z_Construct_UClass_UCommonUserInfo();
	COMMONUSER_API UClass* Z_Construct_UClass_UCommonUserInfo_NoRegister();
	COMMONUSER_API UClass* Z_Construct_UClass_UCommonUserSubsystem();
	COMMONUSER_API UClass* Z_Construct_UClass_UCommonUserSubsystem_NoRegister();
	COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserAvailability();
	COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserInitializationState();
	COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext();
	COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserPrivilege();
	COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserPrivilegeResult();
	COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature();
	COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature();
	COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature();
	COMMONUSER_API UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature();
	COMMONUSER_API UScriptStruct* Z_Construct_UScriptStruct_FCommonUserInitializeParams();
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FInputDeviceId();
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FPlatformUserId();
	ENGINE_API UClass* Z_Construct_UClass_UGameInstanceSubsystem();
	ENGINE_API UScriptStruct* Z_Construct_UScriptStruct_FUniqueNetIdRepl();
	GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
	INPUTCORE_API UScriptStruct* Z_Construct_UScriptStruct_FKey();
	UPackage* Z_Construct_UPackage__Script_CommonUser();
// End Cross Module References
	DEFINE_FUNCTION(UCommonUserInfo::execGetDebugString)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(FString*)Z_Param__Result=P_THIS->GetDebugString();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserInfo::execGetNickname)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(FString*)Z_Param__Result=P_THIS->GetNickname();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserInfo::execGetNetId)
	{
		P_GET_ENUM(ECommonUserOnlineContext,Z_Param_Context);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(FUniqueNetIdRepl*)Z_Param__Result=P_THIS->GetNetId(ECommonUserOnlineContext(Z_Param_Context));
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserInfo::execGetPrivilegeAvailability)
	{
		P_GET_ENUM(ECommonUserPrivilege,Z_Param_Privilege);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(ECommonUserAvailability*)Z_Param__Result=P_THIS->GetPrivilegeAvailability(ECommonUserPrivilege(Z_Param_Privilege));
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserInfo::execGetCachedPrivilegeResult)
	{
		P_GET_ENUM(ECommonUserPrivilege,Z_Param_Privilege);
		P_GET_ENUM(ECommonUserOnlineContext,Z_Param_Context);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(ECommonUserPrivilegeResult*)Z_Param__Result=P_THIS->GetCachedPrivilegeResult(ECommonUserPrivilege(Z_Param_Privilege),ECommonUserOnlineContext(Z_Param_Context));
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserInfo::execIsDoingLogin)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->IsDoingLogin();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserInfo::execIsLoggedIn)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->IsLoggedIn();
		P_NATIVE_END;
	}
	void UCommonUserInfo::StaticRegisterNativesUCommonUserInfo()
	{
		UClass* Class = UCommonUserInfo::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "GetCachedPrivilegeResult", &UCommonUserInfo::execGetCachedPrivilegeResult },
			{ "GetDebugString", &UCommonUserInfo::execGetDebugString },
			{ "GetNetId", &UCommonUserInfo::execGetNetId },
			{ "GetNickname", &UCommonUserInfo::execGetNickname },
			{ "GetPrivilegeAvailability", &UCommonUserInfo::execGetPrivilegeAvailability },
			{ "IsDoingLogin", &UCommonUserInfo::execIsDoingLogin },
			{ "IsLoggedIn", &UCommonUserInfo::execIsLoggedIn },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics
	{
		struct CommonUserInfo_eventGetCachedPrivilegeResult_Parms
		{
			ECommonUserPrivilege Privilege;
			ECommonUserOnlineContext Context;
			ECommonUserPrivilegeResult ReturnValue;
		};
		static const UECodeGen_Private::FBytePropertyParams NewProp_Privilege_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Privilege;
		static const UECodeGen_Private::FBytePropertyParams NewProp_Context_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Context;
		static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Privilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Privilege = { "Privilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetCachedPrivilegeResult_Parms, Privilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 2438391218
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Context_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Context = { "Context", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetCachedPrivilegeResult_Parms, Context), Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext, METADATA_PARAMS(0, nullptr) }; // 2596656004
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetCachedPrivilegeResult_Parms, ReturnValue), Z_Construct_UEnum_CommonUser_ECommonUserPrivilegeResult, METADATA_PARAMS(0, nullptr) }; // 1983666156
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Privilege_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Privilege,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Context_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_Context,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_ReturnValue_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the most recently queries result for a specific privilege, will return unknown if never queried */" },
#endif
		{ "CPP_Default_Context", "Game" },
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the most recently queries result for a specific privilege, will return unknown if never queried" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "GetCachedPrivilegeResult", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::CommonUserInfo_eventGetCachedPrivilegeResult_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::CommonUserInfo_eventGetCachedPrivilegeResult_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics
	{
		struct CommonUserInfo_eventGetDebugString_Parms
		{
			FString ReturnValue;
		};
		static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetDebugString_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns an internal debug string for this player */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns an internal debug string for this player" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "GetDebugString", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::CommonUserInfo_eventGetDebugString_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::CommonUserInfo_eventGetDebugString_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_GetDebugString()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_GetDebugString_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics
	{
		struct CommonUserInfo_eventGetNetId_Parms
		{
			ECommonUserOnlineContext Context;
			FUniqueNetIdRepl ReturnValue;
		};
		static const UECodeGen_Private::FBytePropertyParams NewProp_Context_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Context;
		static const UECodeGen_Private::FStructPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::NewProp_Context_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::NewProp_Context = { "Context", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetNetId_Parms, Context), Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext, METADATA_PARAMS(0, nullptr) }; // 2596656004
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetNetId_Parms, ReturnValue), Z_Construct_UScriptStruct_FUniqueNetIdRepl, METADATA_PARAMS(0, nullptr) }; // 3233782123
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::NewProp_Context_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::NewProp_Context,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the net id for the given context */" },
#endif
		{ "CPP_Default_Context", "Game" },
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the net id for the given context" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "GetNetId", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::CommonUserInfo_eventGetNetId_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::CommonUserInfo_eventGetNetId_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_GetNetId()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_GetNetId_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics
	{
		struct CommonUserInfo_eventGetNickname_Parms
		{
			FString ReturnValue;
		};
		static const UECodeGen_Private::FStrPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetNickname_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the user's human readable nickname */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the user's human readable nickname" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "GetNickname", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::CommonUserInfo_eventGetNickname_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::CommonUserInfo_eventGetNickname_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_GetNickname()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_GetNickname_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics
	{
		struct CommonUserInfo_eventGetPrivilegeAvailability_Parms
		{
			ECommonUserPrivilege Privilege;
			ECommonUserAvailability ReturnValue;
		};
		static const UECodeGen_Private::FBytePropertyParams NewProp_Privilege_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Privilege;
		static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_Privilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_Privilege = { "Privilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetPrivilegeAvailability_Parms, Privilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 2438391218
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserInfo_eventGetPrivilegeAvailability_Parms, ReturnValue), Z_Construct_UEnum_CommonUser_ECommonUserAvailability, METADATA_PARAMS(0, nullptr) }; // 4287094436
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_Privilege_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_Privilege,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_ReturnValue_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Ask about the general availability of a feature, this combines cached results with state */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Ask about the general availability of a feature, this combines cached results with state" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "GetPrivilegeAvailability", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::CommonUserInfo_eventGetPrivilegeAvailability_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::CommonUserInfo_eventGetPrivilegeAvailability_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics
	{
		struct CommonUserInfo_eventIsDoingLogin_Parms
		{
			bool ReturnValue;
		};
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserInfo_eventIsDoingLogin_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserInfo_eventIsDoingLogin_Parms), &Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns true if this user is in the middle of logging in */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns true if this user is in the middle of logging in" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "IsDoingLogin", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::CommonUserInfo_eventIsDoingLogin_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::CommonUserInfo_eventIsDoingLogin_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics
	{
		struct CommonUserInfo_eventIsLoggedIn_Parms
		{
			bool ReturnValue;
		};
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserInfo_eventIsLoggedIn_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserInfo_eventIsLoggedIn_Parms), &Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::Function_MetaDataParams[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns true if this user has successfully logged in */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns true if this user has successfully logged in" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserInfo, nullptr, "IsLoggedIn", nullptr, nullptr, Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::CommonUserInfo_eventIsLoggedIn_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::CommonUserInfo_eventIsLoggedIn_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonUserInfo);
	UClass* Z_Construct_UClass_UCommonUserInfo_NoRegister()
	{
		return UCommonUserInfo::StaticClass();
	}
	struct Z_Construct_UClass_UCommonUserInfo_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PrimaryInputDevice_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PrimaryInputDevice;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlatformUser_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PlatformUser;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayerIndex_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bCanBeGuest_MetaData[];
#endif
		static void NewProp_bCanBeGuest_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCanBeGuest;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIsGuest_MetaData[];
#endif
		static void NewProp_bIsGuest_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsGuest;
		static const UECodeGen_Private::FBytePropertyParams NewProp_InitializationState_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_InitializationState_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_InitializationState;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonUserInfo_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UCommonUserInfo_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonUserInfo_GetCachedPrivilegeResult, "GetCachedPrivilegeResult" }, // 3359782027
		{ &Z_Construct_UFunction_UCommonUserInfo_GetDebugString, "GetDebugString" }, // 3270964307
		{ &Z_Construct_UFunction_UCommonUserInfo_GetNetId, "GetNetId" }, // 1807661381
		{ &Z_Construct_UFunction_UCommonUserInfo_GetNickname, "GetNickname" }, // 3997280042
		{ &Z_Construct_UFunction_UCommonUserInfo_GetPrivilegeAvailability, "GetPrivilegeAvailability" }, // 4268676945
		{ &Z_Construct_UFunction_UCommonUserInfo_IsDoingLogin, "IsDoingLogin" }, // 3003709085
		{ &Z_Construct_UFunction_UCommonUserInfo_IsLoggedIn, "IsLoggedIn" }, // 1023080983
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Logical representation of an individual user, one of these will exist for all initialized local players */" },
#endif
		{ "IncludePath", "CommonUserSubsystem.h" },
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Logical representation of an individual user, one of these will exist for all initialized local players" },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PrimaryInputDevice_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Primary controller input device for this user, they could also have additional secondary devices */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Primary controller input device for this user, they could also have additional secondary devices" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PrimaryInputDevice = { "PrimaryInputDevice", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserInfo, PrimaryInputDevice), Z_Construct_UScriptStruct_FInputDeviceId, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PrimaryInputDevice_MetaData), Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PrimaryInputDevice_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PlatformUser_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Specifies the logical user on the local platform, guest users will point to the primary user */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Specifies the logical user on the local platform, guest users will point to the primary user" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PlatformUser = { "PlatformUser", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserInfo, PlatformUser), Z_Construct_UScriptStruct_FPlatformUserId, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PlatformUser_MetaData), Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PlatformUser_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_LocalPlayerIndex_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** If this user is assigned a LocalPlayer, this will match the index in the GameInstance localplayers array once it is fully created */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If this user is assigned a LocalPlayer, this will match the index in the GameInstance localplayers array once it is fully created" },
#endif
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserInfo, LocalPlayerIndex), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_LocalPlayerIndex_MetaData), Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_LocalPlayerIndex_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** If true, this user is allowed to be a guest */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If true, this user is allowed to be a guest" },
#endif
	};
#endif
	void Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest_SetBit(void* Obj)
	{
		((UCommonUserInfo*)Obj)->bCanBeGuest = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest = { "bCanBeGuest", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonUserInfo), &Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest_MetaData), Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** If true, this is a guest user attached to primary user 0 */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If true, this is a guest user attached to primary user 0" },
#endif
	};
#endif
	void Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest_SetBit(void* Obj)
	{
		((UCommonUserInfo*)Obj)->bIsGuest = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest = { "bIsGuest", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UCommonUserInfo), &Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest_MetaData), Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest_MetaData) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Overall state of the user's initialization process */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Overall state of the user's initialization process" },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState = { "InitializationState", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserInfo, InitializationState), Z_Construct_UEnum_CommonUser_ECommonUserInitializationState, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState_MetaData), Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState_MetaData) }; // 2296305739
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonUserInfo_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PrimaryInputDevice,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_PlatformUser,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bCanBeGuest,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_bIsGuest,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserInfo_Statics::NewProp_InitializationState,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonUserInfo_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonUserInfo>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonUserInfo_Statics::ClassParams = {
		&UCommonUserInfo::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UCommonUserInfo_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonUserInfo_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserInfo_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UCommonUserInfo()
	{
		if (!Z_Registration_Info_UClass_UCommonUserInfo.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonUserInfo.OuterSingleton, Z_Construct_UClass_UCommonUserInfo_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonUserInfo.OuterSingleton;
	}
	template<> COMMONUSER_API UClass* StaticClass<UCommonUserInfo>()
	{
		return UCommonUserInfo::StaticClass();
	}
	UCommonUserInfo::UCommonUserInfo(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonUserInfo);
	UCommonUserInfo::~UCommonUserInfo() {}
	struct Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics
	{
		struct _Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms
		{
			const UCommonUserInfo* UserInfo;
			bool bSuccess;
			FText Error;
			ECommonUserPrivilege RequestedPrivilege;
			ECommonUserOnlineContext OnlineContext;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UserInfo_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_UserInfo;
		static void NewProp_bSuccess_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bSuccess;
		static const UECodeGen_Private::FTextPropertyParams NewProp_Error;
		static const UECodeGen_Private::FBytePropertyParams NewProp_RequestedPrivilege_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_RequestedPrivilege;
		static const UECodeGen_Private::FBytePropertyParams NewProp_OnlineContext_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_OnlineContext;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_UserInfo_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_UserInfo = { "UserInfo", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms, UserInfo), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_UserInfo_MetaData), Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_UserInfo_MetaData) };
	void Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_bSuccess_SetBit(void* Obj)
	{
		((_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms*)Obj)->bSuccess = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_bSuccess = { "bSuccess", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms), &Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_bSuccess_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_Error = { "Error", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms, Error), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_RequestedPrivilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_RequestedPrivilege = { "RequestedPrivilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms, RequestedPrivilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 2438391218
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_OnlineContext_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_OnlineContext = { "OnlineContext", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms, OnlineContext), Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext, METADATA_PARAMS(0, nullptr) }; // 2596656004
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_UserInfo,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_bSuccess,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_Error,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_RequestedPrivilege_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_RequestedPrivilege,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_OnlineContext_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::NewProp_OnlineContext,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Delegates when initialization processes succeed or fail */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Delegates when initialization processes succeed or fail" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonUserOnInitializeCompleteMulticast__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms) < MAX_uint16);
	UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature_Statics::FuncParams);
		}
		return ReturnFunction;
	}
void FCommonUserOnInitializeCompleteMulticast_DelegateWrapper(const FMulticastScriptDelegate& CommonUserOnInitializeCompleteMulticast, const UCommonUserInfo* UserInfo, bool bSuccess, const FText& Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	struct _Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms
	{
		const UCommonUserInfo* UserInfo;
		bool bSuccess;
		FText Error;
		ECommonUserPrivilege RequestedPrivilege;
		ECommonUserOnlineContext OnlineContext;
	};
	_Script_CommonUser_eventCommonUserOnInitializeCompleteMulticast_Parms Parms;
	Parms.UserInfo=UserInfo;
	Parms.bSuccess=bSuccess ? true : false;
	Parms.Error=Error;
	Parms.RequestedPrivilege=RequestedPrivilege;
	Parms.OnlineContext=OnlineContext;
	CommonUserOnInitializeCompleteMulticast.ProcessMulticastDelegate<UObject>(&Parms);
}
	struct Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics
	{
		struct _Script_CommonUser_eventCommonUserOnInitializeComplete_Parms
		{
			const UCommonUserInfo* UserInfo;
			bool bSuccess;
			FText Error;
			ECommonUserPrivilege RequestedPrivilege;
			ECommonUserOnlineContext OnlineContext;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UserInfo_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_UserInfo;
		static void NewProp_bSuccess_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bSuccess;
		static const UECodeGen_Private::FTextPropertyParams NewProp_Error;
		static const UECodeGen_Private::FBytePropertyParams NewProp_RequestedPrivilege_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_RequestedPrivilege;
		static const UECodeGen_Private::FBytePropertyParams NewProp_OnlineContext_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_OnlineContext;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_UserInfo_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_UserInfo = { "UserInfo", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms, UserInfo), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_UserInfo_MetaData), Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_UserInfo_MetaData) };
	void Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_bSuccess_SetBit(void* Obj)
	{
		((_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms*)Obj)->bSuccess = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_bSuccess = { "bSuccess", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms), &Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_bSuccess_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_Error = { "Error", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms, Error), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_RequestedPrivilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_RequestedPrivilege = { "RequestedPrivilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms, RequestedPrivilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 2438391218
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_OnlineContext_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_OnlineContext = { "OnlineContext", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms, OnlineContext), Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext, METADATA_PARAMS(0, nullptr) }; // 2596656004
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_UserInfo,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_bSuccess,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_Error,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_RequestedPrivilege_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_RequestedPrivilege,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_OnlineContext_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::NewProp_OnlineContext,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonUserOnInitializeComplete__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00120000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms) < MAX_uint16);
	UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature_Statics::FuncParams);
		}
		return ReturnFunction;
	}
void FCommonUserOnInitializeComplete_DelegateWrapper(const FScriptDelegate& CommonUserOnInitializeComplete, const UCommonUserInfo* UserInfo, bool bSuccess, const FText& Error, ECommonUserPrivilege RequestedPrivilege, ECommonUserOnlineContext OnlineContext)
{
	struct _Script_CommonUser_eventCommonUserOnInitializeComplete_Parms
	{
		const UCommonUserInfo* UserInfo;
		bool bSuccess;
		FText Error;
		ECommonUserPrivilege RequestedPrivilege;
		ECommonUserOnlineContext OnlineContext;
	};
	_Script_CommonUser_eventCommonUserOnInitializeComplete_Parms Parms;
	Parms.UserInfo=UserInfo;
	Parms.bSuccess=bSuccess ? true : false;
	Parms.Error=Error;
	Parms.RequestedPrivilege=RequestedPrivilege;
	Parms.OnlineContext=OnlineContext;
	CommonUserOnInitializeComplete.ProcessDelegate<UObject>(&Parms);
}
	struct Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics
	{
		struct _Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms
		{
			FGameplayTag MessageType;
			FText TitleText;
			FText BodyText;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_MessageType;
		static const UECodeGen_Private::FTextPropertyParams NewProp_TitleText;
		static const UECodeGen_Private::FTextPropertyParams NewProp_BodyText;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::NewProp_MessageType = { "MessageType", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms, MessageType), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 2083603574
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::NewProp_TitleText = { "TitleText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms, TitleText), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::NewProp_BodyText = { "BodyText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms, BodyText), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::NewProp_MessageType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::NewProp_TitleText,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::NewProp_BodyText,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Delegate when a system error message is sent, the game can choose to display it to the user using the type tag */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Delegate when a system error message is sent, the game can choose to display it to the user using the type tag" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonUserHandleSystemMessageDelegate__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms) < MAX_uint16);
	UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature_Statics::FuncParams);
		}
		return ReturnFunction;
	}
void FCommonUserHandleSystemMessageDelegate_DelegateWrapper(const FMulticastScriptDelegate& CommonUserHandleSystemMessageDelegate, FGameplayTag MessageType, const FText& TitleText, const FText& BodyText)
{
	struct _Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms
	{
		FGameplayTag MessageType;
		FText TitleText;
		FText BodyText;
	};
	_Script_CommonUser_eventCommonUserHandleSystemMessageDelegate_Parms Parms;
	Parms.MessageType=MessageType;
	Parms.TitleText=TitleText;
	Parms.BodyText=BodyText;
	CommonUserHandleSystemMessageDelegate.ProcessMulticastDelegate<UObject>(&Parms);
}
	struct Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics
	{
		struct _Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms
		{
			const UCommonUserInfo* UserInfo;
			ECommonUserPrivilege Privilege;
			ECommonUserAvailability OldAvailability;
			ECommonUserAvailability NewAvailability;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UserInfo_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_UserInfo;
		static const UECodeGen_Private::FBytePropertyParams NewProp_Privilege_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_Privilege;
		static const UECodeGen_Private::FBytePropertyParams NewProp_OldAvailability_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_OldAvailability;
		static const UECodeGen_Private::FBytePropertyParams NewProp_NewAvailability_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_NewAvailability;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_UserInfo_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_UserInfo = { "UserInfo", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms, UserInfo), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_UserInfo_MetaData), Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_UserInfo_MetaData) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_Privilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_Privilege = { "Privilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms, Privilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 2438391218
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_OldAvailability_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_OldAvailability = { "OldAvailability", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms, OldAvailability), Z_Construct_UEnum_CommonUser_ECommonUserAvailability, METADATA_PARAMS(0, nullptr) }; // 4287094436
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_NewAvailability_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_NewAvailability = { "NewAvailability", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms, NewAvailability), Z_Construct_UEnum_CommonUser_ECommonUserAvailability, METADATA_PARAMS(0, nullptr) }; // 4287094436
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_UserInfo,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_Privilege_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_Privilege,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_OldAvailability_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_OldAvailability,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_NewAvailability_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::NewProp_NewAvailability,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Delegate when a privilege changes, this can be bound to see if online status/etc changes during gameplay */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Delegate when a privilege changes, this can be bound to see if online status/etc changes during gameplay" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonUser, nullptr, "CommonUserAvailabilityChangedDelegate__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms) < MAX_uint16);
	UFunction* Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature_Statics::FuncParams);
		}
		return ReturnFunction;
	}
void FCommonUserAvailabilityChangedDelegate_DelegateWrapper(const FMulticastScriptDelegate& CommonUserAvailabilityChangedDelegate, const UCommonUserInfo* UserInfo, ECommonUserPrivilege Privilege, ECommonUserAvailability OldAvailability, ECommonUserAvailability NewAvailability)
{
	struct _Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms
	{
		const UCommonUserInfo* UserInfo;
		ECommonUserPrivilege Privilege;
		ECommonUserAvailability OldAvailability;
		ECommonUserAvailability NewAvailability;
	};
	_Script_CommonUser_eventCommonUserAvailabilityChangedDelegate_Parms Parms;
	Parms.UserInfo=UserInfo;
	Parms.Privilege=Privilege;
	Parms.OldAvailability=OldAvailability;
	Parms.NewAvailability=NewAvailability;
	CommonUserAvailabilityChangedDelegate.ProcessMulticastDelegate<UObject>(&Parms);
}
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_CommonUserInitializeParams;
class UScriptStruct* FCommonUserInitializeParams::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_CommonUserInitializeParams.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_CommonUserInitializeParams.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FCommonUserInitializeParams, (UObject*)Z_Construct_UPackage__Script_CommonUser(), TEXT("CommonUserInitializeParams"));
	}
	return Z_Registration_Info_UScriptStruct_CommonUserInitializeParams.OuterSingleton;
}
template<> COMMONUSER_API UScriptStruct* StaticStruct<FCommonUserInitializeParams>()
{
	return FCommonUserInitializeParams::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayerIndex_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ControllerId_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_ControllerId;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PrimaryInputDevice_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PrimaryInputDevice;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PlatformUser_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_PlatformUser;
		static const UECodeGen_Private::FBytePropertyParams NewProp_RequestedPrivilege_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_RequestedPrivilege_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_RequestedPrivilege;
		static const UECodeGen_Private::FBytePropertyParams NewProp_OnlineContext_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OnlineContext_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_OnlineContext;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bCanCreateNewLocalPlayer_MetaData[];
#endif
		static void NewProp_bCanCreateNewLocalPlayer_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCanCreateNewLocalPlayer;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bCanUseGuestLogin_MetaData[];
#endif
		static void NewProp_bCanUseGuestLogin_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCanUseGuestLogin;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bSuppressLoginErrors_MetaData[];
#endif
		static void NewProp_bSuppressLoginErrors_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bSuppressLoginErrors;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OnUserInitializeComplete_MetaData[];
#endif
		static const UECodeGen_Private::FDelegatePropertyParams NewProp_OnUserInitializeComplete;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Parameter struct for initialize functions, this would normally be filled in by wrapper functions like async nodes */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Parameter struct for initialize functions, this would normally be filled in by wrapper functions like async nodes" },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FCommonUserInitializeParams>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_LocalPlayerIndex_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** What local player index to use, can specify one above current if can create player is enabled */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "What local player index to use, can specify one above current if can create player is enabled" },
#endif
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, LocalPlayerIndex), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_LocalPlayerIndex_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_LocalPlayerIndex_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_ControllerId_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Deprecated method of selecting platform user and input device */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Deprecated method of selecting platform user and input device" },
#endif
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_ControllerId = { "ControllerId", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, ControllerId), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_ControllerId_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_ControllerId_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PrimaryInputDevice_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Primary controller input device for this user, they could also have additional secondary devices */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Primary controller input device for this user, they could also have additional secondary devices" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PrimaryInputDevice = { "PrimaryInputDevice", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, PrimaryInputDevice), Z_Construct_UScriptStruct_FInputDeviceId, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PrimaryInputDevice_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PrimaryInputDevice_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PlatformUser_MetaData[] = {
		{ "Category", "UserInfo" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Specifies the logical user on the local platform */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Specifies the logical user on the local platform" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PlatformUser = { "PlatformUser", nullptr, (EPropertyFlags)0x0010000000000014, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, PlatformUser), Z_Construct_UScriptStruct_FPlatformUserId, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PlatformUser_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PlatformUser_MetaData) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Generally either CanPlay or CanPlayOnline, specifies what level of privilege is required */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Generally either CanPlay or CanPlayOnline, specifies what level of privilege is required" },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege = { "RequestedPrivilege", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, RequestedPrivilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege_MetaData) }; // 2438391218
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** What specific online context to log in to, game means to login to all relevant ones */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "What specific online context to log in to, game means to login to all relevant ones" },
#endif
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext = { "OnlineContext", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, OnlineContext), Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext_MetaData) }; // 2596656004
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** True if this is allowed to create a new local player for initial login */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "True if this is allowed to create a new local player for initial login" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer_SetBit(void* Obj)
	{
		((FCommonUserInitializeParams*)Obj)->bCanCreateNewLocalPlayer = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer = { "bCanCreateNewLocalPlayer", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FCommonUserInitializeParams), &Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** True if this player can be a guest user without an actual online presence */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "True if this player can be a guest user without an actual online presence" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin_SetBit(void* Obj)
	{
		((FCommonUserInitializeParams*)Obj)->bCanUseGuestLogin = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin = { "bCanUseGuestLogin", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FCommonUserInitializeParams), &Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** True if we should not show login errors, the game will be responsible for displaying them */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "True if we should not show login errors, the game will be responsible for displaying them" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors_SetBit(void* Obj)
	{
		((FCommonUserInitializeParams*)Obj)->bSuppressLoginErrors = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors = { "bSuppressLoginErrors", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FCommonUserInitializeParams), &Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnUserInitializeComplete_MetaData[] = {
		{ "Category", "Default" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** If bound, call this dynamic delegate at completion of login */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If bound, call this dynamic delegate at completion of login" },
#endif
	};
#endif
	const UECodeGen_Private::FDelegatePropertyParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnUserInitializeComplete = { "OnUserInitializeComplete", nullptr, (EPropertyFlags)0x0010000000080005, UECodeGen_Private::EPropertyGenFlags::Delegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FCommonUserInitializeParams, OnUserInitializeComplete), Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeComplete__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnUserInitializeComplete_MetaData), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnUserInitializeComplete_MetaData) }; // 3405958328
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_ControllerId,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PrimaryInputDevice,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_PlatformUser,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_RequestedPrivilege,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnlineContext,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanCreateNewLocalPlayer,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bCanUseGuestLogin,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_bSuppressLoginErrors,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewProp_OnUserInitializeComplete,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
		nullptr,
		&NewStructOps,
		"CommonUserInitializeParams",
		Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::PropPointers),
		sizeof(FCommonUserInitializeParams),
		alignof(FCommonUserInitializeParams),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000205),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FCommonUserInitializeParams()
	{
		if (!Z_Registration_Info_UScriptStruct_CommonUserInitializeParams.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_CommonUserInitializeParams.InnerSingleton, Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_CommonUserInitializeParams.InnerSingleton;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execShouldWaitForStartInput)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->ShouldWaitForStartInput();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execHasTraitTag)
	{
		P_GET_STRUCT(FGameplayTag,Z_Param_TraitTag);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->HasTraitTag(Z_Param_TraitTag);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execResetUserState)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->ResetUserState();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execTryToLogOutUser)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_LocalPlayerIndex);
		P_GET_UBOOL(Z_Param_bDestroyPlayer);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->TryToLogOutUser(Z_Param_LocalPlayerIndex,Z_Param_bDestroyPlayer);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execCancelUserInitialization)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_LocalPlayerIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->CancelUserInitialization(Z_Param_LocalPlayerIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execListenForLoginKeyInput)
	{
		P_GET_TARRAY(FKey,Z_Param_AnyUserKeys);
		P_GET_TARRAY(FKey,Z_Param_NewUserKeys);
		P_GET_STRUCT(FCommonUserInitializeParams,Z_Param_Params);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->ListenForLoginKeyInput(Z_Param_AnyUserKeys,Z_Param_NewUserKeys,Z_Param_Params);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execTryToInitializeUser)
	{
		P_GET_STRUCT(FCommonUserInitializeParams,Z_Param_Params);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->TryToInitializeUser(Z_Param_Params);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execTryToLoginForOnlinePlay)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_LocalPlayerIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->TryToLoginForOnlinePlay(Z_Param_LocalPlayerIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execTryToInitializeForLocalPlay)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_LocalPlayerIndex);
		P_GET_STRUCT(FInputDeviceId,Z_Param_PrimaryInputDevice);
		P_GET_UBOOL(Z_Param_bCanUseGuestLogin);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=P_THIS->TryToInitializeForLocalPlay(Z_Param_LocalPlayerIndex,Z_Param_PrimaryInputDevice,Z_Param_bCanUseGuestLogin);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetUserInfoForInputDevice)
	{
		P_GET_STRUCT(FInputDeviceId,Z_Param_InputDevice);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(const UCommonUserInfo**)Z_Param__Result=P_THIS->GetUserInfoForInputDevice(Z_Param_InputDevice);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetUserInfoForControllerId)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_ControllerId);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(const UCommonUserInfo**)Z_Param__Result=P_THIS->GetUserInfoForControllerId(Z_Param_ControllerId);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetUserInfoForUniqueNetId)
	{
		P_GET_STRUCT_REF(FUniqueNetIdRepl,Z_Param_Out_NetId);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(const UCommonUserInfo**)Z_Param__Result=P_THIS->GetUserInfoForUniqueNetId(Z_Param_Out_NetId);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetUserInfoForPlatformUser)
	{
		P_GET_STRUCT(FPlatformUserId,Z_Param_PlatformUser);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(const UCommonUserInfo**)Z_Param__Result=P_THIS->GetUserInfoForPlatformUser(Z_Param_PlatformUser);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetUserInfoForPlatformUserIndex)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_PlatformUserIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(const UCommonUserInfo**)Z_Param__Result=P_THIS->GetUserInfoForPlatformUserIndex(Z_Param_PlatformUserIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetUserInfoForLocalPlayerIndex)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_LocalPlayerIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(const UCommonUserInfo**)Z_Param__Result=P_THIS->GetUserInfoForLocalPlayerIndex(Z_Param_LocalPlayerIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetLocalPlayerInitializationState)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_LocalPlayerIndex);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(ECommonUserInitializationState*)Z_Param__Result=P_THIS->GetLocalPlayerInitializationState(Z_Param_LocalPlayerIndex);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetNumLocalPlayers)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(int32*)Z_Param__Result=P_THIS->GetNumLocalPlayers();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execGetMaxLocalPlayers)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(int32*)Z_Param__Result=P_THIS->GetMaxLocalPlayers();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execSetMaxLocalPlayers)
	{
		P_GET_PROPERTY(FIntProperty,Z_Param_InMaxLocalPLayers);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SetMaxLocalPlayers(Z_Param_InMaxLocalPLayers);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUserSubsystem::execSendSystemMessage)
	{
		P_GET_STRUCT(FGameplayTag,Z_Param_MessageType);
		P_GET_PROPERTY(FTextProperty,Z_Param_TitleText);
		P_GET_PROPERTY(FTextProperty,Z_Param_BodyText);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->SendSystemMessage(Z_Param_MessageType,Z_Param_TitleText,Z_Param_BodyText);
		P_NATIVE_END;
	}
	void UCommonUserSubsystem::StaticRegisterNativesUCommonUserSubsystem()
	{
		UClass* Class = UCommonUserSubsystem::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "CancelUserInitialization", &UCommonUserSubsystem::execCancelUserInitialization },
			{ "GetLocalPlayerInitializationState", &UCommonUserSubsystem::execGetLocalPlayerInitializationState },
			{ "GetMaxLocalPlayers", &UCommonUserSubsystem::execGetMaxLocalPlayers },
			{ "GetNumLocalPlayers", &UCommonUserSubsystem::execGetNumLocalPlayers },
			{ "GetUserInfoForControllerId", &UCommonUserSubsystem::execGetUserInfoForControllerId },
			{ "GetUserInfoForInputDevice", &UCommonUserSubsystem::execGetUserInfoForInputDevice },
			{ "GetUserInfoForLocalPlayerIndex", &UCommonUserSubsystem::execGetUserInfoForLocalPlayerIndex },
			{ "GetUserInfoForPlatformUser", &UCommonUserSubsystem::execGetUserInfoForPlatformUser },
			{ "GetUserInfoForPlatformUserIndex", &UCommonUserSubsystem::execGetUserInfoForPlatformUserIndex },
			{ "GetUserInfoForUniqueNetId", &UCommonUserSubsystem::execGetUserInfoForUniqueNetId },
			{ "HasTraitTag", &UCommonUserSubsystem::execHasTraitTag },
			{ "ListenForLoginKeyInput", &UCommonUserSubsystem::execListenForLoginKeyInput },
			{ "ResetUserState", &UCommonUserSubsystem::execResetUserState },
			{ "SendSystemMessage", &UCommonUserSubsystem::execSendSystemMessage },
			{ "SetMaxLocalPlayers", &UCommonUserSubsystem::execSetMaxLocalPlayers },
			{ "ShouldWaitForStartInput", &UCommonUserSubsystem::execShouldWaitForStartInput },
			{ "TryToInitializeForLocalPlay", &UCommonUserSubsystem::execTryToInitializeForLocalPlay },
			{ "TryToInitializeUser", &UCommonUserSubsystem::execTryToInitializeUser },
			{ "TryToLoginForOnlinePlay", &UCommonUserSubsystem::execTryToLoginForOnlinePlay },
			{ "TryToLogOutUser", &UCommonUserSubsystem::execTryToLogOutUser },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics
	{
		struct CommonUserSubsystem_eventCancelUserInitialization_Parms
		{
			int32 LocalPlayerIndex;
			bool ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventCancelUserInitialization_Parms, LocalPlayerIndex), METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventCancelUserInitialization_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventCancelUserInitialization_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Attempts to cancel an in-progress initialization attempt, this may not work on all platforms but will disable callbacks */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Attempts to cancel an in-progress initialization attempt, this may not work on all platforms but will disable callbacks" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "CancelUserInitialization", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::CommonUserSubsystem_eventCancelUserInitialization_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::CommonUserSubsystem_eventCancelUserInitialization_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics
	{
		struct CommonUserSubsystem_eventGetLocalPlayerInitializationState_Parms
		{
			int32 LocalPlayerIndex;
			ECommonUserInitializationState ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
		static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetLocalPlayerInitializationState_Parms, LocalPlayerIndex), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetLocalPlayerInitializationState_Parms, ReturnValue), Z_Construct_UEnum_CommonUser_ECommonUserInitializationState, METADATA_PARAMS(0, nullptr) }; // 2296305739
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::NewProp_ReturnValue_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the state of initializing the specified local player */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the state of initializing the specified local player" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetLocalPlayerInitializationState", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::CommonUserSubsystem_eventGetLocalPlayerInitializationState_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::CommonUserSubsystem_eventGetLocalPlayerInitializationState_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics
	{
		struct CommonUserSubsystem_eventGetMaxLocalPlayers_Parms
		{
			int32 ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetMaxLocalPlayers_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Gets the maximum number of local players */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gets the maximum number of local players" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetMaxLocalPlayers", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::CommonUserSubsystem_eventGetMaxLocalPlayers_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::CommonUserSubsystem_eventGetMaxLocalPlayers_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics
	{
		struct CommonUserSubsystem_eventGetNumLocalPlayers_Parms
		{
			int32 ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetNumLocalPlayers_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Gets the current number of local players, will always be at least 1 */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gets the current number of local players, will always be at least 1" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetNumLocalPlayers", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::CommonUserSubsystem_eventGetNumLocalPlayers_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::CommonUserSubsystem_eventGetNumLocalPlayers_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics
	{
		struct CommonUserSubsystem_eventGetUserInfoForControllerId_Parms
		{
			int32 ControllerId;
			const UCommonUserInfo* ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_ControllerId;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ControllerId = { "ControllerId", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForControllerId_Parms, ControllerId), METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForControllerId_Parms, ReturnValue), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ControllerId,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Deprecated, use InputDeviceId when available */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Deprecated, use InputDeviceId when available" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetUserInfoForControllerId", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::CommonUserSubsystem_eventGetUserInfoForControllerId_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x44020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::CommonUserSubsystem_eventGetUserInfoForControllerId_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics
	{
		struct FInputDeviceId
		{
			int32 InternalId;
		};

		struct CommonUserSubsystem_eventGetUserInfoForInputDevice_Parms
		{
			FInputDeviceId InputDevice;
			const UCommonUserInfo* ReturnValue;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_InputDevice;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_InputDevice = { "InputDevice", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForInputDevice_Parms, InputDevice), Z_Construct_UScriptStruct_FInputDeviceId, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForInputDevice_Parms, ReturnValue), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_InputDevice,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the user info for a given input device. Can return null */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the user info for a given input device. Can return null" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetUserInfoForInputDevice", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::CommonUserSubsystem_eventGetUserInfoForInputDevice_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x44020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::CommonUserSubsystem_eventGetUserInfoForInputDevice_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics
	{
		struct CommonUserSubsystem_eventGetUserInfoForLocalPlayerIndex_Parms
		{
			int32 LocalPlayerIndex;
			const UCommonUserInfo* ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForLocalPlayerIndex_Parms, LocalPlayerIndex), METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForLocalPlayerIndex_Parms, ReturnValue), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the user info for a given local player index in game instance, 0 is always valid in a running game */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the user info for a given local player index in game instance, 0 is always valid in a running game" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetUserInfoForLocalPlayerIndex", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::CommonUserSubsystem_eventGetUserInfoForLocalPlayerIndex_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x44020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::CommonUserSubsystem_eventGetUserInfoForLocalPlayerIndex_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics
	{
		struct FPlatformUserId
		{
			int32 InternalId;
		};

		struct CommonUserSubsystem_eventGetUserInfoForPlatformUser_Parms
		{
			FPlatformUserId PlatformUser;
			const UCommonUserInfo* ReturnValue;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_PlatformUser;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_PlatformUser = { "PlatformUser", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForPlatformUser_Parms, PlatformUser), Z_Construct_UScriptStruct_FPlatformUserId, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForPlatformUser_Parms, ReturnValue), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_PlatformUser,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the primary user info for a given platform user index. Can return null */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the primary user info for a given platform user index. Can return null" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetUserInfoForPlatformUser", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::CommonUserSubsystem_eventGetUserInfoForPlatformUser_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x44020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::CommonUserSubsystem_eventGetUserInfoForPlatformUser_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics
	{
		struct CommonUserSubsystem_eventGetUserInfoForPlatformUserIndex_Parms
		{
			int32 PlatformUserIndex;
			const UCommonUserInfo* ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_PlatformUserIndex;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_PlatformUserIndex = { "PlatformUserIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForPlatformUserIndex_Parms, PlatformUserIndex), METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForPlatformUserIndex_Parms, ReturnValue), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_PlatformUserIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Deprecated, use PlatformUserId when available */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Deprecated, use PlatformUserId when available" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetUserInfoForPlatformUserIndex", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::CommonUserSubsystem_eventGetUserInfoForPlatformUserIndex_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x44020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::CommonUserSubsystem_eventGetUserInfoForPlatformUserIndex_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics
	{
		struct CommonUserSubsystem_eventGetUserInfoForUniqueNetId_Parms
		{
			FUniqueNetIdRepl NetId;
			const UCommonUserInfo* ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_NetId_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_NetId;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_NetId_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_NetId = { "NetId", nullptr, (EPropertyFlags)0x0010000008000182, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForUniqueNetId_Parms, NetId), Z_Construct_UScriptStruct_FUniqueNetIdRepl, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_NetId_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_NetId_MetaData) }; // 3233782123
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000582, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventGetUserInfoForUniqueNetId_Parms, ReturnValue), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_NetId,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Returns the user info for a unique net id. Can return null */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Returns the user info for a unique net id. Can return null" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "GetUserInfoForUniqueNetId", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::CommonUserSubsystem_eventGetUserInfoForUniqueNetId_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x44420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::CommonUserSubsystem_eventGetUserInfoForUniqueNetId_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics
	{
		struct CommonUserSubsystem_eventHasTraitTag_Parms
		{
			FGameplayTag TraitTag;
			bool ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_TraitTag_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_TraitTag;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_TraitTag_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_TraitTag = { "TraitTag", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventHasTraitTag_Parms, TraitTag), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_TraitTag_MetaData), Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_TraitTag_MetaData) }; // 2083603574
	void Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventHasTraitTag_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventHasTraitTag_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_TraitTag,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Checks if a specific platform/feature tag is enabled */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Checks if a specific platform/feature tag is enabled" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "HasTraitTag", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::CommonUserSubsystem_eventHasTraitTag_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::CommonUserSubsystem_eventHasTraitTag_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics
	{
		struct CommonUserSubsystem_eventListenForLoginKeyInput_Parms
		{
			TArray<FKey> AnyUserKeys;
			TArray<FKey> NewUserKeys;
			FCommonUserInitializeParams Params;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_AnyUserKeys_Inner;
		static const UECodeGen_Private::FArrayPropertyParams NewProp_AnyUserKeys;
		static const UECodeGen_Private::FStructPropertyParams NewProp_NewUserKeys_Inner;
		static const UECodeGen_Private::FArrayPropertyParams NewProp_NewUserKeys;
		static const UECodeGen_Private::FStructPropertyParams NewProp_Params;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_AnyUserKeys_Inner = { "AnyUserKeys", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FKey, METADATA_PARAMS(0, nullptr) }; // 46000949
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_AnyUserKeys = { "AnyUserKeys", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventListenForLoginKeyInput_Parms, AnyUserKeys), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) }; // 46000949
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_NewUserKeys_Inner = { "NewUserKeys", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FKey, METADATA_PARAMS(0, nullptr) }; // 46000949
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_NewUserKeys = { "NewUserKeys", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventListenForLoginKeyInput_Parms, NewUserKeys), EArrayPropertyFlags::None, METADATA_PARAMS(0, nullptr) }; // 46000949
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_Params = { "Params", nullptr, (EPropertyFlags)0x0010008000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventListenForLoginKeyInput_Parms, Params), Z_Construct_UScriptStruct_FCommonUserInitializeParams, METADATA_PARAMS(0, nullptr) }; // 4127159929
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_AnyUserKeys_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_AnyUserKeys,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_NewUserKeys_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_NewUserKeys,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::NewProp_Params,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** \n\x09 * Starts the process of listening for user input for new and existing controllers and logging them.\n\x09 * This will insert a key input handler on the active GameViewportClient and is turned off by calling again with empty key arrays.\n\x09 *\n\x09 * @param AnyUserKeys\x09\x09Listen for these keys for any user, even the default user. Set this for an initial press start screen or empty to disable\n\x09 * @param NewUserKeys\x09\x09Listen for these keys for a new user without a player controller. Set this for splitscreen/local multiplayer or empty to disable\n\x09 * @param Params\x09\x09\x09Params passed to TryToInitializeUser after detecting key input\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Starts the process of listening for user input for new and existing controllers and logging them.\nThis will insert a key input handler on the active GameViewportClient and is turned off by calling again with empty key arrays.\n\n@param AnyUserKeys           Listen for these keys for any user, even the default user. Set this for an initial press start screen or empty to disable\n@param NewUserKeys           Listen for these keys for a new user without a player controller. Set this for splitscreen/local multiplayer or empty to disable\n@param Params                        Params passed to TryToInitializeUser after detecting key input" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "ListenForLoginKeyInput", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::CommonUserSubsystem_eventListenForLoginKeyInput_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::CommonUserSubsystem_eventListenForLoginKeyInput_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Resets the login and initialization state when returning to the main menu after an error */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Resets the login and initialization state when returning to the main menu after an error" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "ResetUserState", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics
	{
		struct CommonUserSubsystem_eventSendSystemMessage_Parms
		{
			FGameplayTag MessageType;
			FText TitleText;
			FText BodyText;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_MessageType;
		static const UECodeGen_Private::FTextPropertyParams NewProp_TitleText;
		static const UECodeGen_Private::FTextPropertyParams NewProp_BodyText;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::NewProp_MessageType = { "MessageType", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventSendSystemMessage_Parms, MessageType), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 2083603574
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::NewProp_TitleText = { "TitleText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventSendSystemMessage_Parms, TitleText), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::NewProp_BodyText = { "BodyText", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventSendSystemMessage_Parms, BodyText), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::NewProp_MessageType,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::NewProp_TitleText,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::NewProp_BodyText,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Send a system message via OnHandleSystemMessage */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Send a system message via OnHandleSystemMessage" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "SendSystemMessage", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::CommonUserSubsystem_eventSendSystemMessage_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::CommonUserSubsystem_eventSendSystemMessage_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics
	{
		struct CommonUserSubsystem_eventSetMaxLocalPlayers_Parms
		{
			int32 InMaxLocalPLayers;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_InMaxLocalPLayers;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::NewProp_InMaxLocalPLayers = { "InMaxLocalPLayers", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventSetMaxLocalPlayers_Parms, InMaxLocalPLayers), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::NewProp_InMaxLocalPLayers,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Sets the maximum number of local players, will not destroy existing ones */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Sets the maximum number of local players, will not destroy existing ones" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "SetMaxLocalPlayers", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::CommonUserSubsystem_eventSetMaxLocalPlayers_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::CommonUserSubsystem_eventSetMaxLocalPlayers_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics
	{
		struct CommonUserSubsystem_eventShouldWaitForStartInput_Parms
		{
			bool ReturnValue;
		};
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	void Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventShouldWaitForStartInput_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventShouldWaitForStartInput_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Checks to see if we should display a press start/input confirmation screen at startup. Games can call this or check the trait tags directly */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Checks to see if we should display a press start/input confirmation screen at startup. Games can call this or check the trait tags directly" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "ShouldWaitForStartInput", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::CommonUserSubsystem_eventShouldWaitForStartInput_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::CommonUserSubsystem_eventShouldWaitForStartInput_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics
	{
		struct FInputDeviceId
		{
			int32 InternalId;
		};

		struct CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms
		{
			int32 LocalPlayerIndex;
			FInputDeviceId PrimaryInputDevice;
			bool bCanUseGuestLogin;
			bool ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
		static const UECodeGen_Private::FStructPropertyParams NewProp_PrimaryInputDevice;
		static void NewProp_bCanUseGuestLogin_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCanUseGuestLogin;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms, LocalPlayerIndex), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_PrimaryInputDevice = { "PrimaryInputDevice", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms, PrimaryInputDevice), Z_Construct_UScriptStruct_FInputDeviceId, METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_bCanUseGuestLogin_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms*)Obj)->bCanUseGuestLogin = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_bCanUseGuestLogin = { "bCanUseGuestLogin", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_bCanUseGuestLogin_SetBit, METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_PrimaryInputDevice,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_bCanUseGuestLogin,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Tries to start the process of creating or updating a local player, including logging in and creating a player controller.\n\x09 * When the process has succeeded or failed, it will broadcast the OnUserInitializeComplete delegate.\n\x09 *\n\x09 * @param LocalPlayerIndex\x09""Desired index of LocalPlayer in Game Instance, 0 will be primary player and 1+ for local multiplayer\n\x09 * @param PrimaryInputDevice The physical controller that should be mapped to this user, will use the default device if invalid\n\x09 * @param bCanUseGuestLogin\x09If true, this player can be a guest without a real Unique Net Id\n\x09 *\n\x09 * @returns true if the process was started, false if it failed before properly starting\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Tries to start the process of creating or updating a local player, including logging in and creating a player controller.\nWhen the process has succeeded or failed, it will broadcast the OnUserInitializeComplete delegate.\n\n@param LocalPlayerIndex      Desired index of LocalPlayer in Game Instance, 0 will be primary player and 1+ for local multiplayer\n@param PrimaryInputDevice The physical controller that should be mapped to this user, will use the default device if invalid\n@param bCanUseGuestLogin     If true, this player can be a guest without a real Unique Net Id\n\n@returns true if the process was started, false if it failed before properly starting" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "TryToInitializeForLocalPlay", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::CommonUserSubsystem_eventTryToInitializeForLocalPlay_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics
	{
		struct CommonUserSubsystem_eventTryToInitializeUser_Parms
		{
			FCommonUserInitializeParams Params;
			bool ReturnValue;
		};
		static const UECodeGen_Private::FStructPropertyParams NewProp_Params;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::NewProp_Params = { "Params", nullptr, (EPropertyFlags)0x0010008000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventTryToInitializeUser_Parms, Params), Z_Construct_UScriptStruct_FCommonUserInitializeParams, METADATA_PARAMS(0, nullptr) }; // 4127159929
	void Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventTryToInitializeUser_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventTryToInitializeUser_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::NewProp_Params,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Starts a general user login and initialization process, using the params structure to determine what to log in to.\n\x09 * When the process has succeeded or failed, it will broadcast the OnUserInitializeComplete delegate.\n\x09 * AsyncAction_CommonUserInitialize provides several wrapper functions for using this in an Event graph.\n\x09 *\n\x09 * @returns true if the process was started, false if it failed before properly starting\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Starts a general user login and initialization process, using the params structure to determine what to log in to.\nWhen the process has succeeded or failed, it will broadcast the OnUserInitializeComplete delegate.\nAsyncAction_CommonUserInitialize provides several wrapper functions for using this in an Event graph.\n\n@returns true if the process was started, false if it failed before properly starting" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "TryToInitializeUser", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::CommonUserSubsystem_eventTryToInitializeUser_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::CommonUserSubsystem_eventTryToInitializeUser_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics
	{
		struct CommonUserSubsystem_eventTryToLoginForOnlinePlay_Parms
		{
			int32 LocalPlayerIndex;
			bool ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventTryToLoginForOnlinePlay_Parms, LocalPlayerIndex), METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventTryToLoginForOnlinePlay_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventTryToLoginForOnlinePlay_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Starts the process of taking a locally logged in user and doing a full online login including account permission checks.\n\x09 * When the process has succeeded or failed, it will broadcast the OnUserInitializeComplete delegate.\n\x09 *\n\x09 * @param LocalPlayerIndex\x09Index of existing LocalPlayer in Game Instance\n\x09 *\n\x09 * @returns true if the process was started, false if it failed before properly starting\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Starts the process of taking a locally logged in user and doing a full online login including account permission checks.\nWhen the process has succeeded or failed, it will broadcast the OnUserInitializeComplete delegate.\n\n@param LocalPlayerIndex      Index of existing LocalPlayer in Game Instance\n\n@returns true if the process was started, false if it failed before properly starting" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "TryToLoginForOnlinePlay", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::CommonUserSubsystem_eventTryToLoginForOnlinePlay_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::CommonUserSubsystem_eventTryToLoginForOnlinePlay_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics
	{
		struct CommonUserSubsystem_eventTryToLogOutUser_Parms
		{
			int32 LocalPlayerIndex;
			bool bDestroyPlayer;
			bool ReturnValue;
		};
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalPlayerIndex;
		static void NewProp_bDestroyPlayer_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bDestroyPlayer;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_LocalPlayerIndex = { "LocalPlayerIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUserSubsystem_eventTryToLogOutUser_Parms, LocalPlayerIndex), METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_bDestroyPlayer_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventTryToLogOutUser_Parms*)Obj)->bDestroyPlayer = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_bDestroyPlayer = { "bDestroyPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventTryToLogOutUser_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_bDestroyPlayer_SetBit, METADATA_PARAMS(0, nullptr) };
	void Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUserSubsystem_eventTryToLogOutUser_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUserSubsystem_eventTryToLogOutUser_Parms), &Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_LocalPlayerIndex,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_bDestroyPlayer,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::Function_MetaDataParams[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Logs a player out of any online systems, and optionally destroys the player entirely if it's not the first one */" },
#endif
		{ "CPP_Default_bDestroyPlayer", "false" },
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Logs a player out of any online systems, and optionally destroys the player entirely if it's not the first one" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUserSubsystem, nullptr, "TryToLogOutUser", nullptr, nullptr, Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::CommonUserSubsystem_eventTryToLogOutUser_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::CommonUserSubsystem_eventTryToLogOutUser_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonUserSubsystem);
	UClass* Z_Construct_UClass_UCommonUserSubsystem_NoRegister()
	{
		return UCommonUserSubsystem::StaticClass();
	}
	struct Z_Construct_UClass_UCommonUserSubsystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OnUserInitializeComplete_MetaData[];
#endif
		static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnUserInitializeComplete;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OnHandleSystemMessage_MetaData[];
#endif
		static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnHandleSystemMessage;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_OnUserPrivilegeChanged_MetaData[];
#endif
		static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnUserPrivilegeChanged;
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_LocalUserInfos_ValueProp;
		static const UECodeGen_Private::FIntPropertyParams NewProp_LocalUserInfos_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LocalUserInfos_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_LocalUserInfos;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonUserSubsystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameInstanceSubsystem,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonUser,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UCommonUserSubsystem_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonUserSubsystem_CancelUserInitialization, "CancelUserInitialization" }, // 3823030060
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetLocalPlayerInitializationState, "GetLocalPlayerInitializationState" }, // 2836072318
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetMaxLocalPlayers, "GetMaxLocalPlayers" }, // 2641959514
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetNumLocalPlayers, "GetNumLocalPlayers" }, // 1185885077
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForControllerId, "GetUserInfoForControllerId" }, // 254639370
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForInputDevice, "GetUserInfoForInputDevice" }, // 2980696293
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForLocalPlayerIndex, "GetUserInfoForLocalPlayerIndex" }, // 2868112226
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUser, "GetUserInfoForPlatformUser" }, // 3176618761
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForPlatformUserIndex, "GetUserInfoForPlatformUserIndex" }, // 4262154544
		{ &Z_Construct_UFunction_UCommonUserSubsystem_GetUserInfoForUniqueNetId, "GetUserInfoForUniqueNetId" }, // 241095754
		{ &Z_Construct_UFunction_UCommonUserSubsystem_HasTraitTag, "HasTraitTag" }, // 1272448058
		{ &Z_Construct_UFunction_UCommonUserSubsystem_ListenForLoginKeyInput, "ListenForLoginKeyInput" }, // 2361993283
		{ &Z_Construct_UFunction_UCommonUserSubsystem_ResetUserState, "ResetUserState" }, // 3597512794
		{ &Z_Construct_UFunction_UCommonUserSubsystem_SendSystemMessage, "SendSystemMessage" }, // 1320927803
		{ &Z_Construct_UFunction_UCommonUserSubsystem_SetMaxLocalPlayers, "SetMaxLocalPlayers" }, // 2382448144
		{ &Z_Construct_UFunction_UCommonUserSubsystem_ShouldWaitForStartInput, "ShouldWaitForStartInput" }, // 2274403609
		{ &Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeForLocalPlay, "TryToInitializeForLocalPlay" }, // 1628717021
		{ &Z_Construct_UFunction_UCommonUserSubsystem_TryToInitializeUser, "TryToInitializeUser" }, // 243825377
		{ &Z_Construct_UFunction_UCommonUserSubsystem_TryToLoginForOnlinePlay, "TryToLoginForOnlinePlay" }, // 2540923162
		{ &Z_Construct_UFunction_UCommonUserSubsystem_TryToLogOutUser, "TryToLogOutUser" }, // 4153737471
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserSubsystem_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Game subsystem that handles queries and changes to user identity and login status.\n * One subsystem is created for each game instance and can be accessed from blueprints or C++ code.\n * If a game-specific subclass exists, this base subsystem will not be created.\n */" },
#endif
		{ "IncludePath", "CommonUserSubsystem.h" },
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Game subsystem that handles queries and changes to user identity and login status.\nOne subsystem is created for each game instance and can be accessed from blueprints or C++ code.\nIf a game-specific subclass exists, this base subsystem will not be created." },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserInitializeComplete_MetaData[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** BP delegate called when any requested initialization request completes */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "BP delegate called when any requested initialization request completes" },
#endif
	};
#endif
	const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserInitializeComplete = { "OnUserInitializeComplete", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserSubsystem, OnUserInitializeComplete), Z_Construct_UDelegateFunction_CommonUser_CommonUserOnInitializeCompleteMulticast__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserInitializeComplete_MetaData), Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserInitializeComplete_MetaData) }; // 1983217145
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnHandleSystemMessage_MetaData[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** BP delegate called when the system sends an error/warning message */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "BP delegate called when the system sends an error/warning message" },
#endif
	};
#endif
	const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnHandleSystemMessage = { "OnHandleSystemMessage", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserSubsystem, OnHandleSystemMessage), Z_Construct_UDelegateFunction_CommonUser_CommonUserHandleSystemMessageDelegate__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnHandleSystemMessage_MetaData), Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnHandleSystemMessage_MetaData) }; // 1558336160
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserPrivilegeChanged_MetaData[] = {
		{ "Category", "CommonUser" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** BP delegate called when privilege availability changes for a user  */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "BP delegate called when privilege availability changes for a user" },
#endif
	};
#endif
	const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserPrivilegeChanged = { "OnUserPrivilegeChanged", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserSubsystem, OnUserPrivilegeChanged), Z_Construct_UDelegateFunction_CommonUser_CommonUserAvailabilityChangedDelegate__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserPrivilegeChanged_MetaData), Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserPrivilegeChanged_MetaData) }; // 149506519
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_ValueProp = { "LocalUserInfos", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_Key_KeyProp = { "LocalUserInfos_Key", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Information about each local user, from local player index to user */" },
#endif
		{ "ModuleRelativePath", "Public/CommonUserSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Information about each local user, from local player index to user" },
#endif
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos = { "LocalUserInfos", nullptr, (EPropertyFlags)0x0024080000000000, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonUserSubsystem, LocalUserInfos), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_MetaData), Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonUserSubsystem_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserInitializeComplete,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnHandleSystemMessage,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_OnUserPrivilegeChanged,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonUserSubsystem_Statics::NewProp_LocalUserInfos,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonUserSubsystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonUserSubsystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonUserSubsystem_Statics::ClassParams = {
		&UCommonUserSubsystem::StaticClass,
		"Game",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UCommonUserSubsystem_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::PropPointers),
		0,
		0x009000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonUserSubsystem_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUserSubsystem_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UCommonUserSubsystem()
	{
		if (!Z_Registration_Info_UClass_UCommonUserSubsystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonUserSubsystem.OuterSingleton, Z_Construct_UClass_UCommonUserSubsystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonUserSubsystem.OuterSingleton;
	}
	template<> COMMONUSER_API UClass* StaticClass<UCommonUserSubsystem>()
	{
		return UCommonUserSubsystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonUserSubsystem);
	UCommonUserSubsystem::~UCommonUserSubsystem() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics::ScriptStructInfo[] = {
		{ FCommonUserInitializeParams::StaticStruct, Z_Construct_UScriptStruct_FCommonUserInitializeParams_Statics::NewStructOps, TEXT("CommonUserInitializeParams"), &Z_Registration_Info_UScriptStruct_CommonUserInitializeParams, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FCommonUserInitializeParams), 4127159929U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCommonUserInfo, UCommonUserInfo::StaticClass, TEXT("UCommonUserInfo"), &Z_Registration_Info_UClass_UCommonUserInfo, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonUserInfo), 2121000680U) },
		{ Z_Construct_UClass_UCommonUserSubsystem, UCommonUserSubsystem::StaticClass, TEXT("UCommonUserSubsystem"), &Z_Registration_Info_UClass_UCommonUserSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonUserSubsystem), 3473130572U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_2387559880(TEXT("/Script/CommonUser"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonUser_Source_CommonUser_Public_CommonUserSubsystem_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
