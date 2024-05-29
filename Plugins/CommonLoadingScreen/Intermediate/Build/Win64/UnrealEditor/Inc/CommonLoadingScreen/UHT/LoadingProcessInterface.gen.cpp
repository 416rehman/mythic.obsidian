// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "CommonLoadingScreen/Public/LoadingProcessInterface.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeLoadingProcessInterface() {}

// Begin Cross Module References
COMMONLOADINGSCREEN_API UClass* Z_Construct_UClass_ULoadingProcessInterface();
COMMONLOADINGSCREEN_API UClass* Z_Construct_UClass_ULoadingProcessInterface_NoRegister();
COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
UPackage* Z_Construct_UPackage__Script_CommonLoadingScreen();
// End Cross Module References

// Begin Interface ULoadingProcessInterface
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
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/LoadingProcessInterface.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ILoadingProcessInterface>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_ULoadingProcessInterface_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UInterface,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonLoadingScreen,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ULoadingProcessInterface_Statics::DependentSingletons) < 16);
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
// End Interface ULoadingProcessInterface

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ULoadingProcessInterface, ULoadingProcessInterface::StaticClass, TEXT("ULoadingProcessInterface"), &Z_Registration_Info_UClass_ULoadingProcessInterface, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ULoadingProcessInterface), 3535459782U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_2047328559(TEXT("/Script/CommonLoadingScreen"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonLoadingScreen_Source_CommonLoadingScreen_Public_LoadingProcessInterface_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
