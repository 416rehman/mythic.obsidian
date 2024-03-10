// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CommonLoadingScreen/Public/LoadingProcessInterface.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeLoadingProcessInterface() {}
// Cross Module References
	COMMONLOADINGSCREEN_API UClass* Z_Construct_UClass_ULoadingProcessInterface();
	COMMONLOADINGSCREEN_API UClass* Z_Construct_UClass_ULoadingProcessInterface_NoRegister();
	COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
	UPackage* Z_Construct_UPackage__Script_CommonLoadingScreen();
// End Cross Module References
	void ULoadingProcessInterface::StaticRegisterNativesULoadingProcessInterface()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ULoadingProcessInterface);
	UClass* Z_Construct_UClass_ULoadingProcessInterface_NoRegister()
	{
		return ULoadingProcessInterface::StaticClass();
	}
	struct Z_Construct_UClass_ULoadingProcessInterface_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_ULoadingProcessInterface_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonLoadingScreen,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ULoadingProcessInterface_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_ULoadingProcessInterface_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/LoadingProcessInterface.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_ULoadingProcessInterface_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ILoadingProcessInterface>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_ULoadingProcessInterface_Statics::ClassParams = {
		&ULoadingProcessInterface::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x001040A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ULoadingProcessInterface_Statics::Class_MetaDataParams), Z_Construct_UClass_ULoadingProcessInterface_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_ULoadingProcessInterface()
	{
		if (!Z_Registration_Info_UClass_ULoadingProcessInterface.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ULoadingProcessInterface.OuterSingleton, Z_Construct_UClass_ULoadingProcessInterface_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_ULoadingProcessInterface.OuterSingleton;
	}
	template<> COMMONLOADINGSCREEN_API UClass* StaticClass<ULoadingProcessInterface>()
	{
		return ULoadingProcessInterface::StaticClass();
	}
	ULoadingProcessInterface::ULoadingProcessInterface(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(ULoadingProcessInterface);
	ULoadingProcessInterface::~ULoadingProcessInterface() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_ULoadingProcessInterface, ULoadingProcessInterface::StaticClass, TEXT("ULoadingProcessInterface"), &Z_Registration_Info_UClass_ULoadingProcessInterface, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ULoadingProcessInterface), 1612588131U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_1105937894(TEXT("/Script/CommonLoadingScreen"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
