// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "Public/GameSettingFilterState.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameSettingFilterState() {}
// Cross Module References
	GAMESETTINGS_API UClass* Z_Construct_UClass_UGameSetting_NoRegister();
	GAMESETTINGS_API UScriptStruct* Z_Construct_UScriptStruct_FGameSettingFilterState();
	UPackage* Z_Construct_UPackage__Script_GameSettings();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_GameSettingFilterState;
class UScriptStruct* FGameSettingFilterState::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_GameSettingFilterState.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_GameSettingFilterState.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FGameSettingFilterState, (UObject*)Z_Construct_UPackage__Script_GameSettings(), TEXT("GameSettingFilterState"));
	}
	return Z_Registration_Info_UScriptStruct_GameSettingFilterState.OuterSingleton;
}
template<> GAMESETTINGS_API UScriptStruct* StaticStruct<FGameSettingFilterState>()
{
	return FGameSettingFilterState::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FGameSettingFilterState_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIncludeDisabled_MetaData[];
#endif
		static void NewProp_bIncludeDisabled_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIncludeDisabled;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIncludeHidden_MetaData[];
#endif
		static void NewProp_bIncludeHidden_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIncludeHidden;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIncludeResetable_MetaData[];
#endif
		static void NewProp_bIncludeResetable_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIncludeResetable;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_bIncludeNestedPages_MetaData[];
#endif
		static void NewProp_bIncludeNestedPages_SetBit(void* Obj);
		static const UECodeGen_Private::FBoolPropertyParams NewProp_bIncludeNestedPages;
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_SettingRootList_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SettingRootList_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_SettingRootList;
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_SettingAllowList_Inner;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_SettingAllowList_MetaData[];
#endif
		static const UECodeGen_Private::FArrayPropertyParams NewProp_SettingAllowList;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::Struct_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "/**\n * The filter state is intended to be any and all filtering we support.\n */" },
#endif
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "The filter state is intended to be any and all filtering we support." },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FGameSettingFilterState>();
	}
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
	};
#endif
	void Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled_SetBit(void* Obj)
	{
		((FGameSettingFilterState*)Obj)->bIncludeDisabled = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled = { "bIncludeDisabled", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FGameSettingFilterState), &Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled_MetaData), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
	};
#endif
	void Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden_SetBit(void* Obj)
	{
		((FGameSettingFilterState*)Obj)->bIncludeHidden = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden = { "bIncludeHidden", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FGameSettingFilterState), &Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden_MetaData), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
	};
#endif
	void Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable_SetBit(void* Obj)
	{
		((FGameSettingFilterState*)Obj)->bIncludeResetable = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable = { "bIncludeResetable", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FGameSettingFilterState), &Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable_MetaData), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable_MetaData) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
	};
#endif
	void Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages_SetBit(void* Obj)
	{
		((FGameSettingFilterState*)Obj)->bIncludeNestedPages = 1;
	}
	const UECodeGen_Private::FBoolPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages = { "bIncludeNestedPages", nullptr, (EPropertyFlags)0x0010000000000000, UECodeGen_Private::EPropertyGenFlags::Bool | UECodeGen_Private::EPropertyGenFlags::NativeBool, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, sizeof(bool), sizeof(FGameSettingFilterState), &Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages_SetBit, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages_MetaData), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages_MetaData) };
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList_Inner = { "SettingRootList", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList_MetaData[] = {
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList = { "SettingRootList", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FGameSettingFilterState, SettingRootList), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList_MetaData), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList_MetaData) };
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList_Inner = { "SettingAllowList", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_UGameSetting_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// If this is non-empty, then only settings in here are allowed\n" },
#endif
		{ "ModuleRelativePath", "Public/GameSettingFilterState.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "If this is non-empty, then only settings in here are allowed" },
#endif
	};
#endif
	const UECodeGen_Private::FArrayPropertyParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList = { "SettingAllowList", nullptr, (EPropertyFlags)0x0044000000000000, UECodeGen_Private::EPropertyGenFlags::Array, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FGameSettingFilterState, SettingAllowList), EArrayPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList_MetaData), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeDisabled,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeHidden,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeResetable,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_bIncludeNestedPages,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingRootList,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList_Inner,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewProp_SettingAllowList,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_GameSettings,
		nullptr,
		&NewStructOps,
		"GameSettingFilterState",
		Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::PropPointers),
		sizeof(FGameSettingFilterState),
		alignof(FGameSettingFilterState),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000201),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FGameSettingFilterState()
	{
		if (!Z_Registration_Info_UScriptStruct_GameSettingFilterState.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_GameSettingFilterState.InnerSingleton, Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_GameSettingFilterState.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingFilterState_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingFilterState_h_Statics::ScriptStructInfo[] = {
		{ FGameSettingFilterState::StaticStruct, Z_Construct_UScriptStruct_FGameSettingFilterState_Statics::NewStructOps, TEXT("GameSettingFilterState"), &Z_Registration_Info_UScriptStruct_GameSettingFilterState, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FGameSettingFilterState), 544065637U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingFilterState_h_4012556259(TEXT("/Script/GameSettings"),
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingFilterState_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameSettings_Source_Public_GameSettingFilterState_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
