// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "TopDownArenaRuntime/Private/TopDownArenaMovementComponent.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTopDownArenaMovementComponent() {}
// Cross Module References
	MYTHIC_API UClass* Z_Construct_UClass_ULyraCharacterMovementComponent();
	TOPDOWNARENARUNTIME_API UClass* Z_Construct_UClass_UTopDownArenaMovementComponent();
	TOPDOWNARENARUNTIME_API UClass* Z_Construct_UClass_UTopDownArenaMovementComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_TopDownArenaRuntime();
// End Cross Module References
	void UTopDownArenaMovementComponent::StaticRegisterNativesUTopDownArenaMovementComponent()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UTopDownArenaMovementComponent);
	UClass* Z_Construct_UClass_UTopDownArenaMovementComponent_NoRegister()
	{
		return UTopDownArenaMovementComponent::StaticClass();
	}
	struct Z_Construct_UClass_UTopDownArenaMovementComponent_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ULyraCharacterMovementComponent,
		(UObject* (*)())Z_Construct_UPackage__Script_TopDownArenaRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "TopDownArenaMovementComponent.h" },
		{ "ModuleRelativePath", "Private/TopDownArenaMovementComponent.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTopDownArenaMovementComponent>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::ClassParams = {
		&UTopDownArenaMovementComponent::StaticClass,
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
		0x00A000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UTopDownArenaMovementComponent()
	{
		if (!Z_Registration_Info_UClass_UTopDownArenaMovementComponent.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTopDownArenaMovementComponent.OuterSingleton, Z_Construct_UClass_UTopDownArenaMovementComponent_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UTopDownArenaMovementComponent.OuterSingleton;
	}
	template<> TOPDOWNARENARUNTIME_API UClass* StaticClass<UTopDownArenaMovementComponent>()
	{
		return UTopDownArenaMovementComponent::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UTopDownArenaMovementComponent);
	UTopDownArenaMovementComponent::~UTopDownArenaMovementComponent() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaMovementComponent_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaMovementComponent_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UTopDownArenaMovementComponent, UTopDownArenaMovementComponent::StaticClass, TEXT("UTopDownArenaMovementComponent"), &Z_Registration_Info_UClass_UTopDownArenaMovementComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTopDownArenaMovementComponent), 1761265596U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaMovementComponent_h_4207574646(TEXT("/Script/TopDownArenaRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaMovementComponent_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_TopDownArena_Source_TopDownArenaRuntime_Private_TopDownArenaMovementComponent_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
