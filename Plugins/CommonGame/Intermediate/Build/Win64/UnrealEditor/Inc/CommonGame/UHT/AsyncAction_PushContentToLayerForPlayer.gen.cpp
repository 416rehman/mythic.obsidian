// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Actions/AsyncAction_PushContentToLayerForPlayer.h"
#include "GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAsyncAction_PushContentToLayerForPlayer() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer();
	COMMONGAME_API UClass* Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_NoRegister();
	COMMONGAME_API UFunction* Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature();
	COMMONUI_API UClass* Z_Construct_UClass_UCommonActivatableWidget_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_APlayerController_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UCancellableAsyncAction();
	GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	struct Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics
	{
		struct _Script_CommonGame_eventPushContentToLayerForPlayerAsyncDelegate_Parms
		{
			UCommonActivatableWidget* UserWidget;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_UserWidget_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_UserWidget;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::NewProp_UserWidget_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::NewProp_UserWidget = { "UserWidget", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(_Script_CommonGame_eventPushContentToLayerForPlayerAsyncDelegate_Parms, UserWidget), Z_Construct_UClass_UCommonActivatableWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::NewProp_UserWidget_MetaData), Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::NewProp_UserWidget_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::NewProp_UserWidget,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_PushContentToLayerForPlayer.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::FuncParams = { (UObject*(*)())Z_Construct_UPackage__Script_CommonGame, nullptr, "PushContentToLayerForPlayerAsyncDelegate__DelegateSignature", nullptr, nullptr, Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::PropPointers), sizeof(Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::_Script_CommonGame_eventPushContentToLayerForPlayerAsyncDelegate_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00130000, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::Function_MetaDataParams), Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::_Script_CommonGame_eventPushContentToLayerForPlayerAsyncDelegate_Parms) < MAX_uint16);
	UFunction* Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature_Statics::FuncParams);
		}
		return ReturnFunction;
	}
void FPushContentToLayerForPlayerAsyncDelegate_DelegateWrapper(const FMulticastScriptDelegate& PushContentToLayerForPlayerAsyncDelegate, UCommonActivatableWidget* UserWidget)
{
	struct _Script_CommonGame_eventPushContentToLayerForPlayerAsyncDelegate_Parms
	{
		UCommonActivatableWidget* UserWidget;
	};
	_Script_CommonGame_eventPushContentToLayerForPlayerAsyncDelegate_Parms Parms;
	Parms.UserWidget=UserWidget;
	PushContentToLayerForPlayerAsyncDelegate.ProcessMulticastDelegate<UObject>(&Parms);
}
	DEFINE_FUNCTION(UAsyncAction_PushContentToLayerForPlayer::execPushContentToLayerForPlayer)
	{
		P_GET_OBJECT(APlayerController,Z_Param_OwningPlayer);
		P_GET_SOFTCLASS(TSoftClassPtr<UCommonActivatableWidget> ,Z_Param_WidgetClass);
		P_GET_STRUCT(FGameplayTag,Z_Param_LayerName);
		P_GET_UBOOL(Z_Param_bSuspendInputUntilComplete);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UAsyncAction_PushContentToLayerForPlayer**)Z_Param__Result=UAsyncAction_PushContentToLayerForPlayer::PushContentToLayerForPlayer(Z_Param_OwningPlayer,Z_Param_WidgetClass,Z_Param_LayerName,Z_Param_bSuspendInputUntilComplete);
		P_NATIVE_END;
	}
	void UAsyncAction_PushContentToLayerForPlayer::StaticRegisterNativesUAsyncAction_PushContentToLayerForPlayer()
	{
		UClass* Class = UAsyncAction_PushContentToLayerForPlayer::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "PushContentToLayerForPlayer", &UAsyncAction_PushContentToLayerForPlayer::execPushContentToLayerForPlayer },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics
	{
		struct AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms
		{
			APlayerController* OwningPlayer;
			TSoftClassPtr<UCommonActivatableWidget>  WidgetClass;
			FGameplayTag LayerName;
			bool bSuspendInputUntilComplete;
			UAsyncAction_PushContentToLayerForPlayer* ReturnValue;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_OwningPlayer;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_WidgetClass_MetaData[];
#endif
		static const UECodeGen_Private::FSoftClassPropertyParams NewProp_WidgetClass;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LayerName_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_LayerName;
		static void NewProp_bSuspendInputUntilComplete_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bSuspendInputUntilComplete;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_OwningPlayer = { "OwningPlayer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms, OwningPlayer), Z_Construct_UClass_APlayerController_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_WidgetClass_MetaData[] = {
		{ "AllowAbstract", "FALSE" },
	};
#endif
	const UECodeGen_Private::FSoftClassPropertyParams Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_WidgetClass = { "WidgetClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::SoftClass, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms, WidgetClass), Z_Construct_UClass_UCommonActivatableWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_WidgetClass_MetaData), Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_WidgetClass_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_LayerName_MetaData[] = {
		{ "Categories", "UI.Layer" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_LayerName = { "LayerName", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms, LayerName), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_LayerName_MetaData), Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_LayerName_MetaData) }; // 2083603574
	void Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_bSuspendInputUntilComplete_SetBit(void* Obj)
	{
		((AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms*)Obj)->bSuspendInputUntilComplete = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_bSuspendInputUntilComplete = { "bSuspendInputUntilComplete", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms), &Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_bSuspendInputUntilComplete_SetBit, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms, ReturnValue), Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_OwningPlayer,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_WidgetClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_LayerName,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_bSuspendInputUntilComplete,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::Function_MetaDataParams[] = {
		{ "BlueprintInternalUseOnly", "true" },
		{ "CPP_Default_bSuspendInputUntilComplete", "true" },
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_PushContentToLayerForPlayer.h" },
		{ "WorldContext", "WorldContextObject" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer, nullptr, "PushContentToLayerForPlayer", nullptr, nullptr, Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04022409, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::AsyncAction_PushContentToLayerForPlayer_eventPushContentToLayerForPlayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAsyncAction_PushContentToLayerForPlayer);
	UClass* Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_NoRegister()
	{
		return UAsyncAction_PushContentToLayerForPlayer::StaticClass();
	}
	struct Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BeforePush_MetaData[];
#endif
		static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_BeforePush;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AfterPush_MetaData[];
#endif
		static const UECodeGen_Private::FMulticastDelegatePropertyParams NewProp_AfterPush;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCancellableAsyncAction,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UAsyncAction_PushContentToLayerForPlayer_PushContentToLayerForPlayer, "PushContentToLayerForPlayer" }, // 2228177215
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Actions/AsyncAction_PushContentToLayerForPlayer.h" },
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_PushContentToLayerForPlayer.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_BeforePush_MetaData[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_PushContentToLayerForPlayer.h" },
	};
#endif
	const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_BeforePush = { "BeforePush", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_PushContentToLayerForPlayer, BeforePush), Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_BeforePush_MetaData), Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_BeforePush_MetaData) }; // 3352260249
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_AfterPush_MetaData[] = {
		{ "ModuleRelativePath", "Public/Actions/AsyncAction_PushContentToLayerForPlayer.h" },
	};
#endif
	const UECodeGen_Private::FMulticastDelegatePropertyParams Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_AfterPush = { "AfterPush", nullptr, (EPropertyFlags)0x0010000010080000, UECodeGen_Private::EPropertyGenFlags::InlineMulticastDelegate, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAsyncAction_PushContentToLayerForPlayer, AfterPush), Z_Construct_UDelegateFunction_CommonGame_PushContentToLayerForPlayerAsyncDelegate__DelegateSignature, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_AfterPush_MetaData), Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_AfterPush_MetaData) }; // 3352260249
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_BeforePush,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::NewProp_AfterPush,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAsyncAction_PushContentToLayerForPlayer>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::ClassParams = {
		&UAsyncAction_PushContentToLayerForPlayer::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::PropPointers),
		0,
		0x009000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::Class_MetaDataParams), Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer()
	{
		if (!Z_Registration_Info_UClass_UAsyncAction_PushContentToLayerForPlayer.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAsyncAction_PushContentToLayerForPlayer.OuterSingleton, Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAsyncAction_PushContentToLayerForPlayer.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UAsyncAction_PushContentToLayerForPlayer>()
	{
		return UAsyncAction_PushContentToLayerForPlayer::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAsyncAction_PushContentToLayerForPlayer);
	UAsyncAction_PushContentToLayerForPlayer::~UAsyncAction_PushContentToLayerForPlayer() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAsyncAction_PushContentToLayerForPlayer, UAsyncAction_PushContentToLayerForPlayer::StaticClass, TEXT("UAsyncAction_PushContentToLayerForPlayer"), &Z_Registration_Info_UClass_UAsyncAction_PushContentToLayerForPlayer, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAsyncAction_PushContentToLayerForPlayer), 3962292022U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_4115083825(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_Actions_AsyncAction_PushContentToLayerForPlayer_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
