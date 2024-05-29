// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "GameplayMessageRuntime/Public/GameFramework/AsyncAction_ListenForGameplayMessage.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAsyncAction_ListenForGameplayMessage() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
COREUOBJECT_API UClass* Z_Construct_UClass_UScriptStruct();
ENGINE_API UClass* Z_Construct_UClass_UCancellableAsyncAction();
GAMEPLAYMESSAGERUNTIME_API UClass* Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage();
GAMEPLAYMESSAGERUNTIME_API UClass* Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_NoRegister();
GAMEPLAYMESSAGERUNTIME_API UEnum* Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch();
GAMEPLAYMESSAGERUNTIME_API UFunction* Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature();
GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
UPackage* Z_Construct_UPackage__Script_GameplayMessageRuntime();
// End Cross Module References

// Begin Delegate FAsyncGameplayMessageDelegate
struct Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics
{
	struct _Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms
	{
		UAsyncAction_ListenForGameplayMessage* ProxyObject;
		FGameplayTag ActualChannel;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Proxy object pin will be hidden in K2Node_GameplayMessageAsyncAction. Is used to get a reference to the object triggering the delegate for the follow up call of 'GetPayload'.\n *\n * @param ActualChannel\x09\x09The actual message channel that we received Payload from (will always start with Channel, but may be more specific if partial matches were enabled)\n */" },
#endif
		{ "ModuleRelativePath", "Public/GameFramework/AsyncAction_ListenForGameplayMessage.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Proxy object pin will be hidden in K2Node_GameplayMessageAsyncAction. Is used to get a reference to the object triggering the delegate for the follow up call of 'GetPayload'.\n\n@param ActualChannel                The actual message channel that we received Payload from (will always start with Channel, but may be more specific if partial matches were enabled)" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ProxyObject;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ActualChannel;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::NewProp_ProxyObject = { "ProxyObject", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms, ProxyObject), Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::NewProp_ActualChannel = { "ActualChannel", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms, ActualChannel), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 1298103297
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::NewProp_ProxyObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::NewProp_ActualChannel,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_GameplayMessageRuntime, nullptr, "AsyncGameplayMessageDelegate__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::_Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::_Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FAsyncGameplayMessageDelegate_DelegateWrapper(const FMulticastScriptDelegate& AsyncGameplayMessageDelegate, UAsyncAction_ListenForGameplayMessage* ProxyObject, FGameplayTag ActualChannel)
{
	struct _Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms
	{
		UAsyncAction_ListenForGameplayMessage* ProxyObject;
		FGameplayTag ActualChannel;
	};
	_Script_GameplayMessageRuntime_eventAsyncGameplayMessageDelegate_Parms Parms;
	Parms.ProxyObject=ProxyObject;
	Parms.ActualChannel=ActualChannel;
	AsyncGameplayMessageDelegate.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FAsyncGameplayMessageDelegate

// Begin Class UAsyncAction_ListenForGameplayMessage Function GetPayload
struct Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics
{
	struct AsyncAction_ListenForGameplayMessage_eventGetPayload_Parms
	{
		int32 OutPayload;
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Messaging" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Attempt to copy the payload received from the broadcasted gameplay message into the specified wildcard.\n\x09 * The wildcard's type must match the type from the received message.\n\x09 *\n\x09 * @param OutPayload\x09The wildcard reference the payload should be copied into\n\x09 * @return\x09\x09\x09\x09If the copy was a success\n\x09 */" },
#endif
		{ "CustomStructureParam", "OutPayload" },
		{ "CustomThunk", "true" },
		{ "ModuleRelativePath", "Public/GameFramework/AsyncAction_ListenForGameplayMessage.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Attempt to copy the payload received from the broadcasted gameplay message into the specified wildcard.\nThe wildcard's type must match the type from the received message.\n\n@param OutPayload    The wildcard reference the payload should be copied into\n@return                              If the copy was a success" },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FIntPropertyParams NewProp_OutPayload;
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::NewProp_OutPayload = { "OutPayload", nullptr, (EPropertyFlags)0x0010000008000180, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ListenForGameplayMessage_eventGetPayload_Parms, OutPayload), METADATA_PARAMS(0, nullptr) };
void Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((AsyncAction_ListenForGameplayMessage_eventGetPayload_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(AsyncAction_ListenForGameplayMessage_eventGetPayload_Parms), &Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::NewProp_OutPayload,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage, nullptr, "GetPayload", nullptr, nullptr, Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::AsyncAction_ListenForGameplayMessage_eventGetPayload_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04420401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::AsyncAction_ListenForGameplayMessage_eventGetPayload_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload_Statics::FuncParams);
	}
	return ReturnFunction;
}
// End Class UAsyncAction_ListenForGameplayMessage Function GetPayload

// Begin Class UAsyncAction_ListenForGameplayMessage Function ListenForGameplayMessages
struct Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics
{
	struct AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms
	{
		UObject* WorldContextObject;
		FGameplayTag Channel;
		UScriptStruct* PayloadType;
		EGameplayMessageMatch MatchType;
		UAsyncAction_ListenForGameplayMessage* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "BlueprintInternalUseOnly", "true" },
		{ "Category", "Messaging" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Asynchronously waits for a gameplay message to be broadcast on the specified channel.\n\x09 *\n\x09 * @param Channel\x09\x09\x09The message channel to listen for\n\x09 * @param PayloadType\x09\x09The kind of message structure to use (this must match the same type that the sender is broadcasting)\n\x09 * @param MatchType\x09\x09\x09The rule used for matching the channel with broadcasted messages\n\x09 */" },
#endif
		{ "CPP_Default_MatchType", "ExactMatch" },
		{ "ModuleRelativePath", "Public/GameFramework/AsyncAction_ListenForGameplayMessage.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Asynchronously waits for a gameplay message to be broadcast on the specified channel.\n\n@param Channel                       The message channel to listen for\n@param PayloadType           The kind of message structure to use (this must match the same type that the sender is broadcasting)\n@param MatchType                     The rule used for matching the channel with broadcasted messages" },
#endif
		{ "WorldContext", "WorldContextObject" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WorldContextObject;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Channel;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_PayloadType;
	static const UECodeGen_Private::FBytePropertyParams NewProp_MatchType_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_MatchType;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_WorldContextObject = { "WorldContextObject", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms, WorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_Channel = { "Channel", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms, Channel), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 1298103297
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_PayloadType = { "PayloadType", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms, PayloadType), Z_Construct_UClass_UScriptStruct, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_MatchType_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_MatchType = { "MatchType", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms, MatchType), Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch, METADATA_PARAMS(0, nullptr) }; // 1992465379
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms, ReturnValue), Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_WorldContextObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_Channel,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_PayloadType,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_MatchType_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_MatchType,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage, nullptr, "ListenForGameplayMessages", nullptr, nullptr, Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::AsyncAction_ListenForGameplayMessage_eventListenForGameplayMessages_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UAsyncAction_ListenForGameplayMessage::execListenForGameplayMessages)
{
	P_GET_OBJECT(UObject,Z_Param_WorldContextObject);
	P_GET_STRUCT(FGameplayTag,Z_Param_Channel);
	P_GET_OBJECT(UScriptStruct,Z_Param_PayloadType);
	P_GET_ENUM(EGameplayMessageMatch,Z_Param_MatchType);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UAsyncAction_ListenForGameplayMessage**)Z_Param__Result=UAsyncAction_ListenForGameplayMessage::ListenForGameplayMessages(Z_Param_WorldContextObject,Z_Param_Channel,Z_Param_PayloadType,EGameplayMessageMatch(Z_Param_MatchType));
	P_NATIVE_END;
}
// End Class UAsyncAction_ListenForGameplayMessage Function ListenForGameplayMessages

// Begin Class UAsyncAction_ListenForGameplayMessage
void UAsyncAction_ListenForGameplayMessage::StaticRegisterNativesUAsyncAction_ListenForGameplayMessage()
{
	UClass* Class = UAsyncAction_ListenForGameplayMessage::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "GetPayload", &UAsyncAction_ListenForGameplayMessage::execGetPayload },
		{ "ListenForGameplayMessages", &UAsyncAction_ListenForGameplayMessage::execListenForGameplayMessages },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAsyncAction_ListenForGameplayMessage);
UClass* Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_NoRegister()
{
	return UAsyncAction_ListenForGameplayMessage::StaticClass();
}
struct Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "HasDedicatedAsyncNode", "" },
		{ "IncludePath", "GameFramework/AsyncAction_ListenForGameplayMessage.h" },
		{ "ModuleRelativePath", "Public/GameFramework/AsyncAction_ListenForGameplayMessage.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OnMessageReceived_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Called when a message is broadcast on the specified channel. Use GetPayload() to request the message payload. */" },
#endif
		{ "ModuleRelativePath", "Public/GameFramework/AsyncAction_ListenForGameplayMessage.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Called when a message is broadcast on the specified channel. Use GetPayload() to request the message payload." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnMessageReceived;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_GetPayload, "GetPayload" }, // 3639819053
		{ &Z_Construct_UFunction_UAsyncAction_ListenForGameplayMessage_ListenForGameplayMessages, "ListenForGameplayMessages" }, // 4154493433
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAsyncAction_ListenForGameplayMessage>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::NewProp_OnMessageReceived = { "OnMessageReceived", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_ListenForGameplayMessage, OnMessageReceived), Z_Construct_UDelegateFunction_GameplayMessageRuntime_AsyncGameplayMessageDelegate__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OnMessageReceived_MetaData), NewProp_OnMessageReceived_MetaData) }; // 3775955378
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::NewProp_OnMessageReceived,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UCancellableAsyncAction,
	(UObject* (*)())Z_Construct_UPackage__Script_GameplayMessageRuntime,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::ClassParams = {
	&UAsyncAction_ListenForGameplayMessage::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::PropPointers),
	0,
	0x009000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::Class_MetaDataParams), Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage()
{
	if (!Z_Registration_Info_UClass_UAsyncAction_ListenForGameplayMessage.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAsyncAction_ListenForGameplayMessage.OuterSingleton, Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UAsyncAction_ListenForGameplayMessage.OuterSingleton;
}
template<> GAMEPLAYMESSAGERUNTIME_API UClass* StaticClass<UAsyncAction_ListenForGameplayMessage>()
{
	return UAsyncAction_ListenForGameplayMessage::StaticClass();
}
UAsyncAction_ListenForGameplayMessage::UAsyncAction_ListenForGameplayMessage(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UAsyncAction_ListenForGameplayMessage);
UAsyncAction_ListenForGameplayMessage::~UAsyncAction_ListenForGameplayMessage() {}
// End Class UAsyncAction_ListenForGameplayMessage

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_AsyncAction_ListenForGameplayMessage_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UAsyncAction_ListenForGameplayMessage, UAsyncAction_ListenForGameplayMessage::StaticClass, TEXT("UAsyncAction_ListenForGameplayMessage"), &Z_Registration_Info_UClass_UAsyncAction_ListenForGameplayMessage, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAsyncAction_ListenForGameplayMessage), 400500357U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_AsyncAction_ListenForGameplayMessage_h_2542487587(TEXT("/Script/GameplayMessageRuntime"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_AsyncAction_ListenForGameplayMessage_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_AsyncAction_ListenForGameplayMessage_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
