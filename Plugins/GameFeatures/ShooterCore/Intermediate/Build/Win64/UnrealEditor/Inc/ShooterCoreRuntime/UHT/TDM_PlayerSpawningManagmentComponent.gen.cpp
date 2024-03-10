// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Private/TDM_PlayerSpawningManagmentComponent.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeTDM_PlayerSpawningManagmentComponent() {}
// Cross Module References
	MYTHIC_API UClass* Z_Construct_UClass_ULyraPlayerSpawningManagerComponent();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_NoRegister();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	void UTDM_PlayerSpawningManagmentComponent::StaticRegisterNativesUTDM_PlayerSpawningManagmentComponent()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UTDM_PlayerSpawningManagmentComponent);
	UClass* Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_NoRegister()
	{
		return UTDM_PlayerSpawningManagmentComponent::StaticClass();
	}
	struct Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_ULyraPlayerSpawningManagerComponent,
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "HideCategories", "Trigger PhysicsVolume" },
		{ "IncludePath", "TDM_PlayerSpawningManagmentComponent.h" },
		{ "ModuleRelativePath", "Private/TDM_PlayerSpawningManagmentComponent.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UTDM_PlayerSpawningManagmentComponent>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::ClassParams = {
		&UTDM_PlayerSpawningManagmentComponent::StaticClass,
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
		0x00A000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::Class_MetaDataParams), Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent()
	{
		if (!Z_Registration_Info_UClass_UTDM_PlayerSpawningManagmentComponent.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UTDM_PlayerSpawningManagmentComponent.OuterSingleton, Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UTDM_PlayerSpawningManagmentComponent.OuterSingleton;
	}
	template<> SHOOTERCORERUNTIME_API UClass* StaticClass<UTDM_PlayerSpawningManagmentComponent>()
	{
		return UTDM_PlayerSpawningManagmentComponent::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UTDM_PlayerSpawningManagmentComponent);
	UTDM_PlayerSpawningManagmentComponent::~UTDM_PlayerSpawningManagmentComponent() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Private_TDM_PlayerSpawningManagmentComponent_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Private_TDM_PlayerSpawningManagmentComponent_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UTDM_PlayerSpawningManagmentComponent, UTDM_PlayerSpawningManagmentComponent::StaticClass, TEXT("UTDM_PlayerSpawningManagmentComponent"), &Z_Registration_Info_UClass_UTDM_PlayerSpawningManagmentComponent, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UTDM_PlayerSpawningManagmentComponent), 776760153U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Private_TDM_PlayerSpawningManagmentComponent_h_1344663160(TEXT("/Script/ShooterCoreRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Private_TDM_PlayerSpawningManagmentComponent_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Private_TDM_PlayerSpawningManagmentComponent_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
