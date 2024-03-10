// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Public/Input/IAimAssistTargetInterface.h"
#include "GameplayTagContainer.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeIAimAssistTargetInterface() {}
// Cross Module References
	COREUOBJECT_API UClass* Z_Construct_UClass_UInterface();
	GAMEPLAYTAGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameplayTagContainer();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistTaget();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAimAssistTaget_NoRegister();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FAimAssistTargetOptions();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_AimAssistTargetOptions;
class UScriptStruct* FAimAssistTargetOptions::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_AimAssistTargetOptions.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_AimAssistTargetOptions.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FAimAssistTargetOptions, (UObject*)Z_Construct_UPackage__Script_ShooterCoreRuntime(), TEXT("AimAssistTargetOptions"));
	}
	return Z_Registration_Info_UScriptStruct_AimAssistTargetOptions.OuterSingleton;
}
template<> SHOOTERCORERUNTIME_API UScriptStruct* StaticStruct<FAimAssistTargetOptions>()
{
	return FAimAssistTargetOptions::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AssociatedTags_MetaData[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_AssociatedTags;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIsActive_MetaData[];
#endif
		static void NewProp_bIsActive_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIsActive;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::Struct_MetaDataParams[] = {
		{ "BlueprintType", "true" },
		{ "ModuleRelativePath", "Public/Input/IAimAssistTargetInterface.h" },
	};
#endif
	void* Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FAimAssistTargetOptions>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_AssociatedTags_MetaData[] = {
		{ "Category", "AimAssistTargetOptions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n\x09 * Gameplay tags that are associated with this target that can be used to filter it out.\n\x09 *\n\x09 * If the player's aim assist settings have any tags that match these, it will be excluded.\n\x09 */" },
#endif
		{ "ModuleRelativePath", "Public/Input/IAimAssistTargetInterface.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Gameplay tags that are associated with this target that can be used to filter it out.\n\nIf the player's aim assist settings have any tags that match these, it will be excluded." },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_AssociatedTags = { "AssociatedTags", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FAimAssistTargetOptions, AssociatedTags), Z_Construct_UScriptStruct_FGameplayTagContainer, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_AssociatedTags_MetaData), Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_AssociatedTags_MetaData) }; // 405371792
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive_MetaData[] = {
		{ "Category", "AimAssistTargetOptions" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "/** Whether or not this target is currently active. If false, it will not be considered for aim assist */" },
#endif
		{ "ModuleRelativePath", "Public/Input/IAimAssistTargetInterface.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Whether or not this target is currently active. If false, it will not be considered for aim assist" },
#endif
	};
#endif
	void Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive_SetBit(void* Obj)
	{
		((FAimAssistTargetOptions*)Obj)->bIsActive = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive = { "bIsActive", nullptr, (EPropertyFlags)0x0010000000000005, UECodeGen_Private::EPropertyGenFlags::Bool , RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(uint8), sizeof(FAimAssistTargetOptions), &Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive_MetaData), Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_AssociatedTags,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewProp_bIsActive,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
		nullptr,
		&NewStructOps,
		"AimAssistTargetOptions",
		Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::PropPointers),
		sizeof(FAimAssistTargetOptions),
		alignof(FAimAssistTargetOptions),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FAimAssistTargetOptions()
	{
		if (!Z_Registration_Info_UScriptStruct_AimAssistTargetOptions.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_AimAssistTargetOptions.InnerSingleton, Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_AimAssistTargetOptions.InnerSingleton;
	}
	void UAimAssistTaget::StaticRegisterNativesUAimAssistTaget()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAimAssistTaget);
	UClass* Z_Construct_UClass_UAimAssistTaget_NoRegister()
	{
		return UAimAssistTaget::StaticClass();
	}
	struct Z_Construct_UClass_UAimAssistTaget_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAimAssistTaget_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UInterface,
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTaget_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAimAssistTaget_Statics::Class_MetaDataParams[] = {
		{ "CannotImplementInterfaceInBlueprint", "" },
		{ "ModuleRelativePath", "Public/Input/IAimAssistTargetInterface.h" },
	};
#endif
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAimAssistTaget_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<IAimAssistTaget>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAimAssistTaget_Statics::ClassParams = {
		&UAimAssistTaget::StaticClass,
		nullptr,
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		nullptr,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		0,
		0,
		0x000840A1u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAimAssistTaget_Statics::Class_MetaDataParams), Z_Construct_UClass_UAimAssistTaget_Statics::Class_MetaDataParams)
	};
	UClass* Z_Construct_UClass_UAimAssistTaget()
	{
		if (!Z_Registration_Info_UClass_UAimAssistTaget.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAimAssistTaget.OuterSingleton, Z_Construct_UClass_UAimAssistTaget_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAimAssistTaget.OuterSingleton;
	}
	template<> SHOOTERCORERUNTIME_API UClass* StaticClass<UAimAssistTaget>()
	{
		return UAimAssistTaget::StaticClass();
	}
	UAimAssistTaget::UAimAssistTaget(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAimAssistTaget);
	UAimAssistTaget::~UAimAssistTaget() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics::ScriptStructInfo[] = {
		{ FAimAssistTargetOptions::StaticStruct, Z_Construct_UScriptStruct_FAimAssistTargetOptions_Statics::NewStructOps, TEXT("AimAssistTargetOptions"), &Z_Registration_Info_UScriptStruct_AimAssistTargetOptions, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FAimAssistTargetOptions), 2137094904U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAimAssistTaget, UAimAssistTaget::StaticClass, TEXT("UAimAssistTaget"), &Z_Registration_Info_UClass_UAimAssistTaget, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAimAssistTaget), 692755740U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_2783757247(TEXT("/Script/ShooterCoreRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_Input_IAimAssistTargetInterface_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
