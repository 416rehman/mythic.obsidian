// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PocketCaptureSubsystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePocketCaptureSubsystem() {}
// Cross Module References
	COREUOBJECT_API UClass* Z_Construct_UClass_UClass();
	ENGINE_API UClass* Z_Construct_UClass_UWorldSubsystem();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketCapture_NoRegister();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketCaptureSubsystem();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketCaptureSubsystem_NoRegister();
	UPackage* Z_Construct_UPackage__Script_PocketWorlds();
// End Cross Module References
	DEFINE_FUNCTION(UPocketCaptureSubsystem::execDestroyThumbnailRenderer)
	{
		P_GET_OBJECT(UPocketCapture,Z_Param_ThumbnailRenderer);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->DestroyThumbnailRenderer(Z_Param_ThumbnailRenderer);
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketCaptureSubsystem::execCreateThumbnailRenderer)
	{
		P_GET_OBJECT(UClass,Z_Param_PocketCaptureClass);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UPocketCapture**)Z_Param__Result=P_THIS->CreateThumbnailRenderer(Z_Param_PocketCaptureClass);
		P_NATIVE_END;
	}
	void UPocketCaptureSubsystem::StaticRegisterNativesUPocketCaptureSubsystem()
	{
		UClass* Class = UPocketCaptureSubsystem::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "CreateThumbnailRenderer", &UPocketCaptureSubsystem::execCreateThumbnailRenderer },
			{ "DestroyThumbnailRenderer", &UPocketCaptureSubsystem::execDestroyThumbnailRenderer },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics
	{
		struct PocketCaptureSubsystem_eventCreateThumbnailRenderer_Parms
		{
			TSubclassOf<UPocketCapture>  PocketCaptureClass;
			UPocketCapture* ReturnValue;
		};
		static const UECodeGen_Private::FClassPropertyParams NewProp_PocketCaptureClass;
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FClassPropertyParams Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::NewProp_PocketCaptureClass = { "PocketCaptureClass", nullptr, (EPropertyFlags)0x0014000000000080, UECodeGen_Private::EPropertyGenFlags::Class, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCaptureSubsystem_eventCreateThumbnailRenderer_Parms, PocketCaptureClass), Z_Construct_UClass_UClass, Z_Construct_UClass_UPocketCapture_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCaptureSubsystem_eventCreateThumbnailRenderer_Parms, ReturnValue), Z_Construct_UClass_UPocketCapture_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::NewProp_PocketCaptureClass,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::Function_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// End USubsystem\n" },
#endif
		{ "DeterminesOutputType", "PocketCaptureClass" },
		{ "ModuleRelativePath", "Public/PocketCaptureSubsystem.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "End USubsystem" },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCaptureSubsystem, nullptr, "CreateThumbnailRenderer", nullptr, nullptr, Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::PocketCaptureSubsystem_eventCreateThumbnailRenderer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::PocketCaptureSubsystem_eventCreateThumbnailRenderer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics
	{
		struct PocketCaptureSubsystem_eventDestroyThumbnailRenderer_Parms
		{
			UPocketCapture* ThumbnailRenderer;
		};
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ThumbnailRenderer;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::NewProp_ThumbnailRenderer = { "ThumbnailRenderer", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PocketCaptureSubsystem_eventDestroyThumbnailRenderer_Parms, ThumbnailRenderer), Z_Construct_UClass_UPocketCapture_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::NewProp_ThumbnailRenderer,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketCaptureSubsystem.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketCaptureSubsystem, nullptr, "DestroyThumbnailRenderer", nullptr, nullptr, Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::PocketCaptureSubsystem_eventDestroyThumbnailRenderer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::PocketCaptureSubsystem_eventDestroyThumbnailRenderer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPocketCaptureSubsystem);
	UClass* Z_Construct_UClass_UPocketCaptureSubsystem_NoRegister()
	{
		return UPocketCaptureSubsystem::StaticClass();
	}
	struct Z_Construct_UClass_UPocketCaptureSubsystem_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPocketCaptureSubsystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UWorldSubsystem,
		(UObject* (*)())Z_Construct_UPackage__Script_PocketWorlds,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCaptureSubsystem_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UPocketCaptureSubsystem_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UPocketCaptureSubsystem_CreateThumbnailRenderer, "CreateThumbnailRenderer" }, // 1657568259
		{ &Z_Construct_UFunction_UPocketCaptureSubsystem_DestroyThumbnailRenderer, "DestroyThumbnailRenderer" }, // 2339207058
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCaptureSubsystem_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketCaptureSubsystem_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "IncludePath", "PocketCaptureSubsystem.h" },
		{ "ModuleRelativePath", "Public/PocketCaptureSubsystem.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPocketCaptureSubsystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPocketCaptureSubsystem>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPocketCaptureSubsystem_Statics::ClassParams = {
		&UPocketCaptureSubsystem::StaticClass,
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
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketCaptureSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UPocketCaptureSubsystem_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UPocketCaptureSubsystem()
	{
		if (!Z_Registration_Info_UClass_UPocketCaptureSubsystem.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPocketCaptureSubsystem.OuterSingleton, Z_Construct_UClass_UPocketCaptureSubsystem_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPocketCaptureSubsystem.OuterSingleton;
	}
	template<> POCKETWORLDS_API UClass* StaticClass<UPocketCaptureSubsystem>()
	{
		return UPocketCaptureSubsystem::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPocketCaptureSubsystem);
	UPocketCaptureSubsystem::~UPocketCaptureSubsystem() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCaptureSubsystem_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCaptureSubsystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPocketCaptureSubsystem, UPocketCaptureSubsystem::StaticClass, TEXT("UPocketCaptureSubsystem"), &Z_Registration_Info_UClass_UPocketCaptureSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPocketCaptureSubsystem), 3517885947U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCaptureSubsystem_h_3086746794(TEXT("/Script/PocketWorlds"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCaptureSubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketCaptureSubsystem_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
