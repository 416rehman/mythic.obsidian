// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Actions/AsyncAction_ShowConfirmation.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAsyncAction_ShowConfirmation() {}

// Begin Cross Module References
COMMONGAME_API UClass* Z_Construct_UClass_UAsyncAction_ShowConfirmation();
COMMONGAME_API UClass* Z_Construct_UClass_UAsyncAction_ShowConfirmation_NoRegister();
COMMONGAME_API UClass* Z_Construct_UClass_UCommonGameDialogDescriptor_NoRegister();
COMMONGAME_API UEnum* Z_Construct_UEnum_CommonGame_ECommonMessagingResult();
COMMONGAME_API UFunction* Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature();
COREUOBJECT_API UClass* Z_Construct_UClass_UObject_NoRegister();
ENGINE_API UClass* Z_Construct_UClass_UBlueprintAsyncActionBase();
ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer_NoRegister();
UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References

// Begin Delegate FCommonMessagingResultMCDelegate
struct Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics
{
	struct _Script_CommonGame_eventCommonMessagingResultMCDelegate_Parms
	{
		ECommonMessagingResult Result;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_Result_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_Result;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::NewProp_Result_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::NewProp_Result = { "Result", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonGame_eventCommonMessagingResultMCDelegate_Parms, Result), Z_Construct_UEnum_CommonGame_ECommonMessagingResult, METADATA_PARAMS(0, nullptr) }; // 3451816972
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::NewProp_Result_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::NewProp_Result,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonGame, nullptr, "CommonMessagingResultMCDelegate__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::_Script_CommonGame_eventCommonMessagingResultMCDelegate_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::_Script_CommonGame_eventCommonMessagingResultMCDelegate_Parms) < MAX_uint16);
UFunction* Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature_Statics::FuncParams);
	}
	return ReturnFunction;
}
void FCommonMessagingResultMCDelegate_DelegateWrapper(const FMulticastScriptDelegate& CommonMessagingResultMCDelegate, ECommonMessagingResult Result)
{
	struct _Script_CommonGame_eventCommonMessagingResultMCDelegate_Parms
	{
		ECommonMessagingResult Result;
	};
	_Script_CommonGame_eventCommonMessagingResultMCDelegate_Parms Parms;
	Parms.Result=Result;
	CommonMessagingResultMCDelegate.ProcessMulticastDelegate<UObject>(&Parms);
}
// End Delegate FCommonMessagingResultMCDelegate

// Begin Class UAsyncAction_ShowConfirmation Function ShowConfirmationCustom
struct Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics
{
	struct AsyncAction_ShowConfirmation_eventShowConfirmationCustom_Parms
	{
		UObject* InWorldContextObject;
		UCommonGameDialogDescriptor* Descriptor;
		UAsyncAction_ShowConfirmation* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "BlueprintInternalUseOnly", "true" },
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
		{ "WorldContext", "InWorldContextObject" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InWorldContextObject;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Descriptor;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::NewProp_InWorldContextObject = { "InWorldContextObject", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationCustom_Parms, InWorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::NewProp_Descriptor = { "Descriptor", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationCustom_Parms, Descriptor), Z_Construct_UClass_UCommonGameDialogDescriptor_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationCustom_Parms, ReturnValue), Z_Construct_UClass_UAsyncAction_ShowConfirmation_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::NewProp_InWorldContextObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::NewProp_Descriptor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UAsyncAction_ShowConfirmation, nullptr, "ShowConfirmationCustom", nullptr, nullptr, Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::AsyncAction_ShowConfirmation_eventShowConfirmationCustom_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::AsyncAction_ShowConfirmation_eventShowConfirmationCustom_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UAsyncAction_ShowConfirmation::execShowConfirmationCustom)
{
	P_GET_OBJECT(UObject,Z_Param_InWorldContextObject);
	P_GET_OBJECT(UCommonGameDialogDescriptor,Z_Param_Descriptor);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UAsyncAction_ShowConfirmation**)Z_Param__Result=UAsyncAction_ShowConfirmation::ShowConfirmationCustom(Z_Param_InWorldContextObject,Z_Param_Descriptor);
	P_NATIVE_END;
}
// End Class UAsyncAction_ShowConfirmation Function ShowConfirmationCustom

// Begin Class UAsyncAction_ShowConfirmation Function ShowConfirmationOkCancel
struct Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics
{
	struct AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms
	{
		UObject* InWorldContextObject;
		FText Title;
		FText Message;
		UAsyncAction_ShowConfirmation* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "BlueprintInternalUseOnly", "true" },
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
		{ "WorldContext", "InWorldContextObject" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InWorldContextObject;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Title;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Message;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_InWorldContextObject = { "InWorldContextObject", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms, InWorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_Title = { "Title", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms, Title), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_Message = { "Message", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms, Message), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms, ReturnValue), Z_Construct_UClass_UAsyncAction_ShowConfirmation_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_InWorldContextObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_Title,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_Message,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UAsyncAction_ShowConfirmation, nullptr, "ShowConfirmationOkCancel", nullptr, nullptr, Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::AsyncAction_ShowConfirmation_eventShowConfirmationOkCancel_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UAsyncAction_ShowConfirmation::execShowConfirmationOkCancel)
{
	P_GET_OBJECT(UObject,Z_Param_InWorldContextObject);
	P_GET_PROPERTY(FTextProperty,Z_Param_Title);
	P_GET_PROPERTY(FTextProperty,Z_Param_Message);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UAsyncAction_ShowConfirmation**)Z_Param__Result=UAsyncAction_ShowConfirmation::ShowConfirmationOkCancel(Z_Param_InWorldContextObject,Z_Param_Title,Z_Param_Message);
	P_NATIVE_END;
}
// End Class UAsyncAction_ShowConfirmation Function ShowConfirmationOkCancel

// Begin Class UAsyncAction_ShowConfirmation Function ShowConfirmationYesNo
struct Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics
{
	struct AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms
	{
		UObject* InWorldContextObject;
		FText Title;
		FText Message;
		UAsyncAction_ShowConfirmation* ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "BlueprintInternalUseOnly", "true" },
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
		{ "WorldContext", "InWorldContextObject" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InWorldContextObject;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Title;
	static const UECodeGen_Private::FTextPropertyParams NewProp_Message;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_InWorldContextObject = { "InWorldContextObject", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms, InWorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_Title = { "Title", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms, Title), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_Message = { "Message", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms, Message), METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms, ReturnValue), Z_Construct_UClass_UAsyncAction_ShowConfirmation_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_InWorldContextObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_Title,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_Message,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UAsyncAction_ShowConfirmation, nullptr, "ShowConfirmationYesNo", nullptr, nullptr, Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::AsyncAction_ShowConfirmation_eventShowConfirmationYesNo_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(UAsyncAction_ShowConfirmation::execShowConfirmationYesNo)
{
	P_GET_OBJECT(UObject,Z_Param_InWorldContextObject);
	P_GET_PROPERTY(FTextProperty,Z_Param_Title);
	P_GET_PROPERTY(FTextProperty,Z_Param_Message);
	P_FINISH;
	P_NATIVE_BEGIN;
	*(UAsyncAction_ShowConfirmation**)Z_Param__Result=UAsyncAction_ShowConfirmation::ShowConfirmationYesNo(Z_Param_InWorldContextObject,Z_Param_Title,Z_Param_Message);
	P_NATIVE_END;
}
// End Class UAsyncAction_ShowConfirmation Function ShowConfirmationYesNo

// Begin Class UAsyncAction_ShowConfirmation
void UAsyncAction_ShowConfirmation::StaticRegisterNativesUAsyncAction_ShowConfirmation()
{
	UClass* Class = UAsyncAction_ShowConfirmation::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "ShowConfirmationCustom", &UAsyncAction_ShowConfirmation::execShowConfirmationCustom },
		{ "ShowConfirmationOkCancel", &UAsyncAction_ShowConfirmation::execShowConfirmationOkCancel },
		{ "ShowConfirmationYesNo", &UAsyncAction_ShowConfirmation::execShowConfirmationYesNo },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAsyncAction_ShowConfirmation);
UClass* Z_Construct_UClass_UAsyncAction_ShowConfirmation_NoRegister()
{
	return UAsyncAction_ShowConfirmation::StaticClass();
}
struct Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Allows easily triggering an async confirmation dialog in blueprints that you can then wait on the result.\n */" },
#endif
		{ "IncludePath", "Actions/AsyncAction_ShowConfirmation.h" },
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Allows easily triggering an async confirmation dialog in blueprints that you can then wait on the result." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OnResult_MetaData[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WorldContextObject_MetaData[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TargetLocalPlayer_MetaData[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Descriptor_MetaData[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_ShowConfirmation.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_OnResult;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WorldContextObject;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_TargetLocalPlayer;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Descriptor;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationCustom, "ShowConfirmationCustom" }, // 4238422266
		{ &Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationOkCancel, "ShowConfirmationOkCancel" }, // 106423069
		{ &Z_Construct_UFunction_UAsyncAction_ShowConfirmation_ShowConfirmationYesNo, "ShowConfirmationYesNo" }, // 303084914
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAsyncAction_ShowConfirmation>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_OnResult = { "OnResult", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_ShowConfirmation, OnResult), Z_Construct_UDelegateFunction_CommonGame_CommonMessagingResultMCDelegate__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OnResult_MetaData), NewProp_OnResult_MetaData) }; // 2358609143
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_WorldContextObject = { "WorldContextObject", nullptr, (EPropertyFlags)0x0144000000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_ShowConfirmation, WorldContextObject), Z_Construct_UClass_UObject_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WorldContextObject_MetaData), NewProp_WorldContextObject_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_TargetLocalPlayer = { "TargetLocalPlayer", nullptr, (EPropertyFlags)0x0144000000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_ShowConfirmation, TargetLocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TargetLocalPlayer_MetaData), NewProp_TargetLocalPlayer_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_Descriptor = { "Descriptor", nullptr, (EPropertyFlags)0x0144000000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_ShowConfirmation, Descriptor), Z_Construct_UClass_UCommonGameDialogDescriptor_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Descriptor_MetaData), NewProp_Descriptor_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_OnResult,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_WorldContextObject,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_TargetLocalPlayer,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::NewProp_Descriptor,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UBlueprintAsyncActionBase,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::ClassParams = {
	&UAsyncAction_ShowConfirmation::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::PropPointers),
	0,
	0x008000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::Class_MetaDataParams), Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UAsyncAction_ShowConfirmation()
{
	if (!Z_Registration_Info_UClass_UAsyncAction_ShowConfirmation.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAsyncAction_ShowConfirmation.OuterSingleton, Z_Construct_UClass_UAsyncAction_ShowConfirmation_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UAsyncAction_ShowConfirmation.OuterSingleton;
}
template<> COMMONGAME_API UClass* StaticClass<UAsyncAction_ShowConfirmation>()
{
	return UAsyncAction_ShowConfirmation::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UAsyncAction_ShowConfirmation);
UAsyncAction_ShowConfirmation::~UAsyncAction_ShowConfirmation() {}
// End Class UAsyncAction_ShowConfirmation

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UAsyncAction_ShowConfirmation, UAsyncAction_ShowConfirmation::StaticClass, TEXT("UAsyncAction_ShowConfirmation"), &Z_Registration_Info_UClass_UAsyncAction_ShowConfirmation, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAsyncAction_ShowConfirmation), 2629366151U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_1888213245(TEXT("/Script/CommonGame"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_ShowConfirmation_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
