// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PrimaryGameLayout.h"
#include "Runtime/GameplayTags/Classes/GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePrimaryGameLayout() {}

// Begin Cross Module References
COMMONGAME_API UClass* Z_Construct_UClass_UPrimaryGameLayout();
COMMONGAME_API UClass* Z_Construct_UClass_UPrimaryGameLayout_NoRegister();
COMMONUI_API UClass* Z_Construct_UClass_UCommonActivatableWidgetContainerBase_NoRegister();
COMMONUI_API UClass* Z_Construct_UClass_UCommonUserWidget();
GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTag();
UPackage* Z_Construct_UPackage__Script_CommonGame();
// End Cross Module References

// Begin Class UPrimaryGameLayout Function RegisterLayer
struct Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics
{
	struct PrimaryGameLayout_eventRegisterLayer_Parms
	{
		FGameplayTag LayerTag;
		UCommonActivatableWidgetContainerBase* LayerWidget;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Layer" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Register a layer that widgets can be pushed onto. */" },
#endif
		{ "ModuleRelativePath", "Public/PrimaryGameLayout.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Register a layer that widgets can be pushed onto." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LayerTag_MetaData[] = {
		{ "Categories", "UI.Layer" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_LayerWidget_MetaData[] = {
		{ "EditInline", "true" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_LayerTag;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_LayerWidget;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag = { "LayerTag", nullptr, (EPropertyFlags)0x0010000000000080, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrimaryGameLayout_eventRegisterLayer_Parms, LayerTag), Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LayerTag_MetaData), NewProp_LayerTag_MetaData) }; // 1298103297
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget = { "LayerWidget", nullptr, (EPropertyFlags)0x0010000000080080, UECodeGen_Private::EPropertyGenFlags::Object, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(PrimaryGameLayout_eventRegisterLayer_Parms, LayerWidget), Z_Construct_UClass_UCommonActivatableWidgetContainerBase_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_LayerWidget_MetaData), NewProp_LayerWidget_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerTag,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::NewProp_LayerWidget,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_UPrimaryGameLayout, nullptr, "RegisterLayer", nullptr, nullptr, Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PropPointers), sizeof(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::PrimaryGameLayout_eventRegisterLayer_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x04080401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::Function_MetaDataParams), Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer_Statics::Function_MetaDataParams) };
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
DEFINE_FUNCTION(UPrimaryGameLayout::execRegisterLayer)
{
	P_GET_STRUCT(FGameplayTag,Z_Param_LayerTag);
	P_GET_OBJECT(UCommonActivatableWidgetContainerBase,Z_Param_LayerWidget);
	P_FINISH;
	P_NATIVE_BEGIN;
	P_THIS->RegisterLayer(Z_Param_LayerTag,Z_Param_LayerWidget);
	P_NATIVE_END;
}
// End Class UPrimaryGameLayout Function RegisterLayer

// Begin Class UPrimaryGameLayout
void UPrimaryGameLayout::StaticRegisterNativesUPrimaryGameLayout()
{
	UClass* Class = UPrimaryGameLayout::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "RegisterLayer", &UPrimaryGameLayout::execRegisterLayer },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPrimaryGameLayout);
UClass* Z_Construct_UClass_UPrimaryGameLayout_NoRegister()
{
	return UPrimaryGameLayout::StaticClass();
}
struct Z_Construct_UClass_UPrimaryGameLayout_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
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
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Layers_MetaData[] = {
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
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Layers_ValueProp;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Layers_Key_KeyProp;
	static const UECodeGen_Private::FMapPropertyParams NewProp_Layers;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_UPrimaryGameLayout_RegisterLayer, "RegisterLayer" }, // 990337099
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPrimaryGameLayout>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_ValueProp = { "Layers", nullptr, (EPropertyFlags)0x0104000000080008, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UClass_UCommonActivatableWidgetContainerBase_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_Key_KeyProp = { "Layers_Key", nullptr, (EPropertyFlags)0x0100000000080008, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UScriptStruct_FGameplayTag, METADATA_PARAMS(0, nullptr) }; // 1298103297
const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers = { "Layers", nullptr, (EPropertyFlags)0x0144008000002008, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPrimaryGameLayout, Layers), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Layers_MetaData), NewProp_Layers_MetaData) }; // 1298103297
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPrimaryGameLayout_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_ValueProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers_Key_KeyProp,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPrimaryGameLayout_Statics::NewProp_Layers,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UPrimaryGameLayout_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UCommonUserWidget,
	(UObject* (*)())Z_Construct_UPackage__Script_CommonGame,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPrimaryGameLayout_Statics::DependentSingletons) < 16);
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
// End Class UPrimaryGameLayout

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UPrimaryGameLayout, UPrimaryGameLayout::StaticClass, TEXT("UPrimaryGameLayout"), &Z_Registration_Info_UClass_UPrimaryGameLayout, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPrimaryGameLayout), 682271318U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_4190008080(TEXT("/Script/CommonGame"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_CommonGame_Source_Public_PrimaryGameLayout_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
