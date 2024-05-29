// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/GameSettingDetailView.h"
#include "Runtime/UMG/Public/Blueprint/UserWidgetPool.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingDetailView() {}

// Begin Cross Module References
COMMONUI_API UClass* Z_Construct_UClass_UCommonRichTextBlock_NoRegister();
COMMONUI_API UClass* Z_Construct_UClass_UCommonTextBlock_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingDetailView();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingDetailView_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingVisualData_NoRegister();
UMG_API UClass* Z_Construct_UClass_UUserWidget();
UMG_API UClass* Z_Construct_UClass_UVerticalBox_NoRegister();
UMG_API UScriptStruct* Z_Construct_UScriptStruct_FUserWidgetPool();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingDetailView
void UGameSettingDetailView::StaticRegisterNativesUGameSettingDetailView()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingDetailView);
UClass* Z_Construct_UClass_UGameSettingDetailView_NoRegister()
{
	return UGameSettingDetailView::StaticClass();
}
struct Z_Construct_UClass_UGameSettingDetailView_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "Widgets/GameSettingDetailView.h" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_VisualData_MetaData[] = {
		{ "Category", "GameSettingDetailView" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_ExtensionWidgetPool_MetaData[] = {
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CurrentSetting_MetaData[] = {
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Text_SettingName_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidgetOptional", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "GameSettingDetailView" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Bound Widgets\n" },
#endif
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Bound Widgets" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RichText_Description_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidgetOptional", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "GameSettingDetailView" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RichText_DynamicDetails_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidgetOptional", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "GameSettingDetailView" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RichText_WarningDetails_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidgetOptional", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "GameSettingDetailView" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_RichText_DisabledDetails_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidgetOptional", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "GameSettingDetailView" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Box_DetailsExtension_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidgetOptional", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "GameSettingDetailView" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailView.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_VisualData;
	static const UECodeGen_Private::FStructPropertyParams NewProp_ExtensionWidgetPool;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CurrentSetting;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Text_SettingName;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RichText_Description;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RichText_DynamicDetails;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RichText_WarningDetails;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_RichText_DisabledDetails;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Box_DetailsExtension;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingDetailView>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_VisualData = { "VisualData", nullptr, (EPropertyFlags)0x0124080000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, VisualData), Z_Construct_UClass_UGameSettingVisualData_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_VisualData_MetaData), NewProp_VisualData_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_ExtensionWidgetPool = { "ExtensionWidgetPool", nullptr, (EPropertyFlags)0x0020088000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, ExtensionWidgetPool), Z_Construct_UScriptStruct_FUserWidgetPool, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_ExtensionWidgetPool_MetaData), NewProp_ExtensionWidgetPool_MetaData) }; // 871085244
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_CurrentSetting = { "CurrentSetting", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, CurrentSetting), Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CurrentSetting_MetaData), NewProp_CurrentSetting_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_Text_SettingName = { "Text_SettingName", nullptr, (EPropertyFlags)0x014400000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, Text_SettingName), Z_Construct_UClass_UCommonTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Text_SettingName_MetaData), NewProp_Text_SettingName_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_Description = { "RichText_Description", nullptr, (EPropertyFlags)0x014400000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, RichText_Description), Z_Construct_UClass_UCommonRichTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RichText_Description_MetaData), NewProp_RichText_Description_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_DynamicDetails = { "RichText_DynamicDetails", nullptr, (EPropertyFlags)0x014400000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, RichText_DynamicDetails), Z_Construct_UClass_UCommonRichTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RichText_DynamicDetails_MetaData), NewProp_RichText_DynamicDetails_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_WarningDetails = { "RichText_WarningDetails", nullptr, (EPropertyFlags)0x014400000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, RichText_WarningDetails), Z_Construct_UClass_UCommonRichTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RichText_WarningDetails_MetaData), NewProp_RichText_WarningDetails_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_DisabledDetails = { "RichText_DisabledDetails", nullptr, (EPropertyFlags)0x014400000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, RichText_DisabledDetails), Z_Construct_UClass_UCommonRichTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_RichText_DisabledDetails_MetaData), NewProp_RichText_DisabledDetails_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_Box_DetailsExtension = { "Box_DetailsExtension", nullptr, (EPropertyFlags)0x014400000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailView, Box_DetailsExtension), Z_Construct_UClass_UVerticalBox_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Box_DetailsExtension_MetaData), NewProp_Box_DetailsExtension_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameSettingDetailView_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_VisualData,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_ExtensionWidgetPool,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_CurrentSetting,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_Text_SettingName,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_Description,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_DynamicDetails,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_WarningDetails,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_RichText_DisabledDetails,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailView_Statics::NewProp_Box_DetailsExtension,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailView_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameSettingDetailView_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UUserWidget,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailView_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingDetailView_Statics::ClassParams = {
	&UGameSettingDetailView::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UGameSettingDetailView_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailView_Statics::PropPointers),
	0,
	0x00B010A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailView_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingDetailView_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingDetailView()
{
	if (!Z_Registration_Info_UClass_UGameSettingDetailView.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingDetailView.OuterSingleton, Z_Construct_UClass_UGameSettingDetailView_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingDetailView.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingDetailView>()
{
	return UGameSettingDetailView::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingDetailView);
UGameSettingDetailView::~UGameSettingDetailView() {}
// End Class UGameSettingDetailView

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailView_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingDetailView, UGameSettingDetailView::StaticClass, TEXT("UGameSettingDetailView"), &Z_Registration_Info_UClass_UGameSettingDetailView, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingDetailView), 1985736856U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailView_h_446180939(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailView_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailView_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
