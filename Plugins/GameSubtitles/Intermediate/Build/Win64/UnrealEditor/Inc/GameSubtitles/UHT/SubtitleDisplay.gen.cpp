// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/Widgets/SubtitleDisplay.h"
#include "Public/SubtitleDisplaySubsystem.h"
#include "Runtime/SlateCore/Public/Styling/SlateBrush.h"
#include "Runtime/SlateCore/Public/Styling/SlateTypes.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSubtitleDisplay() {}

// Begin Cross Module References
GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplay();
GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplay_NoRegister();
GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplayOptions_NoRegister();
GAMESUBTITLES_API UScriptStruct* Z_Construct_UScriptStruct_FSubtitleFormat();
SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FSlateBrush();
SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FTextBlockStyle();
UMG_API UClass* Z_Construct_UClass_UWidget();
UPackage* Z_Construct_UPackage__Script_GameSubtitles();
// End Cross Module References

// Begin Class USubtitleDisplay Function HasSubtitles
struct Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics
{
	struct SubtitleDisplay_eventHasSubtitles_Parms
	{
		bool ReturnValue;
	};
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Function_MetaDataParams[] = {
		{ "Category", "Subtitles" },
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
#if !UE_BUILD_SHIPPING
		{ "Tooltip", "True if there are subtitles currently.  False if the subtitle text is empty." },
#endif
	};
#endif // WITH_METADATA
	static void NewProp_ReturnValue_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_ReturnValue;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static const UECodeGen_Private::FFunctionParams FuncParams;
};
void Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::NewProp_ReturnValue_SetBit(void* Obj)
{
	((SubtitleDisplay_eventHasSubtitles_Parms*)Obj)->ReturnValue = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::NewProp_ReturnValue = { "ReturnValue", nullptr, (EPropertyFlags)0x0010000000000580, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(SubtitleDisplay_eventHasSubtitles_Parms), &Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::NewProp_ReturnValue_SetBit, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::NewProp_ReturnValue,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::PropPointers) < 2048);
const UECodeGen_Private::FFunctionParams Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::FuncParams = { (UObject*(*)())Z_Construct_UClass_USubtitleDisplay, nullptr, "HasSubtitles", nullptr, nullptr, Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::PropPointers, UE_ARRAY_COUNT(Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::PropPointers), sizeof(Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::SubtitleDisplay_eventHasSubtitles_Parms), RF_Public|RF_Transient|RF_MarkAsNative, (EFunctionFlags)0x54020401, 0, 0, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::Function_MetaDataParams), Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::Function_MetaDataParams) };
static_assert(sizeof(Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::SubtitleDisplay_eventHasSubtitles_Parms) < MAX_uint16);
UFunction* Z_Construct_UFunction_USubtitleDisplay_HasSubtitles()
{
	static UFunction* ReturnFunction = nullptr;
	if (!ReturnFunction)
	{
		UECodeGen_Private::ConstructUFunction(&ReturnFunction, Z_Construct_UFunction_USubtitleDisplay_HasSubtitles_Statics::FuncParams);
	}
	return ReturnFunction;
}
DEFINE_FUNCTION(USubtitleDisplay::execHasSubtitles)
{
	P_FINISH;
	P_NATIVE_BEGIN;
	*(bool*)Z_Param__Result=P_THIS->HasSubtitles();
	P_NATIVE_END;
}
// End Class USubtitleDisplay Function HasSubtitles

// Begin Class USubtitleDisplay
void USubtitleDisplay::StaticRegisterNativesUSubtitleDisplay()
{
	UClass* Class = USubtitleDisplay::StaticClass();
	static const FNameNativePtrPair Funcs[] = {
		{ "HasSubtitles", &USubtitleDisplay::execHasSubtitles },
	};
	FNativeFunctionRegistrar::RegisterFunctions(Class, Funcs, UE_ARRAY_COUNT(Funcs));
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(USubtitleDisplay);
UClass* Z_Construct_UClass_USubtitleDisplay_NoRegister()
{
	return USubtitleDisplay::StaticClass();
}
struct Z_Construct_UClass_USubtitleDisplay_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "DisableNativeTick", "" },
		{ "IncludePath", "Widgets/SubtitleDisplay.h" },
		{ "IsBlueprintBase", "true" },
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Format_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Options_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_WrapTextAt_MetaData[] = {
		{ "Category", "Display Info" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs.\n" },
#endif
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether text wraps onto a new line when it's length exceeds this width; if this value is zero or negative, no wrapping occurs." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_bPreviewMode_MetaData[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Preview text to be displayed when designing the widget */" },
#endif
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Preview text to be displayed when designing the widget" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PreviewText_MetaData[] = {
		{ "Category", "Preview" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Preview text to be displayed when designing the widget */" },
#endif
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Preview text to be displayed when designing the widget" },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedStyle_MetaData[] = {
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_GeneratedBackgroundBorder_MetaData[] = {
		{ "ModuleRelativePath", "Public/Widgets/SubtitleDisplay.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_Format;
	static const UECodeGen_Private::FObjectPropertyParams NewProp_Options;
	static const UECodeGen_Private::FFloatPropertyParams NewProp_WrapTextAt;
	static void NewProp_bPreviewMode_SetBit(void* Obj);
	static const UECodeGen_Private::FBoolPropertyParams NewProp_bPreviewMode;
	static const UECodeGen_Private::FTextPropertyParams NewProp_PreviewText;
	static const UECodeGen_Private::FStructPropertyParams NewProp_GeneratedStyle;
	static const UECodeGen_Private::FStructPropertyParams NewProp_GeneratedBackgroundBorder;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FClassFunctionLinkInfo FuncInfo[] = {
		{ &Z_Construct_UFunction_USubtitleDisplay_HasSubtitles, "HasSubtitles" }, // 1970154791
	};
	static_assert(UE_ARRAY_COUNT(FuncInfo) < 2048);
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USubtitleDisplay>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_Format = { "Format", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplay, Format), Z_Construct_UScriptStruct_FSubtitleFormat, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Format_MetaData), NewProp_Format_MetaData) }; // 898911869
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_Options = { "Options", nullptr, (EPropertyFlags)0x0114000000000001, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplay, Options), Z_Construct_UClass_USubtitleDisplayOptions_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Options_MetaData), NewProp_Options_MetaData) };
const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_WrapTextAt = { "WrapTextAt", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplay, WrapTextAt), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_WrapTextAt_MetaData), NewProp_WrapTextAt_MetaData) };
void Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_bPreviewMode_SetBit(void* Obj)
{
	((USubtitleDisplay*)Obj)->bPreviewMode = 1;
}
const UECodeGen_Private::FBoolPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_bPreviewMode = { "bPreviewMode", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(USubtitleDisplay), &Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_bPreviewMode_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_bPreviewMode_MetaData), NewProp_bPreviewMode_MetaData) };
const UECodeGen_Private::FTextPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_PreviewText = { "PreviewText", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Text, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplay, PreviewText), METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PreviewText_MetaData), NewProp_PreviewText_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_GeneratedStyle = { "GeneratedStyle", nullptr, (EPropertyFlags)0x0040000000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplay, GeneratedStyle), Z_Construct_UScriptStruct_FTextBlockStyle, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GeneratedStyle_MetaData), NewProp_GeneratedStyle_MetaData) }; // 4086975916
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_GeneratedBackgroundBorder = { "GeneratedBackgroundBorder", nullptr, (EPropertyFlags)0x0040000000002000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplay, GeneratedBackgroundBorder), Z_Construct_UScriptStruct_FSlateBrush, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_GeneratedBackgroundBorder_MetaData), NewProp_GeneratedBackgroundBorder_MetaData) }; // 1704263518
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USubtitleDisplay_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_Format,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_Options,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_WrapTextAt,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_bPreviewMode,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_PreviewText,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_GeneratedStyle,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplay_Statics::NewProp_GeneratedBackgroundBorder,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplay_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_USubtitleDisplay_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UWidget,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSubtitles,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplay_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USubtitleDisplay_Statics::ClassParams = {
	&USubtitleDisplay::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	FuncInfo,
	Z_Construct_UClass_USubtitleDisplay_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	UE_ARRAY_COUNT(FuncInfo),
	UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplay_Statics::PropPointers),
	0,
	0x00B000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplay_Statics::Class_MetaDataParams), Z_Construct_UClass_USubtitleDisplay_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_USubtitleDisplay()
{
	if (!Z_Registration_Info_UClass_USubtitleDisplay.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USubtitleDisplay.OuterSingleton, Z_Construct_UClass_USubtitleDisplay_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USubtitleDisplay.OuterSingleton;
}
template<> GAMESUBTITLES_API UClass* StaticClass<USubtitleDisplay>()
{
	return USubtitleDisplay::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(USubtitleDisplay);
USubtitleDisplay::~USubtitleDisplay() {}
// End Class USubtitleDisplay

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Widgets_SubtitleDisplay_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USubtitleDisplay, USubtitleDisplay::StaticClass, TEXT("USubtitleDisplay"), &Z_Registration_Info_UClass_USubtitleDisplay, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USubtitleDisplay), 708966923U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Widgets_SubtitleDisplay_h_604726525(TEXT("/Script/GameSubtitles"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Widgets_SubtitleDisplay_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_Widgets_SubtitleDisplay_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
