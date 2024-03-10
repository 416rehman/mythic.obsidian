// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CommonLoadingScreen/Public/LoadingScreenManager.h"
#include "../../Source/Runtime/Engine/Classes/Engine/GameInstance.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeLoadingScreenManager() {}
// Cross Module References
	COMMONLOADINGSCREEN_API UClass* Z_Construct_UClass_ULoadingScreenManager();
	COMMONLOADINGSCREEN_API UClass* Z_Construct_UClass_ULoadingScreenManager_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UGameInstanceSubsystem();
	UPackage* Z_Construct_UPackage__Script_CommonLoadingScreen();
// End Cross Module References
	DEFINE_FUNCTION(ULoadingScreenManager::execGetDebugReasonForShowingOrHidingLoadingScreen)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		*(FString*)Z_Param__Result=P_THIS->GetDebugReasonForShowingOrHidingLoadingScreen();
		P_NATIVE_END;
	}
	void ULoadingScreenManager::StaticRegisterNativesULoadingScreenManager()
	{
		UClass* Class = ULoadingScreenManager::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "GetDebugReasonForShowingOrHidingLoadingScreen", &ULoadingScreenManager::execGetDebugReasonForShowingOrHidingLoadingScreen },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics
	{
		struct LoadingScreenManager_eventGetDebugReasonForShowingOrHidingLoadingScreen_Parms
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
	const UECodeGen_Private::FStrPropertyParams Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Str, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(LoadingScreenManager_eventGetDebugReasonForShowingOrHidingLoadingScreen_Parms, ReturnValue), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::Function_MetaDataParams[] = {
		{ "Category", "LoadingScreen" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "//~End of FTickableObjectBase interface\n" },
#endif
		{ "ModuleRelativePath", "Public/LoadingScreenManager.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_ULoadingScreenManager, nullptr, "GetDebugReasonForShowingOrHidingLoadingScreen", nullptr, nullptr, Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::PropPointers), sizeof(Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::LoadingScreenManager_eventGetDebugReasonForShowingOrHidingLoadingScreen_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::Function_MetaDataParams), Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::LoadingScreenManager_eventGetDebugReasonForShowingOrHidingLoadingScreen_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ULoadingScreenManager);
	UClass* Z_Construct_UClass_ULoadingScreenManager_NoRegister()
	{
		return ULoadingScreenManager::StaticClass();
	}
	struct Z_Construct_UClass_ULoadingScreenManager_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ULoadingScreenManager_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameInstanceSubsystem,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonLoadingScreen,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ULoadingScreenManager_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_ULoadingScreenManager_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_ULoadingScreenManager_GetDebugReasonForShowingOrHidingLoadingScreen, "GetDebugReasonForShowingOrHidingLoadingScreen" }, // 123473677
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ULoadingScreenManager_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ULoadingScreenManager_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Handles showing/hiding the loading screen\n */" },
#endif
		{ "IncludePath", "LoadingScreenManager.h" },
		{ "ModuleRelativePath", "Public/LoadingScreenManager.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Handles showing/hiding the loading screen" },
#endif
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ULoadingScreenManager_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ULoadingScreenManager>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ULoadingScreenManager_Statics::ClassParams = {
		&ULoadingScreenManager::StaticClass,
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
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ULoadingScreenManager_Statics::Class_MetaDataParams), Z_Construct_UClass_ULoadingScreenManager_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_ULoadingScreenManager()
	{
		if (!Z_Registration_Info_UClass_ULoadingScreenManager.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ULoadingScreenManager.OuterSingleton, Z_Construct_UClass_ULoadingScreenManager_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ULoadingScreenManager.OuterSingleton;
	}
	template<> COMMONLOADINGSCREEN_API UClass* StaticClass<ULoadingScreenManager>()
	{
		return ULoadingScreenManager::StaticClass();
	}
	ULoadingScreenManager::ULoadingScreenManager() {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ULoadingScreenManager);
	ULoadingScreenManager::~ULoadingScreenManager() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingScreenManager_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingScreenManager_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ULoadingScreenManager, ULoadingScreenManager::StaticClass, TEXT("ULoadingScreenManager"), &Z_Registration_Info_UClass_ULoadingScreenManager, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ULoadingScreenManager), 3855556211U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingScreenManager_h_3648478912(TEXT("/Script/CommonLoadingScreen"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingScreenManager_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingScreenManager_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
