// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/CommonUIExtensions.h"
#include "GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonUIExtensions() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonUIExtensions();
	COMMONGAME_API UClass* Z_Construct_UClass_UCommonUIExtensions_NoRegister();
	COMMONINPUT_API UEnum* Z_Construct_UEnum_CommonInput_ECommonInputType();
	COMMONUI_API UClass* Z_Construct_UClass_UCommonActivatableWidget_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
	ENGINE_API UClass* Z_Construct_UClass_APlayerController_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UBlueprintFunctionLibrary();
	ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer_NoRegister();
	GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
	UMG_API UClass* Z_Construct_UClass_UUserWidget_NoRegister();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	DEFINE_FUNCTION(UCommonUIExtensions::execResumeInputForPlayer)
	{
		P_GET_OBJECT(APlayerController,Z_Param_PlayerController);
		P_GET_PROPERTY(FNameProperty,Z_Param_SuspendToken);
		P_FINISH;
		P_NATIVE_BEGIN;
		UCommonUIExtensions::ResumeInputForPlayer(Z_Param_PlayerController,Z_Param_SuspendToken);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execSuspendInputForPlayer)
	{
		P_GET_OBJECT(APlayerController,Z_Param_PlayerController);
		P_GET_PROPERTY(FNameProperty,Z_Param_SuspendReason);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(FName*)Z_Param__Result=UCommonUIExtensions::SuspendInputForPlayer(Z_Param_PlayerController,Z_Param_SuspendReason);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execGetLocalPlayerFromController)
	{
		P_GET_OBJECT(APlayerController,Z_Param_PlayerController);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(ULocalPlayer**)Z_Param__Result=UCommonUIExtensions::GetLocalPlayerFromController(Z_Param_PlayerController);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execPopContentFromLayer)
	{
		P_GET_OBJECT(UCommonActivatableWidget,Z_Param_ActivatableWidget);
		P_FINISH;
		P_NATIVE_BEGIN;
		UCommonUIExtensions::PopContentFromLayer(Z_Param_ActivatableWidget);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execPushStreamedContentToLayer_ForPlayer)
	{
		P_GET_OBJECT(ULocalPlayer,Z_Param_LocalPlayer);
		P_GET_STRUCT(FGameplayTag,Z_Param_LayerName);
		P_GET_SOFTCLASS(TSoftClassPtr<UCommonActivatableWidget> ,Z_Param_WidgetClass);
		P_FINISH;
		P_NATIVE_BEGIN;
		UCommonUIExtensions::PushStreamedContentToLayer_ForPlayer(Z_Param_LocalPlayer,Z_Param_LayerName,Z_Param_WidgetClass);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execPushContentToLayer_ForPlayer)
	{
		P_GET_OBJECT(ULocalPlayer,Z_Param_LocalPlayer);
		P_GET_STRUCT(FGameplayTag,Z_Param_LayerName);
		P_GET_OBJECT(UClass,Z_Param_WidgetClass);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UCommonActivatableWidget**)Z_Param__Result=UCommonUIExtensions::PushContentToLayer_ForPlayer(Z_Param_LocalPlayer,Z_Param_LayerName,Z_Param_WidgetClass);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execIsOwningPlayerUsingGamepad)
	{
		P_GET_OBJECT(UUserWidget,Z_Param_WidgetContextObject);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=UCommonUIExtensions::IsOwningPlayerUsingGamepad(Z_Param_WidgetContextObject);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execIsOwningPlayerUsingTouch)
	{
		P_GET_OBJECT(UUserWidget,Z_Param_WidgetContextObject);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(bool*)Z_Param__Result=UCommonUIExtensions::IsOwningPlayerUsingTouch(Z_Param_WidgetContextObject);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UCommonUIExtensions::execGetOwningPlayerInputType)
	{
		P_GET_OBJECT(UUserWidget,Z_Param_WidgetContextObject);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(ECommonInputType*)Z_Param__Result=UCommonUIExtensions::GetOwningPlayerInputType(Z_Param_WidgetContextObject);
		P_NATIVE_END;
	}
	void UCommonUIExtensions::StaticRegisterNativesUCommonUIExtensions()
	{
		UClass* Class = UCommonUIExtensions::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "GetLocalPlayerFromController", &UCommonUIExtensions::execGetLocalPlayerFromController },
			{ "GetOwningPlayerInputType", &UCommonUIExtensions::execGetOwningPlayerInputType },
			{ "IsOwningPlayerUsingGamepad", &UCommonUIExtensions::execIsOwningPlayerUsingGamepad },
			{ "IsOwningPlayerUsingTouch", &UCommonUIExtensions::execIsOwningPlayerUsingTouch },
			{ "PopContentFromLayer", &UCommonUIExtensions::execPopContentFromLayer },
			{ "PushContentToLayer_ForPlayer", &UCommonUIExtensions::execPushContentToLayer_ForPlayer },
			{ "PushStreamedContentToLayer_ForPlayer", &UCommonUIExtensions::execPushStreamedContentToLayer_ForPlayer },
			{ "ResumeInputForPlayer", &UCommonUIExtensions::execResumeInputForPlayer },
			{ "SuspendInputForPlayer", &UCommonUIExtensions::execSuspendInputForPlayer },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics
	{
		struct CommonUIExtensions_eventGetLocalPlayerFromController_Parms
		{
			APlayerController* PlayerController;
			ULocalPlayer* ReturnValue;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlayerController;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::NewProp_PlayerController = { "PlayerController", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventGetLocalPlayerFromController_Parms, PlayerController), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventGetLocalPlayerFromController_Parms, ReturnValue), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::NewProp_PlayerController,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "GetLocalPlayerFromController", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::CommonUIExtensions_eventGetLocalPlayerFromController_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::CommonUIExtensions_eventGetLocalPlayerFromController_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics
	{
		struct CommonUIExtensions_eventGetOwningPlayerInputType_Parms
		{
			const UUserWidget* WidgetContextObject;
			ECommonInputType ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WidgetContextObject_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetContextObject;
		static const UECodeGen_Private::FBytePropertyParams NewProp_ReturnValue_Underlying;
		static const UECodeGen_Private::FEnumPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_WidgetContextObject_MetaData[] = {
		{ "EditInline", "true" },
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_WidgetContextObject = { "WidgetContextObject", nullptr, (EPropertyFlags)0x0010000000080082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventGetOwningPlayerInputType_Parms, WidgetContextObject), Z_Construct_UClass_UUserWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_WidgetContextObject_MetaData), Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_WidgetContextObject_MetaData) };
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_ReturnValue_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventGetOwningPlayerInputType_Parms, ReturnValue), Z_Construct_UEnum_CommonInput_ECommonInputType, METADATA_PARAMS(0, nullptr) }; // 2481650305
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_WidgetContextObject,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_ReturnValue_Underlying,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
		{ "WorldContext", "WidgetContextObject" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "GetOwningPlayerInputType", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::CommonUIExtensions_eventGetOwningPlayerInputType_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x14022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::CommonUIExtensions_eventGetOwningPlayerInputType_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics
	{
		struct CommonUIExtensions_eventIsOwningPlayerUsingGamepad_Parms
		{
			const UUserWidget* WidgetContextObject;
			bool ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WidgetContextObject_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetContextObject;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_WidgetContextObject_MetaData[] = {
		{ "EditInline", "true" },
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_WidgetContextObject = { "WidgetContextObject", nullptr, (EPropertyFlags)0x0010000000080082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventIsOwningPlayerUsingGamepad_Parms, WidgetContextObject), Z_Construct_UClass_UUserWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_WidgetContextObject_MetaData), Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_WidgetContextObject_MetaData) };
	void Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUIExtensions_eventIsOwningPlayerUsingGamepad_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUIExtensions_eventIsOwningPlayerUsingGamepad_Parms), &Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_WidgetContextObject,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
		{ "WorldContext", "WidgetContextObject" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "IsOwningPlayerUsingGamepad", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::CommonUIExtensions_eventIsOwningPlayerUsingGamepad_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x14022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::CommonUIExtensions_eventIsOwningPlayerUsingGamepad_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics
	{
		struct CommonUIExtensions_eventIsOwningPlayerUsingTouch_Parms
		{
			const UUserWidget* WidgetContextObject;
			bool ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WidgetContextObject_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_WidgetContextObject;
		static void NewProp_ReturnValue_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_WidgetContextObject_MetaData[] = {
		{ "EditInline", "true" },
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_WidgetContextObject = { "WidgetContextObject", nullptr, (EPropertyFlags)0x0010000000080082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventIsOwningPlayerUsingTouch_Parms, WidgetContextObject), Z_Construct_UClass_UUserWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_WidgetContextObject_MetaData), Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_WidgetContextObject_MetaData) };
	void Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_ReturnValue_SetBit(void* Obj)
	{
		((CommonUIExtensions_eventIsOwningPlayerUsingTouch_Parms*)Obj)->ReturnValue = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(CommonUIExtensions_eventIsOwningPlayerUsingTouch_Parms), &Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_WidgetContextObject,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
		{ "WorldContext", "WidgetContextObject" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "IsOwningPlayerUsingTouch", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::CommonUIExtensions_eventIsOwningPlayerUsingTouch_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x14022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::CommonUIExtensions_eventIsOwningPlayerUsingTouch_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics
	{
		struct CommonUIExtensions_eventPopContentFromLayer_Parms
		{
			UCommonActivatableWidget* ActivatableWidget;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ActivatableWidget_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ActivatableWidget;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::NewProp_ActivatableWidget_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::NewProp_ActivatableWidget = { "ActivatableWidget", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPopContentFromLayer_Parms, ActivatableWidget), Z_Construct_UClass_UCommonActivatableWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::NewProp_ActivatableWidget_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::NewProp_ActivatableWidget_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::NewProp_ActivatableWidget,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "PopContentFromLayer", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::CommonUIExtensions_eventPopContentFromLayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::CommonUIExtensions_eventPopContentFromLayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics
	{
		struct CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms
		{
			const ULocalPlayer* LocalPlayer;
			FGameplayTag LayerName;
			TSubclassOf<UCommonActivatableWidget>  WidgetClass;
			UCommonActivatableWidget* ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayer_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_LocalPlayer;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LayerName_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_LayerName;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WidgetClass_MetaData[];
#endif
		static const UECodeGen_Private::FClassPropertyParams NewProp_WidgetClass;
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
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer = { "LocalPlayer", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms, LocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LayerName_MetaData[] = {
		{ "Categories", "UI.Layer" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LayerName = { "LayerName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms, LayerName), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LayerName_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LayerName_MetaData) }; // 2083603574
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_WidgetClass_MetaData[] = {
		{ "AllowAbstract", "FALSE" },
	};
#endif
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_WidgetClass = { "WidgetClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms, WidgetClass), Z_Construct_UClass_UClass, Z_Construct_UClass_UCommonActivatableWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_WidgetClass_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_WidgetClass_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000080588, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms, ReturnValue), Z_Construct_UClass_UCommonActivatableWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_LayerName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_WidgetClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "PushContentToLayer_ForPlayer", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::CommonUIExtensions_eventPushContentToLayer_ForPlayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics
	{
		struct CommonUIExtensions_eventPushStreamedContentToLayer_ForPlayer_Parms
		{
			const ULocalPlayer* LocalPlayer;
			FGameplayTag LayerName;
			TSoftClassPtr<UCommonActivatableWidget>  WidgetClass;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayer_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_LocalPlayer;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LayerName_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_LayerName;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WidgetClass_MetaData[];
#endif
		static const UECodeGen_Private::FSoftClassPropertyParams NewProp_WidgetClass;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer_MetaData[] = {
		{ "NativeConst", "" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer = { "LocalPlayer", nullptr, (EPropertyFlags)0x0010000000000082, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushStreamedContentToLayer_ForPlayer_Parms, LocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LayerName_MetaData[] = {
		{ "Categories", "UI.Layer" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LayerName = { "LayerName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushStreamedContentToLayer_ForPlayer_Parms, LayerName), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LayerName_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LayerName_MetaData) }; // 2083603574
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_WidgetClass_MetaData[] = {
		{ "AllowAbstract", "FALSE" },
	};
#endif
	const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_WidgetClass = { "WidgetClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventPushStreamedContentToLayer_ForPlayer_Parms, WidgetClass), Z_Construct_UClass_UCommonActivatableWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_WidgetClass_MetaData), Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_WidgetClass_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LocalPlayer,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_LayerName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::NewProp_WidgetClass,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "PushStreamedContentToLayer_ForPlayer", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::CommonUIExtensions_eventPushStreamedContentToLayer_ForPlayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::CommonUIExtensions_eventPushStreamedContentToLayer_ForPlayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics
	{
		struct CommonUIExtensions_eventResumeInputForPlayer_Parms
		{
			APlayerController* PlayerController;
			FName SuspendToken;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlayerController;
		static const UECodeGen_Private::FNamePropertyParams NewProp_SuspendToken;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::NewProp_PlayerController = { "PlayerController", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventResumeInputForPlayer_Parms, PlayerController), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::NewProp_SuspendToken = { "SuspendToken", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventResumeInputForPlayer_Parms, SuspendToken), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::NewProp_PlayerController,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::NewProp_SuspendToken,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "ResumeInputForPlayer", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::CommonUIExtensions_eventResumeInputForPlayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::CommonUIExtensions_eventResumeInputForPlayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics
	{
		struct CommonUIExtensions_eventSuspendInputForPlayer_Parms
		{
			APlayerController* PlayerController;
			FName SuspendReason;
			FName ReturnValue;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_PlayerController;
		static const UECodeGen_Private::FNamePropertyParams NewProp_SuspendReason;
		static const UECodeGen_Private::FNamePropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::NewProp_PlayerController = { "PlayerController", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventSuspendInputForPlayer_Parms, PlayerController), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::NewProp_SuspendReason = { "SuspendReason", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventSuspendInputForPlayer_Parms, SuspendReason), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FNamePropertyParams Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Name, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(CommonUIExtensions_eventSuspendInputForPlayer_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::NewProp_PlayerController,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::NewProp_SuspendReason,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Global UI Extensions" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UCommonUIExtensions, nullptr, "SuspendInputForPlayer", nullptr, nullptr, Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::CommonUIExtensions_eventSuspendInputForPlayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::CommonUIExtensions_eventSuspendInputForPlayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UCommonUIExtensions);
	UClass* Z_Construct_UClass_UCommonUIExtensions_NoRegister()
	{
		return UCommonUIExtensions::StaticClass();
	}
	struct Z_Construct_UClass_UCommonUIExtensions_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UCommonUIExtensions_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UBlueprintFunctionLibrary,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUIExtensions_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UCommonUIExtensions_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UCommonUIExtensions_GetLocalPlayerFromController, "GetLocalPlayerFromController" }, // 2752266331
		{ &Z_Construct_UFunction_UCommonUIExtensions_GetOwningPlayerInputType, "GetOwningPlayerInputType" }, // 1748546587
		{ &Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingGamepad, "IsOwningPlayerUsingGamepad" }, // 2003267299
		{ &Z_Construct_UFunction_UCommonUIExtensions_IsOwningPlayerUsingTouch, "IsOwningPlayerUsingTouch" }, // 1890205810
		{ &Z_Construct_UFunction_UCommonUIExtensions_PopContentFromLayer, "PopContentFromLayer" }, // 3365902623
		{ &Z_Construct_UFunction_UCommonUIExtensions_PushContentToLayer_ForPlayer, "PushContentToLayer_ForPlayer" }, // 2029546158
		{ &Z_Construct_UFunction_UCommonUIExtensions_PushStreamedContentToLayer_ForPlayer, "PushStreamedContentToLayer_ForPlayer" }, // 2677780728
		{ &Z_Construct_UFunction_UCommonUIExtensions_ResumeInputForPlayer, "ResumeInputForPlayer" }, // 705110634
		{ &Z_Construct_UFunction_UCommonUIExtensions_SuspendInputForPlayer, "SuspendInputForPlayer" }, // 853414426
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUIExtensions_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UCommonUIExtensions_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "CommonUIExtensions.h" },
		{ "ModuleRelativePath", "Public/CommonUIExtensions.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UCommonUIExtensions_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UCommonUIExtensions>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UCommonUIExtensions_Statics::ClassParams = {
		&UCommonUIExtensions::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		0,
		0,
		0x001000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UCommonUIExtensions_Statics::Class_MetaDataParams), Z_Construct_UClass_UCommonUIExtensions_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UCommonUIExtensions()
	{
		if (!Z_Registration_Info_UClass_UCommonUIExtensions.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UCommonUIExtensions.OuterSingleton, Z_Construct_UClass_UCommonUIExtensions_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UCommonUIExtensions.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UCommonUIExtensions>()
	{
		return UCommonUIExtensions::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UCommonUIExtensions);
	UCommonUIExtensions::~UCommonUIExtensions() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UCommonUIExtensions, UCommonUIExtensions::StaticClass, TEXT("UCommonUIExtensions"), &Z_Registration_Info_UClass_UCommonUIExtensions, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UCommonUIExtensions), 477736290U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_937895322(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonUIExtensions_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
