// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Private/Widgets/Responsive/GameResponsivePanel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameResponsivePanel() {}
// Cross Module References
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameResponsivePanel();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameResponsivePanel_NoRegister();
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameResponsivePanelSlot_NoRegister();
	UMG_API UClass* Z_Construct_UClass_UPanelWidget();
	UMG_API UClass* Z_Construct_UClass_UWidget_NoRegister();
	UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References
	DEFINE_FUNCTION(UGameResponsivePanel::execAddChildToGameResponsivePanel)
	{
		P_GET_OBJECT(UWidget,Z_Param_Content);
		P_FINISH;
		P_NATIVE_BEGIN;
		*(UGameResponsivePanelSlot**)Z_Param__Result=P_THIS->AddChildToGameResponsivePanel(Z_Param_Content);
		P_NATIVE_END;
	}
	void UGameResponsivePanel::StaticRegisterNativesUGameResponsivePanel()
	{
		UClass* Class = UGameResponsivePanel::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "AddChildToGameResponsivePanel", &UGameResponsivePanel::execAddChildToGameResponsivePanel },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics
	{
		struct GameResponsivePanel_eventAddChildToGameResponsivePanel_Parms
		{
			UWidget* Content;
			UGameResponsivePanelSlot* ReturnValue;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Content_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_Content;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_ReturnValue_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_ReturnValue;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_Content_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_Content = { "Content", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameResponsivePanel_eventAddChildToGameResponsivePanel_Parms, Content), Z_Construct_UClass_UWidget_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_Content_MetaData), Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_Content_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_ReturnValue_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000080588, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(GameResponsivePanel_eventAddChildToGameResponsivePanel_Parms, ReturnValue), Z_Construct_UClass_UGameResponsivePanelSlot_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_ReturnValue_MetaData), Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_ReturnValue_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_Content,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::NewProp_ReturnValue,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::Function_MetaDataParams[] = {
		{ "Category", "Widget" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**  */" },
#endif
		{ "ModuleRelativePath", "Private/Widgets/Responsive/GameResponsivePanel.h" },
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UGameResponsivePanel, nullptr, "AddChildToGameResponsivePanel", nullptr, nullptr, Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::PropPointers), sizeof(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::GameResponsivePanel_eventAddChildToGameResponsivePanel_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::Function_MetaDataParams), Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::GameResponsivePanel_eventAddChildToGameResponsivePanel_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UGameResponsivePanel);
	UClass* Z_Construct_UClass_UGameResponsivePanel_NoRegister()
	{
		return UGameResponsivePanel::StaticClass();
	}
	struct Z_Construct_UClass_UGameResponsivePanel_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bCanStackVertically_MetaData[];
#endif
		static void NewProp_bCanStackVertically_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bCanStackVertically;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UGameResponsivePanel_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UPanelWidget,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameResponsivePanel_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UGameResponsivePanel_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UGameResponsivePanel_AddChildToGameResponsivePanel, "AddChildToGameResponsivePanel" }, // 617017823
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameResponsivePanel_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameResponsivePanel_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * Allows widgets to be laid out in a flow horizontally.\n *\n * * Many Children\n * * Flow Horizontal\n */" },
#endif
		{ "IncludePath", "Widgets/Responsive/GameResponsivePanel.h" },
		{ "ModuleRelativePath", "Private/Widgets/Responsive/GameResponsivePanel.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Allows widgets to be laid out in a flow horizontally.\n\n* Many Children\n* Flow Horizontal" },
#endif
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically_MetaData[] = {
		{ "Category", "Behavior" },
		{ "ModuleRelativePath", "Private/Widgets/Responsive/GameResponsivePanel.h" },
	};
#endif
	void Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically_SetBit(void* Obj)
	{
		((UGameResponsivePanel*)Obj)->bCanStackVertically = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically = { "bCanStackVertically", nullptr, (EPropertyFlags)0x0010000000000015, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(UGameResponsivePanel), &Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically_MetaData), Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UGameResponsivePanel_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UGameResponsivePanel_Statics::NewProp_bCanStackVertically,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UGameResponsivePanel_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UGameResponsivePanel>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UGameResponsivePanel_Statics::ClassParams = {
		&UGameResponsivePanel::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UGameResponsivePanel_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UGameResponsivePanel_Statics::PropPointers),
		0,
		0x00A000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UGameResponsivePanel_Statics::Class_MetaDataParams), Z_Construct_UClass_UGameResponsivePanel_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UGameResponsivePanel_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UGameResponsivePanel()
	{
		if (!Z_Registration_Info_UClass_UGameResponsivePanel.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UGameResponsivePanel.OuterSingleton, Z_Construct_UClass_UGameResponsivePanel_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UGameResponsivePanel.OuterSingleton;
	}
	template<> GAMESETTINGS_API UClass* StaticClass<UGameResponsivePanel>()
	{
		return UGameResponsivePanel::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UGameResponsivePanel);
	UGameResponsivePanel::~UGameResponsivePanel() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UGameResponsivePanel, UGameResponsivePanel::StaticClass, TEXT("UGameResponsivePanel"), &Z_Registration_Info_UClass_UGameResponsivePanel, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UGameResponsivePanel), 609555779U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_579402748(TEXT("/Script/GameSettings"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Private_Widgets_Responsive_GameResponsivePanel_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
