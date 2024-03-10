// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/SubtitleDisplayOptions.h"
#include "Fonts/SlateFontInfo.h"
#include "Styling/SlateBrush.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSubtitleDisplayOptions() {}
// Cross Module References
	COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FLinearColor();
	ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
	GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplayOptions();
	GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplayOptions_NoRegister();
	GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity();
	GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder();
	GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor();
	GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize();
	SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FSlateBrush();
	SLATECORE_API UScriptStruct* Z_Construct_UScriptStruct_FSlateFontInfo();
	UPackage* Z_Construct_UPackage__Script_GameSubtitles();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ESubtitleDisplayTextSize;
	static UEnum* ESubtitleDisplayTextSize_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayTextSize.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ESubtitleDisplayTextSize.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize, (UObject*)Z_Construct_UPackage__Script_GameSubtitles(), TEXT("ESubtitleDisplayTextSize"));
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayTextSize.OuterSingleton;
	}
	template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayTextSize>()
	{
		return ESubtitleDisplayTextSize_StaticEnum();
	}
	struct Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::Enumerators[] = {
		{ "ESubtitleDisplayTextSize::ExtraSmall", (int64)ESubtitleDisplayTextSize::ExtraSmall },
		{ "ESubtitleDisplayTextSize::Small", (int64)ESubtitleDisplayTextSize::Small },
		{ "ESubtitleDisplayTextSize::Medium", (int64)ESubtitleDisplayTextSize::Medium },
		{ "ESubtitleDisplayTextSize::Large", (int64)ESubtitleDisplayTextSize::Large },
		{ "ESubtitleDisplayTextSize::ExtraLarge", (int64)ESubtitleDisplayTextSize::ExtraLarge },
		{ "ESubtitleDisplayTextSize::ESubtitleDisplayTextSize_MAX", (int64)ESubtitleDisplayTextSize::ESubtitleDisplayTextSize_MAX },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::Enum_MetaDataParams[] = {
		{ "ESubtitleDisplayTextSize_MAX.Name", "ESubtitleDisplayTextSize::ESubtitleDisplayTextSize_MAX" },
		{ "ExtraLarge.Name", "ESubtitleDisplayTextSize::ExtraLarge" },
		{ "ExtraSmall.Name", "ESubtitleDisplayTextSize::ExtraSmall" },
		{ "Large.Name", "ESubtitleDisplayTextSize::Large" },
		{ "Medium.Name", "ESubtitleDisplayTextSize::Medium" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
		{ "Small.Name", "ESubtitleDisplayTextSize::Small" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_GameSubtitles,
		nullptr,
		"ESubtitleDisplayTextSize",
		"ESubtitleDisplayTextSize",
		Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::Enum_MetaDataParams), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayTextSize.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ESubtitleDisplayTextSize.InnerSingleton, Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayTextSize.InnerSingleton;
	}
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ESubtitleDisplayTextColor;
	static UEnum* ESubtitleDisplayTextColor_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayTextColor.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ESubtitleDisplayTextColor.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor, (UObject*)Z_Construct_UPackage__Script_GameSubtitles(), TEXT("ESubtitleDisplayTextColor"));
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayTextColor.OuterSingleton;
	}
	template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayTextColor>()
	{
		return ESubtitleDisplayTextColor_StaticEnum();
	}
	struct Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::Enumerators[] = {
		{ "ESubtitleDisplayTextColor::White", (int64)ESubtitleDisplayTextColor::White },
		{ "ESubtitleDisplayTextColor::Yellow", (int64)ESubtitleDisplayTextColor::Yellow },
		{ "ESubtitleDisplayTextColor::ESubtitleDisplayTextColor_MAX", (int64)ESubtitleDisplayTextColor::ESubtitleDisplayTextColor_MAX },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::Enum_MetaDataParams[] = {
		{ "ESubtitleDisplayTextColor_MAX.Name", "ESubtitleDisplayTextColor::ESubtitleDisplayTextColor_MAX" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
		{ "White.Name", "ESubtitleDisplayTextColor::White" },
		{ "Yellow.Name", "ESubtitleDisplayTextColor::Yellow" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_GameSubtitles,
		nullptr,
		"ESubtitleDisplayTextColor",
		"ESubtitleDisplayTextColor",
		Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::Enum_MetaDataParams), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayTextColor.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ESubtitleDisplayTextColor.InnerSingleton, Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayTextColor.InnerSingleton;
	}
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder;
	static UEnum* ESubtitleDisplayTextBorder_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder, (UObject*)Z_Construct_UPackage__Script_GameSubtitles(), TEXT("ESubtitleDisplayTextBorder"));
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder.OuterSingleton;
	}
	template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayTextBorder>()
	{
		return ESubtitleDisplayTextBorder_StaticEnum();
	}
	struct Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::Enumerators[] = {
		{ "ESubtitleDisplayTextBorder::None", (int64)ESubtitleDisplayTextBorder::None },
		{ "ESubtitleDisplayTextBorder::Outline", (int64)ESubtitleDisplayTextBorder::Outline },
		{ "ESubtitleDisplayTextBorder::DropShadow", (int64)ESubtitleDisplayTextBorder::DropShadow },
		{ "ESubtitleDisplayTextBorder::ESubtitleDisplayTextBorder_MAX", (int64)ESubtitleDisplayTextBorder::ESubtitleDisplayTextBorder_MAX },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::Enum_MetaDataParams[] = {
		{ "DropShadow.Name", "ESubtitleDisplayTextBorder::DropShadow" },
		{ "ESubtitleDisplayTextBorder_MAX.Name", "ESubtitleDisplayTextBorder::ESubtitleDisplayTextBorder_MAX" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
		{ "None.Name", "ESubtitleDisplayTextBorder::None" },
		{ "Outline.Name", "ESubtitleDisplayTextBorder::Outline" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_GameSubtitles,
		nullptr,
		"ESubtitleDisplayTextBorder",
		"ESubtitleDisplayTextBorder",
		Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::Enum_MetaDataParams), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder.InnerSingleton, Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder.InnerSingleton;
	}
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity;
	static UEnum* ESubtitleDisplayBackgroundOpacity_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity.OuterSingleton)
		{
			Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity, (UObject*)Z_Construct_UPackage__Script_GameSubtitles(), TEXT("ESubtitleDisplayBackgroundOpacity"));
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity.OuterSingleton;
	}
	template<> GAMESUBTITLES_API UEnum* StaticEnum<ESubtitleDisplayBackgroundOpacity>()
	{
		return ESubtitleDisplayBackgroundOpacity_StaticEnum();
	}
	struct Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::Enumerators[] = {
		{ "ESubtitleDisplayBackgroundOpacity::Clear", (int64)ESubtitleDisplayBackgroundOpacity::Clear },
		{ "ESubtitleDisplayBackgroundOpacity::Low", (int64)ESubtitleDisplayBackgroundOpacity::Low },
		{ "ESubtitleDisplayBackgroundOpacity::Medium", (int64)ESubtitleDisplayBackgroundOpacity::Medium },
		{ "ESubtitleDisplayBackgroundOpacity::High", (int64)ESubtitleDisplayBackgroundOpacity::High },
		{ "ESubtitleDisplayBackgroundOpacity::Solid", (int64)ESubtitleDisplayBackgroundOpacity::Solid },
		{ "ESubtitleDisplayBackgroundOpacity::ESubtitleDisplayBackgroundOpacity_MAX", (int64)ESubtitleDisplayBackgroundOpacity::ESubtitleDisplayBackgroundOpacity_MAX },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::Enum_MetaDataParams[] = {
		{ "Clear.Name", "ESubtitleDisplayBackgroundOpacity::Clear" },
		{ "ESubtitleDisplayBackgroundOpacity_MAX.Name", "ESubtitleDisplayBackgroundOpacity::ESubtitleDisplayBackgroundOpacity_MAX" },
		{ "High.Name", "ESubtitleDisplayBackgroundOpacity::High" },
		{ "Low.Name", "ESubtitleDisplayBackgroundOpacity::Low" },
		{ "Medium.Name", "ESubtitleDisplayBackgroundOpacity::Medium" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
		{ "Solid.Name", "ESubtitleDisplayBackgroundOpacity::Solid" },
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_GameSubtitles,
		nullptr,
		"ESubtitleDisplayBackgroundOpacity",
		"ESubtitleDisplayBackgroundOpacity",
		Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::Enum_MetaDataParams), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity()
	{
		if (!Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity.InnerSingleton, Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity.InnerSingleton;
	}
	void USubtitleDisplayOptions::StaticRegisterNativesUSubtitleDisplayOptions()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(USubtitleDisplayOptions);
	UClass* Z_Construct_UClass_USubtitleDisplayOptions_NoRegister()
	{
		return USubtitleDisplayOptions::StaticClass();
	}
	struct Z_Construct_UClass_USubtitleDisplayOptions_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_Font_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_Font;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DisplayTextSizes_MetaData[];
#endif
		static const UECodeGen_Private::FIntPropertyParams NewProp_DisplayTextSizes;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DisplayTextColors_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_DisplayTextColors;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DisplayBorderSize_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_DisplayBorderSize;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DisplayBackgroundOpacity_MetaData[];
#endif
		static const UECodeGen_Private::FFloatPropertyParams NewProp_DisplayBackgroundOpacity;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_BackgroundBrush_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_BackgroundBrush;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_USubtitleDisplayOptions_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UDataAsset,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSubtitles,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::Class_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "SubtitleDisplayOptions.h" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_Font_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_Font = { "Font", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplayOptions, Font), Z_Construct_UScriptStruct_FSlateFontInfo, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_Font_MetaData), Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_Font_MetaData) }; // 2074891513
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextSizes_MetaData[] = {
		{ "ArraySizeEnum", "/Script/GameSubtitles.ESubtitleDisplayTextSize" },
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
	const UECodeGen_Private::FIntPropertyParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextSizes = { "DisplayTextSizes", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Int, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, CPP_ARRAY_DIM(DisplayTextSizes, USubtitleDisplayOptions), STRUCT_OFFSET(USubtitleDisplayOptions, DisplayTextSizes), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextSizes_MetaData), Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextSizes_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextColors_MetaData[] = {
		{ "ArraySizeEnum", "/Script/GameSubtitles.ESubtitleDisplayTextColor" },
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextColors = { "DisplayTextColors", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, CPP_ARRAY_DIM(DisplayTextColors, USubtitleDisplayOptions), STRUCT_OFFSET(USubtitleDisplayOptions, DisplayTextColors), Z_Construct_UScriptStruct_FLinearColor, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextColors_MetaData), Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextColors_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBorderSize_MetaData[] = {
		{ "ArraySizeEnum", "/Script/GameSubtitles.ESubtitleDisplayTextBorder" },
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBorderSize = { "DisplayBorderSize", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, CPP_ARRAY_DIM(DisplayBorderSize, USubtitleDisplayOptions), STRUCT_OFFSET(USubtitleDisplayOptions, DisplayBorderSize), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBorderSize_MetaData), Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBorderSize_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBackgroundOpacity_MetaData[] = {
		{ "ArraySizeEnum", "/Script/GameSubtitles.ESubtitleDisplayBackgroundOpacity" },
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBackgroundOpacity = { "DisplayBackgroundOpacity", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, CPP_ARRAY_DIM(DisplayBackgroundOpacity, USubtitleDisplayOptions), STRUCT_OFFSET(USubtitleDisplayOptions, DisplayBackgroundOpacity), METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBackgroundOpacity_MetaData), Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBackgroundOpacity_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_BackgroundBrush_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplayOptions.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_BackgroundBrush = { "BackgroundBrush", nullptr, (EPropertyFlags)0x0010000000010001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplayOptions, BackgroundBrush), Z_Construct_UScriptStruct_FSlateBrush, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_BackgroundBrush_MetaData), Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_BackgroundBrush_MetaData) }; // 899956612
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USubtitleDisplayOptions_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_Font,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextSizes,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayTextColors,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBorderSize,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_DisplayBackgroundOpacity,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplayOptions_Statics::NewProp_BackgroundBrush,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_USubtitleDisplayOptions_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USubtitleDisplayOptions>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_USubtitleDisplayOptions_Statics::ClassParams = {
		&USubtitleDisplayOptions::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_USubtitleDisplayOptions_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::PropPointers),
		0,
		0x001000A0u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::Class_MetaDataParams), Z_Construct_UClass_USubtitleDisplayOptions_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplayOptions_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_USubtitleDisplayOptions()
	{
		if (!Z_Registration_Info_UClass_USubtitleDisplayOptions.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USubtitleDisplayOptions.OuterSingleton, Z_Construct_UClass_USubtitleDisplayOptions_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_USubtitleDisplayOptions.OuterSingleton;
	}
	template<> GAMESUBTITLES_API UClass* StaticClass<USubtitleDisplayOptions>()
	{
		return USubtitleDisplayOptions::StaticClass();
	}
	DEFINE_VTABLE_PTR_HELPER_CTOR(USubtitleDisplayOptions);
	USubtitleDisplayOptions::~USubtitleDisplayOptions() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics::EnumInfo[] = {
		{ ESubtitleDisplayTextSize_StaticEnum, TEXT("ESubtitleDisplayTextSize"), &Z_Registration_Info_UEnum_ESubtitleDisplayTextSize, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 269681562U) },
		{ ESubtitleDisplayTextColor_StaticEnum, TEXT("ESubtitleDisplayTextColor"), &Z_Registration_Info_UEnum_ESubtitleDisplayTextColor, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 728453195U) },
		{ ESubtitleDisplayTextBorder_StaticEnum, TEXT("ESubtitleDisplayTextBorder"), &Z_Registration_Info_UEnum_ESubtitleDisplayTextBorder, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 3253258739U) },
		{ ESubtitleDisplayBackgroundOpacity_StaticEnum, TEXT("ESubtitleDisplayBackgroundOpacity"), &Z_Registration_Info_UEnum_ESubtitleDisplayBackgroundOpacity, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 2257676000U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_USubtitleDisplayOptions, USubtitleDisplayOptions::StaticClass, TEXT("USubtitleDisplayOptions"), &Z_Registration_Info_UClass_USubtitleDisplayOptions, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USubtitleDisplayOptions), 704192751U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_4139702386(TEXT("/Script/GameSubtitles"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics::ClassInfo),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplayOptions_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
