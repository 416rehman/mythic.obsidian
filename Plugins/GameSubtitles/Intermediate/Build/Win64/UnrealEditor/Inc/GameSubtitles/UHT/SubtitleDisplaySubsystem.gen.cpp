// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/SubtitleDisplaySubsystem.h"
#include "../../Source/Runtime/Engine/Classes/Engine/GameInstance.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeSubtitleDisplaySubsystem() {}
// Cross Module References
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
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleTextSize_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleTextSize_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleTextSize;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleTextColor_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleTextColor_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleTextColor;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleTextBorder_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleTextBorder_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleTextBorder;
		static const UECodeGen_Private::FBytePropertyParams NewProp_SubtitleBackgroundOpacity_Underlying;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleBackgroundOpacity_MetaData[];
#endif
		static const UECodeGen_Private::FEnumPropertyParams NewProp_SubtitleBackgroundOpacity;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FSubtitleFormat_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FSubtitleFormat>();
	}
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize = { "SubtitleTextSize", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleTextSize), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextSize, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize_MetaData), Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextSize_MetaData) }; // 269681562
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor = { "SubtitleTextColor", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleTextColor), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextColor, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor_MetaData), Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextColor_MetaData) }; // 728453195
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder = { "SubtitleTextBorder", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleTextBorder), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayTextBorder, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder_MetaData), Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleTextBorder_MetaData) }; // 3253258739
	const UECodeGen_Private::FBytePropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity_Underlying = { "UnderlyingType", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Byte, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, nullptr, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity_MetaData[] = {
		{ "Category", "Display Info" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
	const UECodeGen_Private::FEnumPropertyParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity = { "SubtitleBackgroundOpacity", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Enum, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FSubtitleFormat, SubtitleBackgroundOpacity), Z_Construct_UEnum_GameSubtitles_ESubtitleDisplayBackgroundOpacity, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity_MetaData), Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewProp_SubtitleBackgroundOpacity_MetaData) }; // 2257676000
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
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FSubtitleFormat_Statics::ReturnStructParams = {
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
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FSubtitleFormat_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FSubtitleFormat()
	{
		if (!Z_Registration_Info_UScriptStruct_SubtitleFormat.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_SubtitleFormat.InnerSingleton, Z_Construct_UScriptStruct_FSubtitleFormat_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_SubtitleFormat.InnerSingleton;
	}
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
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SubtitleFormat_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_SubtitleFormat;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameInstanceSubsystem,
		(UObject* (*)())Z_Construct_UPackage__Script_GameSubtitles,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::Class_MetaDataParams[] = {
		{ "DisplayName", "Subtitle Display" },
		{ "IncludePath", "SubtitleDisplaySubsystem.h" },
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat_MetaData[] = {
		{ "ModuleRelativePath", "Public/SubtitleDisplaySubsystem.h" },
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat = { "SubtitleFormat", nullptr, (EPropertyFlags)0x0040000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(USubtitleDisplaySubsystem, SubtitleFormat), Z_Construct_UScriptStruct_FSubtitleFormat, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat_MetaData), Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat_MetaData) }; // 4240008095
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::NewProp_SubtitleFormat,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<USubtitleDisplaySubsystem>::IsAbstract,
	};
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
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_USubtitleDisplaySubsystem_Statics::PropPointers) < 2048);
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
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ScriptStructInfo[] = {
		{ FSubtitleFormat::StaticStruct, Z_Construct_UScriptStruct_FSubtitleFormat_Statics::NewStructOps, TEXT("SubtitleFormat"), &Z_Registration_Info_UScriptStruct_SubtitleFormat, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FSubtitleFormat), 4240008095U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_USubtitleDisplaySubsystem, USubtitleDisplaySubsystem::StaticClass, TEXT("USubtitleDisplaySubsystem"), &Z_Registration_Info_UClass_USubtitleDisplaySubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(USubtitleDisplaySubsystem), 1919822693U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_3820274784(TEXT("/Script/GameSubtitles"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSubtitles_Source_Public_SubtitleDisplaySubsystem_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
