// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingRegistry.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingRegistry() {}

// Begin Cross Module References
COREUOBJECT_API UClass* Z_Construct_UClass_UObject();
ENGINE_API UClass* Z_Construct_UClass_ULocalPlayer_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingRegistry();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingRegistry_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingRegistry
void UGameSettingRegistry::StaticRegisterNativesUGameSettingRegistry()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingRegistry);
UClass* Z_Construct_UClass_UGameSettingRegistry_NoRegister()
{
	return UGameSettingRegistry::StaticClass();
}
struct Z_Construct_UClass_UGameSettingRegistry_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "GameSettingRegistry.h" },
		{ "ModuleRelativePath", "Public/GameSettingRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_TopLevelSettings_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RegisteredSettings_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingRegistry.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_OwningLocalPlayer_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingRegistry.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_TopLevelSettings_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_TopLevelSettings;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RegisteredSettings_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_RegisteredSettings;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_OwningLocalPlayer;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingRegistry>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_TopLevelSettings_Inner = { "TopLevelSettings", nullptr, (EPropertyFlags)0x0104000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_TopLevelSettings = { "TopLevelSettings", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingRegistry, TopLevelSettings), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_TopLevelSettings_MetaData), NewProp_TopLevelSettings_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_RegisteredSettings_Inner = { "RegisteredSettings", nullptr, (EPropertyFlags)0x0104000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_RegisteredSettings = { "RegisteredSettings", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingRegistry, RegisteredSettings), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RegisteredSettings_MetaData), NewProp_RegisteredSettings_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_OwningLocalPlayer = { "OwningLocalPlayer", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingRegistry, OwningLocalPlayer), Z_Construct_UClass_ULocalPlayer_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_OwningLocalPlayer_MetaData), NewProp_OwningLocalPlayer_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameSettingRegistry_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_TopLevelSettings_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_TopLevelSettings,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_RegisteredSettings_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_RegisteredSettings,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingRegistry_Statics::NewProp_OwningLocalPlayer,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRegistry_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameSettingRegistry_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UObject,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRegistry_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingRegistry_Statics::ClassParams = {
	&UGameSettingRegistry::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UGameSettingRegistry_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRegistry_Statics::PropPointers),
	0,
	0x001000A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRegistry_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingRegistry_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingRegistry()
{
	if (!Z_Registration_Info_UClass_UGameSettingRegistry.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingRegistry.OuterSingleton, Z_Construct_UClass_UGameSettingRegistry_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingRegistry.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingRegistry>()
{
	return UGameSettingRegistry::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingRegistry);
UGameSettingRegistry::~UGameSettingRegistry() {}
// End Class UGameSettingRegistry

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingRegistry_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingRegistry, UGameSettingRegistry::StaticClass, TEXT("UGameSettingRegistry"), &Z_Registration_Info_UClass_UGameSettingRegistry, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingRegistry), 3677350250U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingRegistry_h_3500073381(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingRegistry_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingRegistry_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
