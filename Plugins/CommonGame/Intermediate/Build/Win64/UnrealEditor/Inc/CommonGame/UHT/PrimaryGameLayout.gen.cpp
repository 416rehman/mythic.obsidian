// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PrimaryGameLayout.h"
#include "GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePrimaryGameLayout() {}
// Cross Module References
	COMMONGAME_API UClass* Z_Construct_UClass_UPrimaryGameLayout();
	COMMONGAME_API UClass* Z_Construct_UClass_UPrimaryGameLayout_NoRegister();
	COMMONUI_API UClass* Z_Construct_UClass_UCommonActivatableWidgetContainerBase_NoRegister();
	COMMONUI_API UClass* Z_Construct_UClass_UCommonUserWidget();
	GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
	UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References
	DEFINE_FUNCTION(UPrimaryGameLayout::execRegisterLayer)
	{
		P_GET_STRUCT(FGameplayTag,Z_Param_LayerTag);
		P_GET_OBJECT(UCommonActivatableWidgetContainerBase,Z_Param_LayerWidget);
		P_FINISH;
		P_NATIVE_BEGIN;
		P_THIS->RegisterLayer(Z_Param_LayerTag,Z_Param_LayerWidget);
		P_NATIVE_END;
	}
	void UPrimaryGameLayout::StaticRegisterNativesUPrimaryGameLayout()
	{
		UClass* Class = UPrimaryGameLayout::StaticClass();
		static const FNameNativePtrPair Funcs[] = {
			{ "RegisterLayer", &UPrimaryGameLayout::execRegisterLayer },
		};
		FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
	}
	struct Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics
	{
		struct PrimaryGameLayout_eventRegisterLayer_Parms
		{
			FGameplayTag LayerTag;
			UCommonActivatableWidgetContainerBase* LayerWidget;
		};
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LayerTag_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_LayerTag;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_LayerWidget_MetaData[];
#endif
		static const UECodeGen_Private::FObjectPropertyParams NewProp_LayerWidget;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[];
#endif
		static const UECodeGen_Private::FFunctionParams FuncParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag_MetaData[] = {
		{ "Categories", "UI.Layer" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag = { "LayerTag", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrimaryGameLayout_eventRegisterLayer_Parms, LayerTag), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag_MetaData), Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag_MetaData) }; // 2083603574
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif
	const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget = { "LayerWidget", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrimaryGameLayout_eventRegisterLayer_Parms, LayerWidget), Z_Construct_UClass_UCommonActivatableWidgetContainerBase_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget_MetaData), Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget,
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::Function_MetaDataParams[] = {
		{ "Category", "Layer" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Register a layer that widgets can be pushed onto. */" },
#endif
		{ "ModuleRelativePath", "Public/PrimaryGameLayout.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Register a layer that widgets can be pushed onto." },
#endif
	};
#endif
	const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPrimaryGameLayout, nullptr, "RegisterLayer", nullptr, nullptr, Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PrimaryGameLayout_eventRegisterLayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04080401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::Function_MetaDataParams) };
	static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers) < 2048);
	static_assert(sizeof(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PrimaryGameLayout_eventRegisterLayer_Parms) < MAX_uint16);
	UFunction* Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer()
	{
		static UFunction* ReturnFunction = nullptr;
		if (!ReturnFunction)
		{
			UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::FuncParams);
		}
		return ReturnFunction;
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPrimaryGameLayout);
	UClass* Z_Construct_UClass_UPrimaryGameLayout_NoRegister()
	{
		return UPrimaryGameLayout::StaticClass();
	}
	struct Z_Construct_UClass_UPrimaryGameLayout_Statics
	{
		static UObject* (*const DependentSingletons[])();
		static const FClassFunctionLinkInfo FuncInfo[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_Layers_ValueProp;
		static const UECodeGen_Private::FStructPropertyParams NewProp_Layers_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Layers_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_Layers;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UPrimaryGameLayout_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UCommonUserWidget,
		(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::DependentSingletons) < 16);
	const FClassFunctionLinkInfo Z_Construct_UClass_UPrimaryGameLayout_Statics::FuncInfo[] = {
		{ &Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer, "RegisterLayer" }, // 3908630959
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::FuncInfo) < 2048);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPrimaryGameLayout_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * The primary game UI layout of your game.  This widget class represents how to layout, push and display all layers\n * of the UI for a single player.  Each player in a split-screen game will receive their own primary game layout.\n */" },
#endif
		{ "DisableNativeTick", "" },
		{ "IncludePath", "PrimaryGameLayout.h" },
		{ "ModuleRelativePath", "Public/PrimaryGameLayout.h" },
		{ "ObjectInitializerConstructorDeclared", "" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The primary game UI layout of your game.  This widget class represents how to layout, push and display all layers\nof the UI for a single player.  Each player in a split-screen game will receive their own primary game layout." },
#endif
	};
#endif
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_ValueProp = { "Layers", nullptr, (EPropertyFlags)0x0004000000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UClass_UCommonActivatableWidgetContainerBase_NoRegister, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_Key_KeyProp = { "Layers_Key", nullptr, (EPropertyFlags)0x0000000000080008, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 2083603574
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_MetaData[] = {
		{ "Categories", "UI.Layer" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The registered layers for the primary layout.\n" },
#endif
		{ "EditInline", "true" },
		{ "ModuleRelativePath", "Public/PrimaryGameLayout.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The registered layers for the primary layout." },
#endif
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers = { "Layers", nullptr, (EPropertyFlags)0x0044008000002008, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPrimaryGameLayout, Layers), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_MetaData), Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_MetaData) }; // 2083603574
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPrimaryGameLayout_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UPrimaryGameLayout_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPrimaryGameLayout>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UPrimaryGameLayout_Statics::ClassParams = {
		&UPrimaryGameLayout::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		FuncInfo,
		Z_Construct_UClass_UPrimaryGameLayout_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		UE_ARRAY_COUNT(FuncInfo),
		UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::PropPointers),
		0,
		0x00B010A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::Class_MetaDataParams), Z_Construct_UClass_UPrimaryGameLayout_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UPrimaryGameLayout()
	{
		if (!Z_Registration_Info_UClass_UPrimaryGameLayout.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPrimaryGameLayout.OuterSingleton, Z_Construct_UClass_UPrimaryGameLayout_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UPrimaryGameLayout.OuterSingleton;
	}
	template<> COMMONGAME_API UClass* StaticClass<UPrimaryGameLayout>()
	{
		return UPrimaryGameLayout::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UPrimaryGameLayout);
	UPrimaryGameLayout::~UPrimaryGameLayout() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics
	{
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UPrimaryGameLayout, UPrimaryGameLayout::StaticClass, TEXT("UPrimaryGameLayout"), &Z_Registration_Info_UClass_UPrimaryGameLayout, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPrimaryGameLayout), 2018131600U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_3575306899(TEXT("/Script/CommonGame"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics::ClassInfo),
		nullptr, 0,
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
