// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ModularGameplayActors/Public/ModularPlayerState.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeModularPlayerState() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_APlayerState();
	MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularPlayerState();
	MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularPlayerState_NoRegister();
	UPackage* Z_Construct_UPackage__Script_ModularGameplayActors();
// End Cross Module References
	void AModularPlayerState::StaticRegisterNativesAModularPlayerState()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(AModularPlayerState);
	UClass* Z_Construct_UClass_AModularPlayerState_NoRegister()
	{
		return AModularPlayerState::StaticClass();
	}
	struct Z_Construct_UClass_AModularPlayerState_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_AModularPlayerState_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_APlayerState,
		(UObject* (*)())Z_Construct_UPackage__Script_ModularGameplayActors,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_AModularPlayerState_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_AModularPlayerState_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Minimal class that supports extension by game feature plugins */" },
#endif
		{ "HideCategories", "Input Movement Collision Rendering HLOD WorldPartition DataLayers Transformation" },
		{ "IncludePath", "ModularPlayerState.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/ModularPlayerState.h" },
		{ "ShowCategories", "Input|MouseInput Input|TouchInput" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Minimal class that supports extension by game feature plugins" },
#endif
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_AModularPlayerState_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<AModularPlayerState>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_AModularPlayerState_Statics::ClassParams = {
		&AModularPlayerState::StaticClass,
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
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_AModularPlayerState_Statics::Class_MetaDataParams), Z_Construct_UClass_AModularPlayerState_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_AModularPlayerState()
	{
		if (!Z_Registration_Info_UClass_AModularPlayerState.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_AModularPlayerState.OuterSingleton, Z_Construct_UClass_AModularPlayerState_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_AModularPlayerState.OuterSingleton;
	}
	template<> MODULARGAMEPLAYACTORS_API UClass* StaticClass<AModularPlayerState>()
	{
		return AModularPlayerState::StaticClass();
	}
	AModularPlayerState::AModularPlayerState(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(AModularPlayerState);
	AModularPlayerState::~AModularPlayerState() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerState_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerState_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_AModularPlayerState, AModularPlayerState::StaticClass, TEXT("AModularPlayerState"), &Z_Registration_Info_UClass_AModularPlayerState, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(AModularPlayerState), 2202002100U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerState_h_3503942618(TEXT("/Script/ModularGameplayActors"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerState_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_ModularGameplayActors_Source_ModularGameplayActors_Public_ModularPlayerState_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
