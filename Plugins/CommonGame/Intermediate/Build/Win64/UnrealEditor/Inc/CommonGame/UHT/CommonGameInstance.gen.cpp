// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/CommonGameInstance.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonGameInstance() {}

// Begin Cross Module References
COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameInstance();
COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameInstance_NoRegister();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonSession_SearchResult_NoRegister();
COMMONUSER_API UClass* Z_Construct_UClass_UCommonUserInfo_NoRegister();
COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserAvailability();
COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext();
COMMONUSER_API UEnum* Z_Construct_UEnum_CommonUser_ECommonUserPrivilege();
ENGINE_API UClass* Z_Construct_UClass_UGameInstance();
GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References

// Begin Class UCommonGameInstance Function HandlePrivilegeChanged
struct Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics
{
	struct CommonGameInstance_eventHandlePrivilegeChanged_Parms
	{
		const UCommonUserInfo* UserInfo;
		ECommonUserPrivilege Privilege;
		ECommonUserAvailability OldAvailability;
		ECommonUserAvailability NewAvailability;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonGameInstance.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UserInfo_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_UserInfo;
	static const UECodeGen_Private::FBytePropertyParams NewProp_Privilege_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_Privilege;
	static const UECodeGen_Private::FBytePropertyParams NewProp_OldAvailability_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_OldAvailability;
	static const UECodeGen_Private::FBytePropertyParams NewProp_NewAvailability_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_NewAvailability;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_UserInfo = { "UserInfo", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlePrivilegeChanged_Parms, UserInfo), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UserInfo_MetaData), NewProp_UserInfo_MetaData) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_Privilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_Privilege = { "Privilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlePrivilegeChanged_Parms, Privilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 3165184135
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_OldAvailability_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_OldAvailability = { "OldAvailability", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlePrivilegeChanged_Parms, OldAvailability), Z_Construct_UEnum_CommonUser_ECommonUserAvailability, METADATA_PARAMS(0, nullptr) }; // 3023508109
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_NewAvailability_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_NewAvailability = { "NewAvailability", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlePrivilegeChanged_Parms, NewAvailability), Z_Construct_UEnum_CommonUser_ECommonUserAvailability, METADATA_PARAMS(0, nullptr) }; // 3023508109
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_UserInfo,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_Privilege_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_Privilege,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_OldAvailability_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_OldAvailability,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_NewAvailability_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::NewProp_NewAvailability,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonGameInstance, nullptr, "HandlePrivilegeChanged", nullptr, nullptr, Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::CommonGameInstance_eventHandlePrivilegeChanged_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::CommonGameInstance_eventHandlePrivilegeChanged_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonGameInstance::execHandlePrivilegeChanged)
{
	P_GET_OBJECT(UCommonUserInfo,Z_Param_UserInfo);
	P_GET_ENUM(ECommonUserPrivilege,Z_Param_Privilege);
	P_GET_ENUM(ECommonUserAvailability,Z_Param_OldAvailability);
	P_GET_ENUM(ECommonUserAvailability,Z_Param_NewAvailability);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->HandlePrivilegeChanged(Z_Param_UserInfo,ECommonUserPrivilege(Z_Param_Privilege),ECommonUserAvailability(Z_Param_OldAvailability),ECommonUserAvailability(Z_Param_NewAvailability));
	P_NATIVE_END;
}
// End Class UCommonGameInstance Function HandlePrivilegeChanged

// Begin Class UCommonGameInstance Function HandlerUserInitialized
struct Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics
{
	struct CommonGameInstance_eventHandlerUserInitialized_Parms
	{
		const UCommonUserInfo* UserInfo;
		bool bSuccess;
		FText Error;
		ECommonUserPrivilege RequestedPrivilege;
		ECommonUserOnlineContext OnlineContext;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/CommonGameInstance.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_UserInfo_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_UserInfo;
	static void NewProp_bSuccess_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bSuccess;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Error;
	static const UECodeGen_Private::FBytePropertyParams NewProp_RequestedPrivilege_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_RequestedPrivilege;
	static const UECodeGen_Private::FBytePropertyParams NewProp_OnlineContext_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_OnlineContext;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_UserInfo = { "UserInfo", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlerUserInitialized_Parms, UserInfo), Z_Construct_UClass_UCommonUserInfo_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_UserInfo_MetaData), NewProp_UserInfo_MetaData) };
void Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_bSuccess_SetBit(void* Obj)
{
	((CommonGameInstance_eventHandlerUserInitialized_Parms*)Obj)->bSuccess = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_bSuccess = { "bSuccess", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonGameInstance_eventHandlerUserInitialized_Parms), &Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_bSuccess_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_Error = { "Error", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlerUserInitialized_Parms, Error), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_RequestedPrivilege_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_RequestedPrivilege = { "RequestedPrivilege", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlerUserInitialized_Parms, RequestedPrivilege), Z_Construct_UEnum_CommonUser_ECommonUserPrivilege, METADATA_PARAMS(0, nullptr) }; // 3165184135
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_OnlineContext_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_OnlineContext = { "OnlineContext", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandlerUserInitialized_Parms, OnlineContext), Z_Construct_UEnum_CommonUser_ECommonUserOnlineContext, METADATA_PARAMS(0, nullptr) }; // 3178011620
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_UserInfo,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_bSuccess,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_Error,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_RequestedPrivilege_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_RequestedPrivilege,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_OnlineContext_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::NewProp_OnlineContext,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonGameInstance, nullptr, "HandlerUserInitialized", nullptr, nullptr, Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::CommonGameInstance_eventHandlerUserInitialized_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::CommonGameInstance_eventHandlerUserInitialized_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonGameInstance::execHandlerUserInitialized)
{
	P_GET_OBJECT(UCommonUserInfo,Z_Param_UserInfo);
	P_GET_UBOOL(Z_Param_bSuccess);
	P_GET_PROPERTY(FTextProperty,Z_Param_Error);
	P_GET_ENUM(ECommonUserPrivilege,Z_Param_RequestedPrivilege);
	P_GET_ENUM(ECommonUserOnlineContext,Z_Param_OnlineContext);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->HandlerUserInitialized(Z_Param_UserInfo,Z_Param_bSuccess,Z_Param_Error,ECommonUserPrivilege(Z_Param_RequestedPrivilege),ECommonUserOnlineContext(Z_Param_OnlineContext));
	P_NATIVE_END;
}
// End Class UCommonGameInstance Function HandlerUserInitialized

// Begin Class UCommonGameInstance Function HandleSystemMessage
struct Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics
{
	struct CommonGameInstance_eventHandleSystemMessage_Parms
	{
		FGameplayTag MessageType;
		FText Title;
		FText Message;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Handles errors/warnings from CommonUser, can be overridden per game */" },
#endif
		{ "ModuleRelativePath", "Public/CommonGameInstance.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Handles errors/warnings from CommonUser, can be overridden per game" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_MessageType;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Title;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Message;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::NewProp_MessageType = { "MessageType", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandleSystemMessage_Parms, MessageType), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 1298103297
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::NewProp_Title = { "Title", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandleSystemMessage_Parms, Title), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::NewProp_Message = { "Message", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonGameInstance_eventHandleSystemMessage_Parms, Message), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::NewProp_MessageType,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::NewProp_Title,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::NewProp_Message,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonGameInstance, nullptr, "HandleSystemMessage", nullptr, nullptr, Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::CommonGameInstance_eventHandleSystemMessage_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00020400, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::CommonGameInstance_eventHandleSystemMessage_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UCommonGameInstance::execHandleSystemMessage)
{
	P_GET_STRUCT(FGameplayTag,Z_Param_MessageType);
	P_GET_PROPERTY(FTextProperty,Z_Param_Title);
	P_GET_PROPERTY(FTextProperty,Z_Param_Message);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->HandleSystemMessage(Z_Param_MessageType,Z_Param_Title,Z_Param_Message);
	P_NATIVE_END;
}
// End Class UCommonGameInstance Function HandleSystemMessage

// Begin Class UCommonGameInstance
void UCommonGameInstance::StaticRegisterNativesUCommonGameInstance()
{
	UClass* Class = UCommonGameInstance::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "HandlePrivilegeChanged", &UCommonGameInstance::execHandlePrivilegeChanged },
		{ "HandlerUserInitialized", &UCommonGameInstance::execHandlerUserInitialized },
		{ "HandleSystemMessage", &UCommonGameInstance::execHandleSystemMessage },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonGameInstance);
UClass* Z_Construct_UClass_UCommonGameInstance_NoRegister()
{
	return UCommonGameInstance::StaticClass();
}
struct Z_Construct_UClass_UCommonGameInstance_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "CommonGameInstance.h" },
		{ "ModuleRelativePath", "Public/CommonGameInstance.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RequestedSession_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Session the player has requested to join */" },
#endif
		{ "ModuleRelativePath", "Public/CommonGameInstance.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Session the player has requested to join" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RequestedSession;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonGameInstance_HandlePrivilegeChanged, "HandlePrivilegeChanged" }, // 3604793383
		{ &Z_Construct_UFunction_UCommonGameInstance_HandlerUserInitialized, "HandlerUserInitialized" }, // 4113025410
		{ &Z_Construct_UFunction_UCommonGameInstance_HandleSystemMessage, "HandleSystemMessage" }, // 211936282
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonGameInstance>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UCommonGameInstance_Statics::NewProp_RequestedSession = { "RequestedSession", nullptr, (EPropertyFlags)0x0144000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UCommonGameInstance, RequestedSession), Z_Construct_UClass_UCommonSession_SearchResult_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RequestedSession_MetaData), NewProp_RequestedSession_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UCommonGameInstance_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UCommonGameInstance_Statics::NewProp_RequestedSession,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameInstance_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UCommonGameInstance_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameInstance,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameInstance_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonGameInstance_Statics::ClassParams = {
	&UCommonGameInstance::StaticClass,
	"Game",
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UCommonGameInstance_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameInstance_Statics::PropPointers),
	0,
	0x009000A9u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonGameInstance_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonGameInstance_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UCommonGameInstance()
{
	if (!Z_Registration_Info_UClass_UCommonGameInstance.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonGameInstance.OuterSingleton, Z_Construct_UClass_UCommonGameInstance_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UCommonGameInstance.OuterSingleton;
}
template<> COMMONGAME_API UClass* StaticClass<UCommonGameInstance>()
{
	return UCommonGameInstance::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonGameInstance);
UCommonGameInstance::~UCommonGameInstance() {}
// End Class UCommonGameInstance

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonGameInstance_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UCommonGameInstance, UCommonGameInstance::StaticClass, TEXT("UCommonGameInstance"), &Z_Registration_Info_UClass_UCommonGameInstance, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonGameInstance), 900315804U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonGameInstance_h_1144254567(TEXT("/Script/CommonGame"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonGameInstance_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonGameInstance_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
