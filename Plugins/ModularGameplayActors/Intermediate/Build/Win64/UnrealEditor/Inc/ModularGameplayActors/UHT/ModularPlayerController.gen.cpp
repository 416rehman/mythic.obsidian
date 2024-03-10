// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ModularGameplayActors/Public/ModularPlayerController.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeModularPlayerController() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_APlayerController();
	MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularPlayerController();
	MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularPlayerController_NoRegister();
	UPackage* Z_Construct_UPackage__Script_ModularGameplayActors();
// End Cross Module References
	void AModularPlayerController::StaticRegisterNativesAModularPlayerController()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AModularPlayerController);
	UClass* Z_Construct_UClass_AModularPlayerController_NoRegister()
	{
		return AModularPlayerController::StaticClass();
	}
	struct Z_Construct_UClass_AModularPlayerController_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AModularPlayerController_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APlayerController,
		(UObject* (*)())Z_Construct_UPackage__Script_ModularGameplayActors,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AModularPlayerController_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AModularPlayerController_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Minimal class that supports extension by game feature plugins */" },
#endif
		{ "HideCategories", "Collision Rendering Transformation" },
		{ "IncludePath", "ModularPlayerController.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/ModularPlayerController.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Minimal class that supports extension by game feature plugins" },
#endif
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AModularPlayerController_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AModularPlayerController>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AModularPlayerController_Statics::ClassParams = {
		&AModularPlayerController::StaticClass,
		"Game",
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
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AModularPlayerController_Statics::Class_MetaDataParams), Z_Construct_UClass_AModularPlayerController_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_AModularPlayerController()
	{
		if (!Z_Registration_Info_UClass_AModularPlayerController.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AModularPlayerController.OuterSingleton, Z_Construct_UClass_AModularPlayerController_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AModularPlayerController.OuterSingleton;
	}
	template<> MODULARGAMEPLAYACTORS_API UClass* StaticClass<AModularPlayerController>()
	{
		return AModularPlayerController::StaticClass();
	}
	AModularPlayerController::AModularPlayerController(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AModularPlayerController);
	AModularPlayerController::~AModularPlayerController() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerController_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerController_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AModularPlayerController, AModularPlayerController::StaticClass, TEXT("AModularPlayerController"), &Z_Registration_Info_UClass_AModularPlayerController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AModularPlayerController), 3171097978U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerController_h_1681805480(TEXT("/Script/ModularGameplayActors"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerController_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerController_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
