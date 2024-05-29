// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingValue.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingValue() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValue();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValue_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingValue
void UGameSettingValue::StaticRegisterNativesUGameSettingValue()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingValue);
UClass* Z_Construct_UClass_UGameSettingValue_NoRegister()
{
	return UGameSettingValue::StaticClass();
}
struct Z_Construct_UClass_UGameSettingValue_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * The base class for all settings that are conceptually a value, that can be \n * changed, and thus reset or restored to their initial value.\n */" },
#endif
		{ "IncludePath", "GameSettingValue.h" },
		{ "ModuleRelativePath", "Public/GameSettingValue.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The base class for all settings that are conceptually a value, that can be\nchanged, and thus reset or restored to their initial value." },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingValue>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGameSettingValue_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameSetting,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValue_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingValue_Statics::ClassParams = {
	&UGameSettingValue::StaticClass,
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
	0x001000A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValue_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingValue_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingValue()
{
	if (!Z_Registration_Info_UClass_UGameSettingValue.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingValue.OuterSingleton, Z_Construct_UClass_UGameSettingValue_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingValue.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingValue>()
{
	return UGameSettingValue::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingValue);
UGameSettingValue::~UGameSettingValue() {}
// End Class UGameSettingValue

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingValue, UGameSettingValue::StaticClass, TEXT("UGameSettingValue"), &Z_Registration_Info_UClass_UGameSettingValue, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingValue), 1373387846U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_2601201315(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValue_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
