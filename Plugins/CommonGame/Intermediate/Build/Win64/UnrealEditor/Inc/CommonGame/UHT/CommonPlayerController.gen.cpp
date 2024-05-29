// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/CommonPlayerController.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeCommonPlayerController() {}

// Begin Cross Module References
COMMONGAME_API UClass* Z_Construct_UClass_ACommonPlayerController();
COMMONGAME_API UClass* Z_Construct_UClass_ACommonPlayerController_NoRegister();
MODULARGAMEPLAYACTORS_API UClass* Z_Construct_UClass_AModularPlayerController();
UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References

// Begin Class ACommonPlayerController
void ACommonPlayerController::StaticRegisterNativesACommonPlayerController()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(ACommonPlayerController);
UClass* Z_Construct_UClass_ACommonPlayerController_NoRegister()
{
	return ACommonPlayerController::StaticClass();
}
struct Z_Construct_UClass_ACommonPlayerController_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "HideCategories", "Collision Rendering Transformation" },
		{ "IncludePath", "CommonPlayerController.h" },
		{ "ModuleRelativePath", "Public/CommonPlayerController.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<ACommonPlayerController>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_ACommonPlayerController_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_AModularPlayerController,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_ACommonPlayerController_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_ACommonPlayerController_Statics::ClassParams = {
	&ACommonPlayerController::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_ACommonPlayerController_Statics::Class_MetaDataParams), Z_Construct_UClass_ACommonPlayerController_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_ACommonPlayerController()
{
	if (!Z_Registration_Info_UClass_ACommonPlayerController.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_ACommonPlayerController.OuterSingleton, Z_Construct_UClass_ACommonPlayerController_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_ACommonPlayerController.OuterSingleton;
}
template<> COMMONGAME_API UClass* StaticClass<ACommonPlayerController>()
{
	return ACommonPlayerController::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(ACommonPlayerController);
ACommonPlayerController::~ACommonPlayerController() {}
// End Class ACommonPlayerController

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerController_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_ACommonPlayerController, ACommonPlayerController::StaticClass, TEXT("ACommonPlayerController"), &Z_Registration_Info_UClass_ACommonPlayerController, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(ACommonPlayerController), 235574473U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerController_h_3908378734(TEXT("/Script/CommonGame"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerController_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_CommonPlayerController_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
