// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/Misc/GameSettingRotator.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingRotator() {}
// Cross Module References
	COMMONUI_API UClass* Z_Construct_UClass_UCommonRotator();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingRotator();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSettingRotator_NoRegister();
	UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References
	struct GameSettingRotator_eventBP_OnDefaultOptionSpecified_Parms
	{
		int32 DefaultOptionIndex;
	};
	static FName NAME_UGameSettingRotator_BP_OnDefaultOptionSpecified = FName(TEXT("BP_OnDefaultOptionSpecified"));
	void UGameSettingRotator::BP_OnDefaultOptionSpecified(int32 DefaultOptionIndex)
	{
		GameSettingRotator_eventBP_OnDefaultOptionSpecified_Parms Parms;
		Parms.DefaultOptionIndex=DefaultOptionIndex;
		ProcessEvent(FindFunctionChecked(NAME_UGameSettingRotator_BP_OnDefaultOptionSpecified),&Parms);
	}
	void UGameSettingRotator::StaticRegisterNativesUGameSettingRotator()
	{
	}
	struct Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics
	{
		static const UECodeGen_Private::FIntPropertyParams NewProp_DefaultOptionIndex;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::NewProp_DefaultOptionIndex = { "DefaultOptionIndex", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameSettingRotator_eventBP_OnDefaultOptionSpecified_Parms, DefaultOptionIndex), METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::NewProp_DefaultOptionIndex,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::Function_MetaDataParams[] = {
		{ "Category", "Events" },
		{ "DisplayName", "On Default Option Specified" },
		{ "ModuleRelativePath", "Public/Widgets/Misc/GameSettingRotator.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameSettingRotator, nullptr, "BP_OnDefaultOptionSpecified", nullptr, nullptr, Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::PropPointers), sizeof(GameSettingRotator_eventBP_OnDefaultOptionSpecified_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x08080800, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::PropPointers) < 2048);
	static_assert(sizeof(GameSettingRotator_eventBP_OnDefaultOptionSpecified_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameSettingRotator);
	UClass* Z_Construct_UClass_UGameSettingRotator_NoRegister()
	{
		return UGameSettingRotator::StaticClass();
	}
	struct Z_Construct_UClass_UGameSettingRotator_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGameSettingRotator_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCommonRotator,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRotator_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UGameSettingRotator_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UGameSettingRotator_BP_OnDefaultOptionSpecified, "BP_OnDefaultOptionSpecified" }, // 2933320112
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRotator_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameSettingRotator_Statics::Class_MetaDataParams[] = {
		{ "Category", "Settings" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "DisableNativeTick", "" },
		{ "IncludePath", "Widgets/Misc/GameSettingRotator.h" },
		{ "ModuleRelativePath", "Public/Widgets/Misc/GameSettingRotator.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGameSettingRotator_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameSettingRotator>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameSettingRotator_Statics::ClassParams = {
		&UGameSettingRotator::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		0,
		0,
		0x00B010A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameSettingRotator_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameSettingRotator_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UGameSettingRotator()
	{
		if (!Z_Registration_Info_UClass_UGameSettingRotator.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameSettingRotator.OuterSingleton, Z_Construct_UClass_UGameSettingRotator_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGameSettingRotator.OuterSingleton;
	}
	template<> GAMESETTINGS_API UClass* StaticClass<UGameSettingRotator>()
	{
		return UGameSettingRotator::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGameSettingRotator);
	UGameSettingRotator::~UGameSettingRotator() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingRotator_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingRotator_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGameSettingRotator, UGameSettingRotator::StaticClass, TEXT("UGameSettingRotator"), &Z_Registration_Info_UClass_UGameSettingRotator, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameSettingRotator), 868161376U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingRotator_h_4079296964(TEXT("/Script/GameSettings"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingRotator_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_Widgets_Misc_GameSettingRotator_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
