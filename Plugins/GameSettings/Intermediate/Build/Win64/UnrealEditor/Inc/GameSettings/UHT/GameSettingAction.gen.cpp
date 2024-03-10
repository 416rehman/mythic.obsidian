// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingAction.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingAction() {}
// Cross Module References
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingAction();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingAction_NoRegister();
	UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References
	void UGameSettingAction::StaticRegisterNativesUGameSettingAction()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingAction);
	UClass* Z_Construct_UClass_UGameSettingAction_NoRegister()
	{
		return UGameSettingAction::StaticClass();
	}
	struct Z_Construct_UClass_UGameSettingAction_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGameSettingAction_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameSetting,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingAction_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameSettingAction_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "GameSettingAction.h" },
		{ "ModuleRelativePath", "Public/GameSettingAction.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGameSettingAction_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingAction>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingAction_Statics::ClassParams = {
		&UGameSettingAction::StaticClass,
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
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingAction_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingAction_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UGameSettingAction()
	{
		if (!Z_Registration_Info_UClass_UGameSettingAction.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingAction.OuterSingleton, Z_Construct_UClass_UGameSettingAction_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGameSettingAction.OuterSingleton;
	}
	template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingAction>()
	{
		return UGameSettingAction::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingAction);
	UGameSettingAction::~UGameSettingAction() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingAction, UGameSettingAction::StaticClass, TEXT("UGameSettingAction"), &Z_Registration_Info_UClass_UGameSettingAction, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingAction), 2270378044U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_1076508801(TEXT("/Script/GameSettings"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingAction_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
