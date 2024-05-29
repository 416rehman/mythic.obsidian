// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingValueScalarDynamic.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingValueScalarDynamic() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueScalar();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueScalarDynamic();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueScalarDynamic_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingValueScalarDynamic
void UGameSettingValueScalarDynamic::StaticRegisterNativesUGameSettingValueScalarDynamic()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingValueScalarDynamic);
UClass* Z_Construct_UClass_UGameSettingValueScalarDynamic_NoRegister()
{
	return UGameSettingValueScalarDynamic::StaticClass();
}
struct Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "IncludePath", "GameSettingValueScalarDynamic.h" },
		{ "ModuleRelativePath", "Public/GameSettingValueScalarDynamic.h" },
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingValueScalarDynamic>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameSettingValueScalar,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::ClassParams = {
	&UGameSettingValueScalarDynamic::StaticClass,
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
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingValueScalarDynamic()
{
	if (!Z_Registration_Info_UClass_UGameSettingValueScalarDynamic.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingValueScalarDynamic.OuterSingleton, Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingValueScalarDynamic.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingValueScalarDynamic>()
{
	return UGameSettingValueScalarDynamic::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingValueScalarDynamic);
UGameSettingValueScalarDynamic::~UGameSettingValueScalarDynamic() {}
// End Class UGameSettingValueScalarDynamic

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingValueScalarDynamic, UGameSettingValueScalarDynamic::StaticClass, TEXT("UGameSettingValueScalarDynamic"), &Z_Registration_Info_UClass_UGameSettingValueScalarDynamic, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingValueScalarDynamic), 2151772642U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_3580281015(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
