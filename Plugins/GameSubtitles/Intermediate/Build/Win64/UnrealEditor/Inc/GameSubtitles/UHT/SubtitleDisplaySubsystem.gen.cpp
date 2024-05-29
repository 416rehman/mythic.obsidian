// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/SubtitleDisplaySubsystem.h"
#include "Runtime/Engine/Classes/Engine/GameInstance.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSubtitleDisplaySubsystem() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_UGameInstanceSubsystem();
GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplaySubsystem();
GAMESUBTITLES_API UClass* Z_Construct_UClass_USubtitleDisplaySubsystem_NoRegister();
GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity();
GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder();
GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor();
GAMESUBTITLES_API UEnum* Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize();
GAMESUBTITLES_API UScriptStruct* Z_Construct_UScriptStruct_FSubtitleFormat();
UPackage* Z_Construct_UPackage__Script_GameSubtitles();
// End Cross Module References

// Begin ScriptStruct FSubtitleFormat
static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_SubtitleFormat;
class UScriptStruct* FSubtitleFormat::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_SubtitleFormat.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_SubtitleFormat.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FSubtitleFormat, (UObject*)Z_Construct_UPackage__Script_GameSubtitles(), TEXT("SubtitleFormat"));
	}
	return Z_Registration_Info_UScriptStruct_SubtitleFormat.OuterSingleton;
}
template<> GAMESUBTITLES_API UScriptStruct* StaticStruct<FSubtitleFormat>()
{
	return FSubtitleFormat::StaticStruct();
}
struct Z_Construct_UScriptStruct_FSubtitleFormat_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleTextSize_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleTextColor_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleTextBorder_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleBackgroundOpacity_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleTextSize_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleTextSize;
	static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleTextColor_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleTextColor;
	static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleTextBorder_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleTextBorder;
	static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleBackgroundOpacity_Underlying;
	static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleBackgroundOpacity;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static void* NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FSubtitleFormat>();
	}
	static const UECodeGen_Private::FStructParams StructParams;
};
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize = { "SubtitleTextSize", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleTextSize), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SubtitleTextSize_MetaData), NewProp_SubtitleTextSize_MetaData) }; // 4054621106
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor = { "SubtitleTextColor", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleTextColor), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SubtitleTextColor_MetaData), NewProp_SubtitleTextColor_MetaData) }; // 3844635282
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder = { "SubtitleTextBorder", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleTextBorder), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SubtitleTextBorder_MetaData), NewProp_SubtitleTextBorder_MetaData) }; // 4054656008
const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity = { "SubtitleBackgroundOpacity", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleBackgroundOpacity), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SubtitleBackgroundOpacity_MetaData), NewProp_SubtitleBackgroundOpacity_MetaData) }; // 587908002
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FSubtitleFormat_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity_Underlying,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::PropPointers) < 2048);
const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::StructParams = {
	(UObject* (*)())Z_Construct_UPackage__Script_GameSubtitles,
	nullptr,
	&NewStructOps,
	"SubtitleFormat",
	Z_Construct_UScriptStruct_FSubtitleFormat_Statics::PropPointers,
	UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::PropPointers),
	sizeof(FSubtitleFormat),
	alignof(FSubtitleFormat),
	RF_Public|RF_Transient|RF_MarkAsNative,
	EStructFlags(0x00000201),
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FSubtitleFormat_Statics::Struct_MetaDataParams)
};
UScriptStruct* Z_Construct_UScriptStruct_FSubtitleFormat()
{
	if (!Z_Registration_Info_UScriptStruct_SubtitleFormat.InnerSingleton)
	{
		UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_SubtitleFormat.InnerSingleton, Z_Construct_UScriptStruct_FSubtitleFormat_Statics::StructParams);
	}
	return Z_Registration_Info_UScriptStruct_SubtitleFormat.InnerSingleton;
}
// End ScriptStruct FSubtitleFormat

// Begin Class USubtitleDisplaySubsystem
void USubtitleDisplaySubsystem::StaticRegisterNativesUSubtitleDisplaySubsystem()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(USubtitleDisplaySubsystem);
UClass* Z_Construct_UClass_USubtitleDisplaySubsystem_NoRegister()
{
	return USubtitleDisplaySubsystem::StaticClass();
}
struct Z_Construct_UClass_USubtitleDisplaySubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
		{ "DisplayName", "Subtitle Display" },
		{ "IncludePath", "SubtitleDisplaySubsystem.h" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleFormat_MetaData[] = {
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FStructPropertyParams NewProp_SubtitleFormat;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USubtitleDisplaySubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat = { "SubtitleFormat", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplaySubsystem, SubtitleFormat), Z_Construct_UScriptStruct_FSubtitleFormat, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_SubtitleFormat_MetaData), NewProp_SubtitleFormat_MetaData) }; // 898911869
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UGameInstanceSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_GameSubtitles,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::ClassParams = {
	&USubtitleDisplaySubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_USubtitleDisplaySubsystem()
{
	if (!Z_Registration_Info_UClass_USubtitleDisplaySubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_USubtitleDisplaySubsystem.OuterSingleton, Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_USubtitleDisplaySubsystem.OuterSingleton;
}
template<> GAMESUBTITLES_API UClass* StaticClass<USubtitleDisplaySubsystem>()
{
	return USubtitleDisplaySubsystem::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(USubtitleDisplaySubsystem);
USubtitleDisplaySubsystem::~USubtitleDisplaySubsystem() {}
// End Class USubtitleDisplaySubsystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics
{
	static constexpr FStructRegisterCompiledInInfo ScriptStructInfo[] = {
		{ FSubtitleFormat::StaticStruct, Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewStructOps, TEXT("SubtitleFormat"), &Z_Registration_Info_UScriptStruct_SubtitleFormat, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FSubtitleFormat), 898911869U) },
	};
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_USubtitleDisplaySubsystem, USubtitleDisplaySubsystem::StaticClass, TEXT("USubtitleDisplaySubsystem"), &Z_Registration_Info_UClass_USubtitleDisplaySubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USubtitleDisplaySubsystem), 2239785932U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_3654209853(TEXT("/Script/GameSubtitles"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ClassInfo),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ScriptStructInfo),
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
