// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/GameSettingListView.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingListView() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingListView();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingListView_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingVisualData_NoRegister();
UMG_API UClass* Z_Construct_UClass_UListView();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingListView
void UGameSettingListView::StaticRegisterNativesUGameSettingListView()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingListView);
UClass* Z_Construct_UClass_UGameSettingListView_NoRegister()
{
	return UGameSettingListView::StaticClass();
}
struct Z_Construct_UClass_UGameSettingListView_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * List of game settings.  Every entry widget needs to extend from GameSettingListEntryBase.\n */" },
#endif
		{ "EntryClass", "GameSettingListEntryBase" },
		{ "IncludePath", "Widgets/GameSettingListView.h" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingListView.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "List of game settings.  Every entry widget needs to extend from GameSettingListEntryBase." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_VisualData_MetaData[] = {
		{ "Category", "GameSettingListView" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingListView.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_VisualData;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingListView>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingListView_Statics::NewProp_VisualData = { "VisualData", nullptr, (EPropertyFlags)0x0124080000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingListView, VisualData), Z_Construct_UClass_UGameSettingVisualData_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_VisualData_MetaData), NewProp_VisualData_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameSettingListView_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingListView_Statics::NewProp_VisualData,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingListView_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameSettingListView_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UListView,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingListView_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingListView_Statics::ClassParams = {
	&UGameSettingListView::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UGameSettingListView_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingListView_Statics::PropPointers),
	0,
	0x00B000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingListView_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingListView_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingListView()
{
	if (!Z_Registration_Info_UClass_UGameSettingListView.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingListView.OuterSingleton, Z_Construct_UClass_UGameSettingListView_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingListView.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingListView>()
{
	return UGameSettingListView::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingListView);
UGameSettingListView::~UGameSettingListView() {}
// End Class UGameSettingListView

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingListView_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingListView, UGameSettingListView::StaticClass, TEXT("UGameSettingListView"), &Z_Registration_Info_UClass_UGameSettingListView, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingListView), 3632248256U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingListView_h_2703521232(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingListView_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingListView_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
