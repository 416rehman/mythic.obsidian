// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/GameSettingDetailExtension.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingDetailExtension() {}

// Begin Cross Module References
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingDetailExtension();
GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingDetailExtension_NoRegister();
UMG_API UClass* Z_Construct_UClass_UUserWidget();
UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References

// Begin Class UGameSettingDetailExtension Function OnSettingAssigned
struct GameSettingDetailExtension_eventOnSettingAssigned_Parms
{
	UGameSetting* InSetting;
};
static FName NAME_UGameSettingDetailExtension_OnSettingAssigned = FName(TEXT("OnSettingAssigned"));
void UGameSettingDetailExtension::OnSettingAssigned(UGameSetting* InSetting)
{
	GameSettingDetailExtension_eventOnSettingAssigned_Parms Parms;
	Parms.InSetting=InSetting;
	ProcessEvent(FindFunctionChecked(NAME_UGameSettingDetailExtension_OnSettingAssigned),&Parms);
}
struct Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailExtension.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InSetting;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::NewProp_InSetting = { "InSetting", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingDetailExtension_eventOnSettingAssigned_Parms, InSetting), Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::NewProp_InSetting,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingDetailExtension, nullptr, "OnSettingAssigned", nullptr, nullptr, Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::PropPointers), sizeof(GameSettingDetailExtension_eventOnSettingAssigned_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08080800, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::Function_MetaDataParams) };
static_assert(sizeof(GameSettingDetailExtension_eventOnSettingAssigned_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned_Statics::FuncParams);
	}
	return ReturnFunction;
}
// End Class UGameSettingDetailExtension Function OnSettingAssigned

// Begin Class UGameSettingDetailExtension Function OnSettingValueChanged
struct GameSettingDetailExtension_eventOnSettingValueChanged_Parms
{
	UGameSetting* InSetting;
};
static FName NAME_UGameSettingDetailExtension_OnSettingValueChanged = FName(TEXT("OnSettingValueChanged"));
void UGameSettingDetailExtension::OnSettingValueChanged(UGameSetting* InSetting)
{
	GameSettingDetailExtension_eventOnSettingValueChanged_Parms Parms;
	Parms.InSetting=InSetting;
	ProcessEvent(FindFunctionChecked(NAME_UGameSettingDetailExtension_OnSettingValueChanged),&Parms);
}
struct Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailExtension.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_InSetting;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::NewProp_InSetting = { "InSetting", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingDetailExtension_eventOnSettingValueChanged_Parms, InSetting), Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::NewProp_InSetting,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingDetailExtension, nullptr, "OnSettingValueChanged", nullptr, nullptr, Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::PropPointers), sizeof(GameSettingDetailExtension_eventOnSettingValueChanged_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08080800, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::Function_MetaDataParams) };
static_assert(sizeof(GameSettingDetailExtension_eventOnSettingValueChanged_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged_Statics::FuncParams);
	}
	return ReturnFunction;
}
// End Class UGameSettingDetailExtension Function OnSettingValueChanged

// Begin Class UGameSettingDetailExtension
void UGameSettingDetailExtension::StaticRegisterNativesUGameSettingDetailExtension()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingDetailExtension);
UClass* Z_Construct_UClass_UGameSettingDetailExtension_NoRegister()
{
	return UGameSettingDetailExtension::StaticClass();
}
struct Z_Construct_UClass_UGameSettingDetailExtension_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "Category", "Settings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "DisableNativeTick", "" },
		{ "IncludePath", "Widgets/GameSettingDetailExtension.h" },
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailExtension.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Setting_MetaData[] = {
		{ "ModuleRelativePath", "Public/Widgets/GameSettingDetailExtension.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Setting;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingAssigned, "OnSettingAssigned" }, // 3610385031
		{ &Z_Construct_UFunction_UGameSettingDetailExtension_OnSettingValueChanged, "OnSettingValueChanged" }, // 829104172
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingDetailExtension>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UGameSettingDetailExtension_Statics::NewProp_Setting = { "Setting", nullptr, (EPropertyFlags)0x0124080000002000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UGameSettingDetailExtension, Setting), Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Setting_MetaData), NewProp_Setting_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameSettingDetailExtension_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameSettingDetailExtension_Statics::NewProp_Setting,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailExtension_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UGameSettingDetailExtension_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UUserWidget,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailExtension_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingDetailExtension_Statics::ClassParams = {
	&UGameSettingDetailExtension::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_UGameSettingDetailExtension_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailExtension_Statics::PropPointers),
	0,
	0x00B010A1u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingDetailExtension_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingDetailExtension_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UGameSettingDetailExtension()
{
	if (!Z_Registration_Info_UClass_UGameSettingDetailExtension.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingDetailExtension.OuterSingleton, Z_Construct_UClass_UGameSettingDetailExtension_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UGameSettingDetailExtension.OuterSingleton;
}
template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingDetailExtension>()
{
	return UGameSettingDetailExtension::StaticClass();
}
UGameSettingDetailExtension::UGameSettingDetailExtension(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingDetailExtension);
UGameSettingDetailExtension::~UGameSettingDetailExtension() {}
// End Class UGameSettingDetailExtension

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingDetailExtension, UGameSettingDetailExtension::StaticClass, TEXT("UGameSettingDetailExtension"), &Z_Registration_Info_UClass_UGameSettingDetailExtension, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingDetailExtension), 3832456785U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_2291478037(TEXT("/Script/GameSettings"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_GameSettingDetailExtension_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
