// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/PocketLevelSystem.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodePocketLevelSystem() {}

// Begin Cross Module References
ENGINE_API UClass* Z_Construct_UClass_UWorldSubsystem();
POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevelInstance_NoRegister();
POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevelSubsystem();
POCKETWORLDS_API UClass* Z_Construct_UClass_UPocketLevelSubsystem_NoRegister();
UPackage* Z_Construct_UPackage__Script_PocketWorlds();
// End Cross Module References

// Begin Class UPocketLevelSubsystem
void UPocketLevelSubsystem::StaticRegisterNativesUPocketLevelSubsystem()
{
}
IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UPocketLevelSubsystem);
UClass* Z_Construct_UClass_UPocketLevelSubsystem_NoRegister()
{
	return UPocketLevelSubsystem::StaticClass();
}
struct Z_Construct_UClass_UPocketLevelSubsystem_Statics
{
#if WITH_METADATA
	static constexpr UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n *\n */" },
#endif
		{ "IncludePath", "PocketLevelSystem.h" },
		{ "ModuleRelativePath", "Public/PocketLevelSystem.h" },
	};
	static constexpr UECodeGen_Private::FMetaDataPairParam NewProp_PocketInstances_MetaData[] = {
		{ "ModuleRelativePath", "Public/PocketLevelSystem.h" },
	};
#endif // WITH_METADATA
	static const UECodeGen_Private::FObjectPropertyParams NewProp_PocketInstances_Inner;
	static const UECodeGen_Private::FArrayPropertyParams NewProp_PocketInstances;
	static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
	static UObject* (*const DependentSingletons[])();
	static constexpr FCppClassTypeInfoStatic StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UPocketLevelSubsystem>::IsAbstract,
	};
	static const UECodeGen_Private::FClassParams ClassParams;
};
const UECodeGen_Private::FObjectPropertyParams Z_Construct_UClass_UPocketLevelSubsystem_Statics::NewProp_PocketInstances_Inner = { "PocketInstances", nullptr, (EPropertyFlags)0x0104000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UPocketLevelInstance_NoRegister, METADATA_PARAMS(0, nullptr) };
const UECodeGen_Private::FArrayPropertyParams Z_Construct_UClass_UPocketLevelSubsystem_Statics::NewProp_PocketInstances = { "PocketInstances", nullptr, (EPropertyFlags)0x0144000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UPocketLevelSubsystem, PocketInstances), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(NewProp_PocketInstances_MetaData), NewProp_PocketInstances_MetaData) };
const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UPocketLevelSubsystem_Statics::PropPointers[] = {
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevelSubsystem_Statics::NewProp_PocketInstances_Inner,
	(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UPocketLevelSubsystem_Statics::NewProp_PocketInstances,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelSubsystem_Statics::PropPointers) < 2048);
UObject* (*const Z_Construct_UClass_UPocketLevelSubsystem_Statics::DependentSingletons[])() = {
	(UObject* (*)())Z_Construct_UClass_UWorldSubsystem,
	(UObject* (*)())Z_Construct_UPackage__Script_PocketWorlds,
};
static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelSubsystem_Statics::DependentSingletons) < 16);
const UECodeGen_Private::FClassParams Z_Construct_UClass_UPocketLevelSubsystem_Statics::ClassParams = {
	&UPocketLevelSubsystem::StaticClass,
	nullptr,
	&StaticCppClassTypeInfo,
	DependentSingletons,
	nullptr,
	Z_Construct_UClass_UPocketLevelSubsystem_Statics::PropPointers,
	nullptr,
	UE_ARRAY_COUNT(DependentSingletons),
	0,
	UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelSubsystem_Statics::PropPointers),
	0,
	0x001000A0u,
	METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UPocketLevelSubsystem_Statics::Class_MetaDataParams), Z_Construct_UClass_UPocketLevelSubsystem_Statics::Class_MetaDataParams)
};
UClass* Z_Construct_UClass_UPocketLevelSubsystem()
{
	if (!Z_Registration_Info_UClass_UPocketLevelSubsystem.OuterSingleton)
	{
		UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UPocketLevelSubsystem.OuterSingleton, Z_Construct_UClass_UPocketLevelSubsystem_Statics::ClassParams);
	}
	return Z_Registration_Info_UClass_UPocketLevelSubsystem.OuterSingleton;
}
template<> POCKETWORLDS_API UClass* StaticClass<UPocketLevelSubsystem>()
{
	return UPocketLevelSubsystem::StaticClass();
}
UPocketLevelSubsystem::UPocketLevelSubsystem() {}
DEFINE_VTABLE_PTR_HELPER_CTOR(UPocketLevelSubsystem);
UPocketLevelSubsystem::~UPocketLevelSubsystem() {}
// End Class UPocketLevelSubsystem

// Begin Registration
struct Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_Statics
{
	static constexpr FClassRegisterCompiledInInfo ClassInfo[] = {
		{ Z_Construct_UClass_UPocketLevelSubsystem, UPocketLevelSubsystem::StaticClass, TEXT("UPocketLevelSubsystem"), &Z_Registration_Info_UClass_UPocketLevelSubsystem, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UPocketLevelSubsystem), 2762088548U) },
	};
};
static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_2948742409(TEXT("/Script/PocketWorlds"),
	Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_PocketWorlds_Source_Public_PocketLevelSystem_h_Statics::ClassInfo),
	nullptr, 0,
	nullptr, 0);
// End Registration
PRAGMA_ENABLE_DEPRECATION_WARNINGS
