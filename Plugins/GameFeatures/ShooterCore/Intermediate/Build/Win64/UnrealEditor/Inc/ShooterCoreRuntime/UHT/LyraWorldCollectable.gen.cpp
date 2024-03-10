// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Public/LyraWorldCollectable.h"
#include "Interaction/InteractionOption.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeLyraWorldCollectable() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_AActor();
	MYTHIC_API UClass* Z_Construct_UClass_UInteractableTarget_NoRegister();
	MYTHIC_API UScriptStruct* Z_Construct_UScriptStruct_FInteractionOption();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_ALyraWorldCollectable();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_ALyraWorldCollectable_NoRegister();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	void ALyraWorldCollectable::StaticRegisterNativesALyraWorldCollectable()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ALyraWorldCollectable);
	UClass* Z_Construct_UClass_ALyraWorldCollectable_NoRegister()
	{
		return ALyraWorldCollectable::StaticClass();
	}
	struct Z_Construct_UClass_ALyraWorldCollectable_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Option_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_Option;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FImplementedInterfaceParams InterfaceParams[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ALyraWorldCollectable_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AActor,
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALyraWorldCollectable_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ALyraWorldCollectable_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "LyraWorldCollectable.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/LyraWorldCollectable.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ALyraWorldCollectable_Statics::NewProp_Option_MetaData[] = {
		{ "Category", "LyraWorldCollectable" },
		{ "ModuleRelativePath", "Public/LyraWorldCollectable.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_ALyraWorldCollectable_Statics::NewProp_Option = { "Option", nullptr, (EPropertyFlags)0x0020088000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(ALyraWorldCollectable, Option), Z_Construct_UScriptStruct_FInteractionOption, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ALyraWorldCollectable_Statics::NewProp_Option_MetaData), Z_Construct_UClass_ALyraWorldCollectable_Statics::NewProp_Option_MetaData) }; // 932122258
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_ALyraWorldCollectable_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_ALyraWorldCollectable_Statics::NewProp_Option,
	};
		const UECodeGen_Private::FImplementedInterfaceParams Z_Construct_UClass_ALyraWorldCollectable_Statics::InterfaceParams[] = {
			{ Z_Construct_UClass_UInteractableTarget_NoRegister, (int32)VTABLE_OFFSET(ALyraWorldCollectable, IInteractableTarget), false },  // 368118561
		};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALyraWorldCollectable_Statics::InterfaceParams) < 64);
	const FCppClassTypeInfoStatic Z_Construct_UClass_ALyraWorldCollectable_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ALyraWorldCollectable>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ALyraWorldCollectable_Statics::ClassParams = {
		&ALyraWorldCollectable::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_ALyraWorldCollectable_Statics::PropPointers,
		InterfaceParams,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_ALyraWorldCollectable_Statics::PropPointers),
		UE_ARRAY_COUNT(InterfaceParams),
		0x008000A5u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ALyraWorldCollectable_Statics::Class_MetaDataParams), Z_Construct_UClass_ALyraWorldCollectable_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ALyraWorldCollectable_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_ALyraWorldCollectable()
	{
		if (!Z_Registration_Info_UClass_ALyraWorldCollectable.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ALyraWorldCollectable.OuterSingleton, Z_Construct_UClass_ALyraWorldCollectable_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ALyraWorldCollectable.OuterSingleton;
	}
	template<> SHOOTERCORERUNTIME_API UClass* StaticClass<ALyraWorldCollectable>()
	{
		return ALyraWorldCollectable::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ALyraWorldCollectable);
	ALyraWorldCollectable::~ALyraWorldCollectable() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_LyraWorldCollectable_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_LyraWorldCollectable_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ALyraWorldCollectable, ALyraWorldCollectable::StaticClass, TEXT("ALyraWorldCollectable"), &Z_Registration_Info_UClass_ALyraWorldCollectable, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ALyraWorldCollectable), 545704197U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_LyraWorldCollectable_h_1269499149(TEXT("/Script/ShooterCoreRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_LyraWorldCollectable_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_LyraWorldCollectable_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
