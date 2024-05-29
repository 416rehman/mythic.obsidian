// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingCollection.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingCollection() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingCollection();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingCollection_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingCollectionPage();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingCollectionPage_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingCollection
void UGameSettingCollection::StaticRegisterNativesUGameSettingCollection()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingCollection);
UClass* Z_Construct_UClass_UGameSettingCollection_NoRegister()
{
	return UGameSettingCollection::StaticClass();
}
struct Z_Construct_UClass_UGameSettingCollection_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "//--------------------------------------\n// UGameSettingCollection\n//--------------------------------------\n" },
#endif
		{ "IncludePath", "GameSettingCollection.h" },
		{ "ModuleRelativePath", "Public/GameSettingCollection.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UGameSettingCollection" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Settings_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** The settings owned by this collection. */" },
#endif
		{ "ModuleRelativePath", "Public/GameSettingCollection.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The settings owned by this collection." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Settings_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_Settings;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingCollection>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingCollection_Statics::NewProp_Settings_Inner = { "Settings", nullptr, (EPropertyFlags)0x0104000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UGameSettingCollection_Statics::NewProp_Settings = { "Settings", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingCollection, Settings), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Settings_MetaData), NewProp_Settings_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameSettingCollection_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingCollection_Statics::NewProp_Settings_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingCollection_Statics::NewProp_Settings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingCollection_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameSettingCollection_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameSetting,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingCollection_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingCollection_Statics::ClassParams = {
	&UGameSettingCollection::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UGameSettingCollection_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingCollection_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingCollection_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingCollection_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingCollection()
{
	if (!Z_Registration_Info_UClass_UGameSettingCollection.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingCollection.OuterSingleton, Z_Construct_UClass_UGameSettingCollection_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingCollection.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingCollection>()
{
	return UGameSettingCollection::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingCollection);
UGameSettingCollection::~UGameSettingCollection() {}
// End Class UGameSettingCollection

// Begin Class UGameSettingCollectionPage
void UGameSettingCollectionPage::StaticRegisterNativesUGameSettingCollectionPage()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingCollectionPage);
UClass* Z_Construct_UClass_UGameSettingCollectionPage_NoRegister()
{
	return UGameSettingCollectionPage::StaticClass();
}
struct Z_Construct_UClass_UGameSettingCollectionPage_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "//--------------------------------------\n// UGameSettingCollectionPage\n//--------------------------------------\n" },
#endif
		{ "IncludePath", "GameSettingCollection.h" },
		{ "ModuleRelativePath", "Public/GameSettingCollection.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UGameSettingCollectionPage" },
#endif
	};
#endif // WITH_METADATA
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingCollectionPage>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
UObject* (*const Z_Construct_UClass_UGameSettingCollectionPage_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameSettingCollection,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingCollectionPage_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingCollectionPage_Statics::ClassParams = {
	&UGameSettingCollectionPage::StaticClass,
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
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingCollectionPage_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingCollectionPage_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingCollectionPage()
{
	if (!Z_Registration_Info_UClass_UGameSettingCollectionPage.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingCollectionPage.OuterSingleton, Z_Construct_UClass_UGameSettingCollectionPage_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingCollectionPage.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingCollectionPage>()
{
	return UGameSettingCollectionPage::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingCollectionPage);
UGameSettingCollectionPage::~UGameSettingCollectionPage() {}
// End Class UGameSettingCollectionPage

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingCollection, UGameSettingCollection::StaticClass, TEXT("UGameSettingCollection"), &Z_Registration_Info_UClass_UGameSettingCollection, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingCollection), 2749658513U) },
		{ Z_Construct_UClass_UGameSettingCollectionPage, UGameSettingCollectionPage::StaticClass, TEXT("UGameSettingCollectionPage"), &Z_Registration_Info_UClass_UGameSettingCollectionPage, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingCollectionPage), 1490178399U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_2581113274(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingCollection_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
