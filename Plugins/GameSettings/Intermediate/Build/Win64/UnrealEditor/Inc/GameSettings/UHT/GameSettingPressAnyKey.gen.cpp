// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/Misc/GameSettingPressAnyKey.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingPressAnyKey() {}
// Cross Module References
	COMMONUI_API UClass* Z_Construct_UClass_UCommonActivatableWidget();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingPressAnyKey();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingPressAnyKey_NoRegister();
	UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References
	void UGameSettingPressAnyKey::StaticRegisterNativesUGameSettingPressAnyKey()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingPressAnyKey);
	UClass* Z_Construct_UClass_UGameSettingPressAnyKey_NoRegister()
	{
		return UGameSettingPressAnyKey::StaticClass();
	}
	struct Z_Construct_UClass_UGameSettingPressAnyKey_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGameSettingPressAnyKey_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCommonActivatableWidget,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingPressAnyKey_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameSettingPressAnyKey_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Widgets/Misc/GameSettingPressAnyKey.h" },
		{ "ModuleRelativePath", "Public/Widgets/Misc/GameSettingPressAnyKey.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGameSettingPressAnyKey_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingPressAnyKey>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingPressAnyKey_Statics::ClassParams = {
		&UGameSettingPressAnyKey::StaticClass,
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
		0x00B010A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingPressAnyKey_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingPressAnyKey_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UGameSettingPressAnyKey()
	{
		if (!Z_Registration_Info_UClass_UGameSettingPressAnyKey.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingPressAnyKey.OuterSingleton, Z_Construct_UClass_UGameSettingPressAnyKey_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGameSettingPressAnyKey.OuterSingleton;
	}
	template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingPressAnyKey>()
	{
		return UGameSettingPressAnyKey::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingPressAnyKey);
	UGameSettingPressAnyKey::~UGameSettingPressAnyKey() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingPressAnyKey_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingPressAnyKey_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingPressAnyKey, UGameSettingPressAnyKey::StaticClass, TEXT("UGameSettingPressAnyKey"), &Z_Registration_Info_UClass_UGameSettingPressAnyKey, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingPressAnyKey), 711652570U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingPressAnyKey_h_3308384809(TEXT("/Script/GameSettings"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingPressAnyKey_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingPressAnyKey_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
