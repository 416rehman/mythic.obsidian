// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/Misc/KeyAlreadyBoundWarning.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeKeyAlreadyBoundWarning() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingPressAnyKey();
GAMESETTINGS_API UClass* Z_Construct_UClass_UKeyAlreadyBoundWarning();
GAMESETTINGS_API UClass* Z_Construct_UClass_UKeyAlreadyBoundWarning_NoRegister();
UMG_API UClass* Z_Construct_UClass_UTextBlock_NoRegister();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UKeyAlreadyBoundWarning
void UKeyAlreadyBoundWarning::StaticRegisterNativesUKeyAlreadyBoundWarning()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UKeyAlreadyBoundWarning);
UClass* Z_Construct_UClass_UKeyAlreadyBoundWarning_NoRegister()
{
	return UKeyAlreadyBoundWarning::StaticClass();
}
struct Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * UKeyAlreadyBoundWarning\n * Press any key screen with text blocks for warning users when a key is already bound\n */" },
#endif
		{ "IncludePath", "Widgets/Misc/KeyAlreadyBoundWarning.h" },
		{ "ModuleRelativePath", "Public/Widgets/Misc/KeyAlreadyBoundWarning.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "UKeyAlreadyBoundWarning\nPress any key screen with text blocks for warning users when a key is already bound" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WarningText_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidget", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "KeyAlreadyBoundWarning" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/Misc/KeyAlreadyBoundWarning.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_CancelText_MetaData[] = {
		{ "AllowPrivateAccess", "TRUE" },
		{ "BindWidget", "" },
		{ "BlueprintProtected", "TRUE" },
		{ "Category", "KeyAlreadyBoundWarning" },
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/Widgets/Misc/KeyAlreadyBoundWarning.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_WarningText;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_CancelText;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UKeyAlreadyBoundWarning>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::NewProp_WarningText = { "WarningText", nullptr, (EPropertyFlags)0x012408000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UKeyAlreadyBoundWarning, WarningText), Z_Construct_UClass_UTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WarningText_MetaData), NewProp_WarningText_MetaData) };
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::NewProp_CancelText = { "CancelText", nullptr, (EPropertyFlags)0x012408000008001c, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UKeyAlreadyBoundWarning, CancelText), Z_Construct_UClass_UTextBlock_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_CancelText_MetaData), NewProp_CancelText_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::NewProp_WarningText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::NewProp_CancelText,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameSettingPressAnyKey,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::ClassParams = {
	&UKeyAlreadyBoundWarning::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::PropPointers),
	0,
	0x00B010A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::Class_MetaDataParams), Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UKeyAlreadyBoundWarning()
{
	if (!Z_Registration_Info_UClass_UKeyAlreadyBoundWarning.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UKeyAlreadyBoundWarning.OuterSingleton, Z_Construct_UClass_UKeyAlreadyBoundWarning_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UKeyAlreadyBoundWarning.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UKeyAlreadyBoundWarning>()
{
	return UKeyAlreadyBoundWarning::StaticClass();
}
UKeyAlreadyBoundWarning::UKeyAlreadyBoundWarning(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UKeyAlreadyBoundWarning);
UKeyAlreadyBoundWarning::~UKeyAlreadyBoundWarning() {}
// End Class UKeyAlreadyBoundWarning

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_KeyAlreadyBoundWarning_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UKeyAlreadyBoundWarning, UKeyAlreadyBoundWarning::StaticClass, TEXT("UKeyAlreadyBoundWarning"), &Z_Registration_Info_UClass_UKeyAlreadyBoundWarning, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UKeyAlreadyBoundWarning), 242373186U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_KeyAlreadyBoundWarning_h_2622966470(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_KeyAlreadyBoundWarning_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_KeyAlreadyBoundWarning_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
