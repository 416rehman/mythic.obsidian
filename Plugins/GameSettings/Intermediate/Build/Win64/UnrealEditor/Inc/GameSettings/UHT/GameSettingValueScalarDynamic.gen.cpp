// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingValueScalarDynamic.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingValueScalarDynamic() {}
// Cross Module References
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueScalar();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueScalarDynamic();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingValueScalarDynamic_NoRegister();
	UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References
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
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameSettingValueScalar,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::Class_MetaDataParams[] = {
		{ "IncludePath", "GameSettingValueScalarDynamic.h" },
		{ "ModuleRelativePath", "Public/GameSettingValueScalarDynamic.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGameSettingValueScalarDynamic_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingValueScalarDynamic>::IsAbstract,
	};
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
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingValueScalarDynamic, UGameSettingValueScalarDynamic::StaticClass, TEXT("UGameSettingValueScalarDynamic"), &Z_Registration_Info_UClass_UGameSettingValueScalarDynamic, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingValueScalarDynamic), 394167049U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_4073873737(TEXT("/Script/GameSettings"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingValueScalarDynamic_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
