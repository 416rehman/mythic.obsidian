// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "ShooterCoreRuntime/Public/MessageProcessors/AssistProcessor.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeAssistProcessor() {}
// Cross Module References
	ENGINE_API UClass* Z_Construct_UClass_APlayerState_NoRegister();
	MYTHIC_API UClass* Z_Construct_UClass_UGameplayMessageProcessor();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAssistProcessor();
	SHOOTERCORERUNTIME_API UClass* Z_Construct_UClass_UAssistProcessor_NoRegister();
	SHOOTERCORERUNTIME_API UScriptStruct* Z_Construct_UScriptStruct_FPlayerAssistDamageTracking();
	UPackage* Z_Construct_UPackage__Script_ShooterCoreRuntime();
// End Cross Module References
	static FStructRegistrationInfo Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking;
class UScriptStruct* FPlayerAssistDamageTracking::StaticStruct()
{
	if (!Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking.OuterSingleton)
	{
		Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking.OuterSingleton = GetStaticStruct(Z_Construct_UScriptStruct_FPlayerAssistDamageTracking, (UObject*)Z_Construct_UPackage__Script_ShooterCoreRuntime(), TEXT("PlayerAssistDamageTracking"));
	}
	return Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking.OuterSingleton;
}
template<> SHOOTERCORERUNTIME_API UScriptStruct* StaticStruct<FPlayerAssistDamageTracking>()
{
	return FPlayerAssistDamageTracking::StaticStruct();
}
	struct Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics
	{
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Struct_MetaDataParams[];
#endif
		static void* NewStructOps();
		static const UECodeGen_Private::FFloatPropertyParams NewProp_AccumulatedDamageByPlayer_ValueProp;
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_AccumulatedDamageByPlayer_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_AccumulatedDamageByPlayer_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_AccumulatedDamageByPlayer;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const UECodeGen_Private::FStructParams ReturnStructParams;
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::Struct_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Tracks the damage done to a player by other players\n" },
#endif
		{ "ModuleRelativePath", "Public/MessageProcessors/AssistProcessor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Tracks the damage done to a player by other players" },
#endif
	};
#endif
	void* Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewStructOps()
	{
		return (UScriptStruct::ICppStructOps*)new UScriptStruct::TCppStructOps<FPlayerAssistDamageTracking>();
	}
	const UECodeGen_Private::FFloatPropertyParams Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_ValueProp = { "AccumulatedDamageByPlayer", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Float, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, METADATA_PARAMS(0, nullptr) };
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_Key_KeyProp = { "AccumulatedDamageByPlayer_Key", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_APlayerState_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Map of damager to damage dealt\n" },
#endif
		{ "ModuleRelativePath", "Public/MessageProcessors/AssistProcessor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Map of damager to damage dealt" },
#endif
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer = { "AccumulatedDamageByPlayer", nullptr, (EPropertyFlags)0x0010000000002000, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(FPlayerAssistDamageTracking, AccumulatedDamageByPlayer), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_MetaData), Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_MetaData) };
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewProp_AccumulatedDamageByPlayer,
	};
	const UECodeGen_Private::FStructParams Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::ReturnStructParams = {
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
		nullptr,
		&NewStructOps,
		"PlayerAssistDamageTracking",
		Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::PropPointers,
		UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::PropPointers),
		sizeof(FPlayerAssistDamageTracking),
		alignof(FPlayerAssistDamageTracking),
		RF_Public|RF_Transient|RF_MarkAsNative,
		EStructFlags(0x00000001),
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::Struct_MetaDataParams), Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::Struct_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::PropPointers) < 2048);
	UScriptStruct* Z_Construct_UScriptStruct_FPlayerAssistDamageTracking()
	{
		if (!Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking.InnerSingleton)
		{
			UECodeGen_Private::ConstructUScriptStruct(Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking.InnerSingleton, Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::ReturnStructParams);
		}
		return Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking.InnerSingleton;
	}
	void UAssistProcessor::StaticRegisterNativesUAssistProcessor()
	{
	}
	IMPLEMENT_CLASS_NO_AUTO_REGISTRATION(UAssistProcessor);
	UClass* Z_Construct_UClass_UAssistProcessor_NoRegister()
	{
		return UAssistProcessor::StaticClass();
	}
	struct Z_Construct_UClass_UAssistProcessor_Statics
	{
		static UObject* (*const DependentSingletons[])();
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Class_MetaDataParams[];
#endif
		static const UECodeGen_Private::FStructPropertyParams NewProp_DamageHistory_ValueProp;
		static const UECodeGen_Private::FObjectPtrPropertyParams NewProp_DamageHistory_Key_KeyProp;
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam NewProp_DamageHistory_MetaData[];
#endif
		static const UECodeGen_Private::FMapPropertyParams NewProp_DamageHistory;
		static const UECodeGen_Private::FPropertyParamsBase* const PropPointers[];
		static const FCppClassTypeInfoStatic StaticCppClassTypeInfo;
		static const UECodeGen_Private::FClassParams ClassParams;
	};
	UObject* (*const Z_Construct_UClass_UAssistProcessor_Statics::DependentSingletons[])() = {
		(UObject* (*)())Z_Construct_UClass_UGameplayMessageProcessor,
		(UObject* (*)())Z_Construct_UPackage__Script_ShooterCoreRuntime,
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAssistProcessor_Statics::DependentSingletons) < 16);
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAssistProcessor_Statics::Class_MetaDataParams[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Tracks assists (dealing damage to another player without finishing them)\n" },
#endif
		{ "IncludePath", "MessageProcessors/AssistProcessor.h" },
		{ "ModuleRelativePath", "Public/MessageProcessors/AssistProcessor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Tracks assists (dealing damage to another player without finishing them)" },
#endif
	};
#endif
	const UECodeGen_Private::FStructPropertyParams Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_ValueProp = { "DamageHistory", nullptr, (EPropertyFlags)0x0000000000000000, UECodeGen_Private::EPropertyGenFlags::Struct, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 1, Z_Construct_UScriptStruct_FPlayerAssistDamageTracking, METADATA_PARAMS(0, nullptr) }; // 4227000023
	const UECodeGen_Private::FObjectPtrPropertyParams Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_Key_KeyProp = { "DamageHistory_Key", nullptr, (EPropertyFlags)0x0004000000000000, UECodeGen_Private::EPropertyGenFlags::Object | UECodeGen_Private::EPropertyGenFlags::ObjectPtr, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, 0, Z_Construct_UClass_APlayerState_NoRegister, METADATA_PARAMS(0, nullptr) };
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_MetaData[] = {
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Map of player to damage dealt to them\n" },
#endif
		{ "ModuleRelativePath", "Public/MessageProcessors/AssistProcessor.h" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Map of player to damage dealt to them" },
#endif
	};
#endif
	const UECodeGen_Private::FMapPropertyParams Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory = { "DamageHistory", nullptr, (EPropertyFlags)0x0040000000002000, UECodeGen_Private::EPropertyGenFlags::Map, RF_Public|RF_Transient|RF_MarkAsNative, nullptr, nullptr, 1, STRUCT_OFFSET(UAssistProcessor, DamageHistory), EMapPropertyFlags::None, METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_MetaData), Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_MetaData) }; // 4227000023
	const UECodeGen_Private::FPropertyParamsBase* const Z_Construct_UClass_UAssistProcessor_Statics::PropPointers[] = {
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_ValueProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory_Key_KeyProp,
		(const UECodeGen_Private::FPropertyParamsBase*)&Z_Construct_UClass_UAssistProcessor_Statics::NewProp_DamageHistory,
	};
	const FCppClassTypeInfoStatic Z_Construct_UClass_UAssistProcessor_Statics::StaticCppClassTypeInfo = {
		TCppClassTypeTraits<UAssistProcessor>::IsAbstract,
	};
	const UECodeGen_Private::FClassParams Z_Construct_UClass_UAssistProcessor_Statics::ClassParams = {
		&UAssistProcessor::StaticClass,
		"Engine",
		&StaticCppClassTypeInfo,
		DependentSingletons,
		nullptr,
		Z_Construct_UClass_UAssistProcessor_Statics::PropPointers,
		nullptr,
		UE_ARRAY_COUNT(DependentSingletons),
		0,
		UE_ARRAY_COUNT(Z_Construct_UClass_UAssistProcessor_Statics::PropPointers),
		0,
		0x00A000A4u,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UClass_UAssistProcessor_Statics::Class_MetaDataParams), Z_Construct_UClass_UAssistProcessor_Statics::Class_MetaDataParams)
	};
	static_assert(UE_ARRAY_COUNT(Z_Construct_UClass_UAssistProcessor_Statics::PropPointers) < 2048);
	UClass* Z_Construct_UClass_UAssistProcessor()
	{
		if (!Z_Registration_Info_UClass_UAssistProcessor.OuterSingleton)
		{
			UECodeGen_Private::ConstructUClass(Z_Registration_Info_UClass_UAssistProcessor.OuterSingleton, Z_Construct_UClass_UAssistProcessor_Statics::ClassParams);
		}
		return Z_Registration_Info_UClass_UAssistProcessor.OuterSingleton;
	}
	template<> SHOOTERCORERUNTIME_API UClass* StaticClass<UAssistProcessor>()
	{
		return UAssistProcessor::StaticClass();
	}
	UAssistProcessor::UAssistProcessor(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer) {}
	DEFINE_VTABLE_PTR_HELPER_CTOR(UAssistProcessor);
	UAssistProcessor::~UAssistProcessor() {}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics
	{
		static const FStructRegisterCompiledInInfo ScriptStructInfo[];
		static const FClassRegisterCompiledInInfo ClassInfo[];
	};
	const FStructRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics::ScriptStructInfo[] = {
		{ FPlayerAssistDamageTracking::StaticStruct, Z_Construct_UScriptStruct_FPlayerAssistDamageTracking_Statics::NewStructOps, TEXT("PlayerAssistDamageTracking"), &Z_Registration_Info_UScriptStruct_PlayerAssistDamageTracking, CONSTRUCT_RELOAD_VERSION_INFO(FStructReloadVersionInfo, sizeof(FPlayerAssistDamageTracking), 4227000023U) },
	};
	const FClassRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics::ClassInfo[] = {
		{ Z_Construct_UClass_UAssistProcessor, UAssistProcessor::StaticClass, TEXT("UAssistProcessor"), &Z_Registration_Info_UClass_UAssistProcessor, CONSTRUCT_RELOAD_VERSION_INFO(FClassReloadVersionInfo, sizeof(UAssistProcessor), 2513396070U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_2998081538(TEXT("/Script/ShooterCoreRuntime"),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics::ClassInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics::ClassInfo),
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics::ScriptStructInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameFeatures_ShooterCore_Source_ShooterCoreRuntime_Public_MessageProcessors_AssistProcessor_h_Statics::ScriptStructInfo),
		nullptr, 0);
PRAGMA_ENABLE_DEPRECATION_WARNINGS
