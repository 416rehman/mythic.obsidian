// Copyright Epic Games, Inc. All Rights Reserved.
/*===========================================================================
	Generated code exported from UnrealHeaderTool.
	DO NOT modify this manually! Edit the corresponding .h files instead!
===========================================================================*/

#include "UObject/GeneratedCppIncludes.h"
#include "GameplayMessageRuntime/Public/GameFramework/GameplayMessageTypes2.h"
PRAGMA_DISABLE_DEPRECATION_WARNINGS
void EmptyLinkFunctionForGeneratedCodeGameplayMessageTypes2() {}
// Cross Module References
	GAMEPLAYMESSAGERUNTIME_API UEnum* Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch();
	UPackage* Z_Construct_UPackage__Script_GameplayMessageRuntime();
// End Cross Module References
	static FEnumRegistrationInfo Z_Registration_Info_UEnum_EGameplayMessageMatch;
	static UEnum* EGameplayMessageMatch_StaticEnum()
	{
		if (!Z_Registration_Info_UEnum_EGameplayMessageMatch.OuterSingleton)
		{
			Z_Registration_Info_UEnum_EGameplayMessageMatch.OuterSingleton = GetStaticEnum(Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch, (UObject*)Z_Construct_UPackage__Script_GameplayMessageRuntime(), TEXT("EGameplayMessageMatch"));
		}
		return Z_Registration_Info_UEnum_EGameplayMessageMatch.OuterSingleton;
	}
	template<> GAMEPLAYMESSAGERUNTIME_API UEnum* StaticEnum<EGameplayMessageMatch>()
	{
		return EGameplayMessageMatch_StaticEnum();
	}
	struct Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics
	{
		static const UECodeGen_Private::FEnumeratorParam Enumerators[];
#if WITH_METADATA
		static const UECodeGen_Private::FMetaDataPairParam Enum_MetaDataParams[];
#endif
		static const UECodeGen_Private::FEnumParams EnumParams;
	};
	const UECodeGen_Private::FEnumeratorParam Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::Enumerators[] = {
		{ "EGameplayMessageMatch::ExactMatch", (int64)EGameplayMessageMatch::ExactMatch },
		{ "EGameplayMessageMatch::PartialMatch", (int64)EGameplayMessageMatch::PartialMatch },
	};
#if WITH_METADATA
	const UECodeGen_Private::FMetaDataPairParam Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::Enum_MetaDataParams[] = {
		{ "BlueprintType", "true" },
#if !UE_BUILD_SHIPPING
		{ "Comment", "// Match rule for message listeners\n" },
#endif
		{ "ExactMatch.Comment", "// An exact match will only receive messages with exactly the same channel\n// (e.g., registering for \"A.B\" will match a broadcast of A.B but not A.B.C)\n" },
		{ "ExactMatch.Name", "EGameplayMessageMatch::ExactMatch" },
		{ "ExactMatch.ToolTip", "An exact match will only receive messages with exactly the same channel\n(e.g., registering for \"A.B\" will match a broadcast of A.B but not A.B.C)" },
		{ "ModuleRelativePath", "Public/GameFramework/GameplayMessageTypes2.h" },
		{ "PartialMatch.Comment", "// A partial match will receive any messages rooted in the same channel\n// (e.g., registering for \"A.B\" will match a broadcast of A.B as well as A.B.C)\n" },
		{ "PartialMatch.Name", "EGameplayMessageMatch::PartialMatch" },
		{ "PartialMatch.ToolTip", "A partial match will receive any messages rooted in the same channel\n(e.g., registering for \"A.B\" will match a broadcast of A.B as well as A.B.C)" },
#if !UE_BUILD_SHIPPING
		{ "ToolTip", "Match rule for message listeners" },
#endif
	};
#endif
	const UECodeGen_Private::FEnumParams Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::EnumParams = {
		(UObject*(*)())Z_Construct_UPackage__Script_GameplayMessageRuntime,
		nullptr,
		"EGameplayMessageMatch",
		"EGameplayMessageMatch",
		Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::Enumerators,
		RF_Public|RF_Transient|RF_MarkAsNative,
		UE_ARRAY_COUNT(Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::Enumerators),
		EEnumFlags::None,
		(uint8)UEnum::ECppForm::EnumClass,
		METADATA_PARAMS(UE_ARRAY_COUNT(Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::Enum_MetaDataParams), Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::Enum_MetaDataParams)
	};
	UEnum* Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch()
	{
		if (!Z_Registration_Info_UEnum_EGameplayMessageMatch.InnerSingleton)
		{
			UECodeGen_Private::ConstructUEnum(Z_Registration_Info_UEnum_EGameplayMessageMatch.InnerSingleton, Z_Construct_UEnum_GameplayMessageRuntime_EGameplayMessageMatch_Statics::EnumParams);
		}
		return Z_Registration_Info_UEnum_EGameplayMessageMatch.InnerSingleton;
	}
	struct Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_GameplayMessageTypes2_h_Statics
	{
		static const FEnumRegisterCompiledInInfo EnumInfo[];
	};
	const FEnumRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_GameplayMessageTypes2_h_Statics::EnumInfo[] = {
		{ EGameplayMessageMatch_StaticEnum, TEXT("EGameplayMessageMatch"), &Z_Registration_Info_UEnum_EGameplayMessageMatch, CONSTRUCT_RELOAD_VERSION_INFO(FEnumReloadVersionInfo, 3309916309U) },
	};
	static FRegisterCompiledInInfo Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_GameplayMessageTypes2_h_275582762(TEXT("/Script/GameplayMessageRuntime"),
		nullptr, 0,
		nullptr, 0,
		Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_GameplayMessageTypes2_h_Statics::EnumInfo, UE_ARRAY_COUNT(Z_CompiledInDeferFile_FID_Mythic_Plugins_GameplayMessageRouter_Source_GameplayMessageRuntime_Public_GameFramework_GameplayMessageTypes2_h_Statics::EnumInfo));
PRAGMA_ENABLE_DEPRECATION_WARNINGS
