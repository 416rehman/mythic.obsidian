// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ModularGameplayActors/Public/ModularAIController.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeModularAIController() {}
// Cross Module References
	AIMODULE_API UClass* Z_Construct_UClass_AAIController();
	MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularAIController();
	MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularAIController_NoRegister();
	UPackage* Z_Construct_UPackage__Script_ModularGameplayActors();
// End Cross Module References
	void AModularAIController::StaticRegisterNativesAModularAIController()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AModularAIController);
	UClass* Z_Construct_UClass_AModularAIController_NoRegister()
	{
		return AModularAIController::StaticClass();
	}
	struct Z_Construct_UClass_AModularAIController_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AModularAIController_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_AAIController,
		(UObject* (*)())Z_Construct_UPackage__Script_ModularGameplayActors,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AModularAIController_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AModularAIController_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Minimal class that supports extension by game feature plugins */" },
#endif
		{ "HideCategories", "Collision Rendering Transformation" },
		{ "IncludePath", "ModularAIController.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/ModularAIController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Minimal class that supports extension by game feature plugins" },
#endif
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AModularAIController_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AModularAIController>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AModularAIController_Statics::ClassParams = {
		&AModularAIController::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x009002A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AModularAIController_Statics::Class_MetaDataParams), Z_Construct_UClass_AModularAIController_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_AModularAIController()
	{
		if (!Z_Registration_Info_UClass_AModularAIController.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AModularAIController.OuterSingleton, Z_Construct_UClass_AModularAIController_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AModularAIController.OuterSingleton;
	}
	template<> MODULARGAMEPLAYACTORS_API UClass* StaticClass<AModularAIController>()
	{
		return AModularAIController::StaticClass();
	}
	AModularAIController::AModularAIController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AModularAIController);
	AModularAIController::~AModularAIController() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularAIController_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularAIController_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AModularAIController, AModularAIController::StaticClass, TEXT("AModularAIController"), &Z_Registration_Info_UClass_AModularAIController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AModularAIController), 1949191473U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularAIController_h_1326555386(TEXT("/Script/ModularGameplayActors"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularAIController_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularAIController_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
