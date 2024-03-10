// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PocketLevelInstance.h"
#include "Public/PocketLevelSystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePocketLevelInstance() {}
// Cross Module References
	COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
	ENGINE_API UClass* Z_Construct_UClass_ULevelStreamingDynamic_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer_NoRegister();
	ENGINE_API UClass* Z_Construct_UClass_UWorld_NoRegister();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevel_NoRegister();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevelInstance();
	POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevelInstance_NoRegister();
	UPackage* Z_Construct_UPackage__Script_PocketWorlds();
// End Cross Module References
	DEFINE_FUNCTION(UPocketLevelInstance::execHandlePocketLevelShown)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->HandlePocketLevelShown();
		P_NATIVE_END;
	}
	DEFINE_FUNCTION(UPocketLevelInstance::execHandlePocketLevelLoaded)
	{
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->HandlePocketLevelLoaded();
		P_NATIVE_END;
	}
	void UPocketLevelInstance::StaticRegisterNativesUPocketLevelInstance()
	{
		UClass* Class = UPocketLevelInstance::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "HandlePocketLevelLoaded", &UPocketLevelInstance::execHandlePocketLevelLoaded },
			{ "HandlePocketLevelShown", &UPocketLevelInstance::execHandlePocketLevelShown },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketLevelInstance, nullptr, "HandlePocketLevelLoaded", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00040401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	struct Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown_Statics::Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPocketLevelInstance, nullptr, "HandlePocketLevelShown", nullptr, nullptr, nullptr, 0, 0, RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x00040401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown_Statics::Function_MetaDataParams) };
	UFunction* Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPocketLevelInstance);
	UClass* Z_Construct_UClass_UPocketLevelInstance_NoRegister()
	{
		return UPocketLevelInstance::StaticClass();
	}
	struct Z_Construct_UClass_UPocketLevelInstance_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LocalPlayer_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_LocalPlayer;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_PocketLevel_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_PocketLevel;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_World_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_World;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_StreamingPocketLevel_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_StreamingPocketLevel;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPocketLevelInstance_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UObject,
		(UObject* (*)())Z_Construct_UPackage__Script_PocketWorlds,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UPocketLevelInstance_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelLoaded, "HandlePocketLevelLoaded" }, // 24741800
		{ &Z_Construct_UFunction_UPocketLevelInstance_HandlePocketLevelShown, "HandlePocketLevelShown" }, // 1653312849
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketLevelInstance_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n *\n */" },
#endif
		{ "IncludePath", "PocketLevelInstance.h" },
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_LocalPlayer_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_LocalPlayer = { "LocalPlayer", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevelInstance, LocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_LocalPlayer_MetaData), Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_LocalPlayer_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_PocketLevel_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_PocketLevel = { "PocketLevel", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevelInstance, PocketLevel), Z_Construct_UClass_UPocketLevel_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_PocketLevel_MetaData), Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_PocketLevel_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_World_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_World = { "World", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevelInstance, World), Z_Construct_UClass_UWorld_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_World_MetaData), Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_World_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_StreamingPocketLevel_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketLevelInstance.h" },
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_StreamingPocketLevel = { "StreamingPocketLevel", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevelInstance, StreamingPocketLevel), Z_Construct_UClass_ULevelStreamingDynamic_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_StreamingPocketLevel_MetaData), Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_StreamingPocketLevel_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPocketLevelInstance_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_LocalPlayer,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_PocketLevel,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_World,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevelInstance_Statics::NewProp_StreamingPocketLevel,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPocketLevelInstance_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPocketLevelInstance>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPocketLevelInstance_Statics::ClassParams = {
		&UPocketLevelInstance::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UPocketLevelInstance_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::Class_MetaDataParams), Z_Construct_UClass_UPocketLevelInstance_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelInstance_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UPocketLevelInstance()
	{
		if (!Z_Registration_Info_UClass_UPocketLevelInstance.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPocketLevelInstance.OuterSingleton, Z_Construct_UClass_UPocketLevelInstance_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPocketLevelInstance.OuterSingleton;
	}
	template<> POCKETWORLDS_API UClass* StaticClass<UPocketLevelInstance>()
	{
		return UPocketLevelInstance::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPocketLevelInstance);
	UPocketLevelInstance::~UPocketLevelInstance() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelInstance_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelInstance_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPocketLevelInstance, UPocketLevelInstance::StaticClass, TEXT("UPocketLevelInstance"), &Z_Registration_Info_UClass_UPocketLevelInstance, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPocketLevelInstance), 1875288159U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelInstance_h_2167594822(TEXT("/Script/PocketWorlds"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelInstance_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelInstance_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
