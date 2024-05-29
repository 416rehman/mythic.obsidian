// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PocketLevel.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePocketLevel() {}

// Begin Cross Module References
COREUOBJECT_API UScriptStruct* Z_Construct_UScriptStruct_FVector();
ENGINE_API UClass* Z_Construct_UClass_UDataAsset();
ENGINE_API UClass* Z_Construct_UClass_UWorld_NoRegister();
POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevel();
POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevel_NoRegister();
UPackage* Z_Construct_UPackage__Script_PocketWorlds();
// End Cross Module References

// Begin Class UPocketLevel
void UPocketLevel::StaticRegisterNativesUPocketLevel()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPocketLevel);
UClass* Z_Construct_UClass_UPocketLevel_NoRegister()
{
	return UPocketLevel::StaticClass();
}
struct Z_Construct_UClass_UPocketLevel_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * \n */" },
#endif
		{ "IncludePath", "PocketLevel.h" },
		{ "ModuleRelativePath", "Public/PocketLevel.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Level_MetaData[] = {
		{ "Category", "Streaming" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The level that will be streamed in for this pocket level.\n" },
#endif
		{ "ModuleRelativePath", "Public/PocketLevel.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The level that will be streamed in for this pocket level." },
#endif
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_Bounds_MetaData[] = {
		{ "Category", "Streaming" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// The bounds of the pocket level so that we can create multiple instances without overlapping each other.\n" },
#endif
		{ "ModuleRelativePath", "Public/PocketLevel.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The bounds of the pocket level so that we can create multiple instances without overlapping each other." },
#endif
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FSoftObjectPropertyParams NewProp_Level;
	static const UECodeGen_Private::FStructPropertyParams NewProp_Bounds;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPocketLevel>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FSoftObjectPropertyParams Z_Construct_UClass_UPocketLevel_Statics::NewProp_Level = { "Level", nullptr, (EPropertyFlags)0x0014000000000001, UECodeGen_Private::EPropertyGenFlags::SoftObject, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevel, Level), Z_Construct_UClass_UWorld_NoRegister, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Level_MetaData), NewProp_Level_MetaData) };
const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UPocketLevel_Statics::NewProp_Bounds = { "Bounds", nullptr, (EPropertyFlags)0x0010000000000001, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevel, Bounds), Z_Construct_UScriptStruct_FVector, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_Bounds_MetaData), NewProp_Bounds_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPocketLevel_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevel_Statics::NewProp_Level,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevel_Statics::NewProp_Bounds,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevel_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UPocketLevel_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UDataAsset,
	(UObject* (*)())Z_Construct_UPackage__Script_PocketWorlds,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevel_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UPocketLevel_Statics::ClassParams = {
	&UPocketLevel::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UPocketLevel_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevel_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevel_Statics::Class_MetaDataParams), Z_Construct_UClass_UPocketLevel_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UPocketLevel()
{
	if (!Z_Registration_Info_UClass_UPocketLevel.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPocketLevel.OuterSingleton, Z_Construct_UClass_UPocketLevel_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UPocketLevel.OuterSingleton;
}
template<> POCKETWORLDS_API UClass* StaticClass<UPocketLevel>()
{
	return UPocketLevel::StaticClass();
}
DEFINE_VTABLE_PTR_HELPER_CTOR(UPocketLevel);
UPocketLevel::~UPocketLevel() {}
// End Class UPocketLevel

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevel_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UPocketLevel, UPocketLevel::StaticClass, TEXT("UPocketLevel"), &Z_Registration_Info_UClass_UPocketLevel, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPocketLevel), 2537423078U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevel_h_3582480902(TEXT("/Script/PocketWorlds"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevel_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevel_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
